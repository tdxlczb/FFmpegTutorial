#include <iostream>
#include <string>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <functional>
#include <chrono>
#include <logger/logger.h>
#include <foundation/file/file_utils.h>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}


extern "C" {
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#include <libavutil/opt.h>
}

class AudioSpeedFilter {
private:
    AVFilterGraph* filterGraph = nullptr;
    AVFilterContext* inFilterCtx = nullptr;    // abuffer（输入）
    AVFilterContext* outFilterCtx = nullptr;   // abuffersink（输出）
    AVFilterContext* atempoCtx = nullptr;      // atempo（倍速）

public:
    // 初始化滤镜图
    bool init(int sampleRate, AVSampleFormat sampleFmt, int64_t channelLayout, double speed) {
        // 1. 创建滤镜图
        filterGraph = avfilter_graph_alloc();
        if (!filterGraph) return false;

        // 2. 创建 abuffer 滤镜（接收解码后的音频帧）
        const AVFilter* abuffer = avfilter_get_by_name("abuffer");
        if (!abuffer) return false;

        // 设置音频参数：采样率、格式、声道布局
        char args[512];
        snprintf(args, sizeof(args), "time_base=%d/1:sample_rate=%d:sample_fmt=%s:channel_layout=0x%" PRIx64,
            1, sampleRate, av_get_sample_fmt_name(sampleFmt), channelLayout);

        int ret = avfilter_graph_create_filter(&inFilterCtx, abuffer, "in", args, nullptr, filterGraph);
        if (ret < 0) return false;

        // 3. 创建 atempo 滤镜（核心倍速）
        const AVFilter* atempo = avfilter_get_by_name("atempo");
        if (!atempo) return false;

        // 设置倍速参数（0.5~2.0，超过需串联多个）
        char speedStr[32];
        snprintf(speedStr, sizeof(speedStr), "%.2f", speed);

        ret = avfilter_graph_create_filter(&atempoCtx, atempo, "atempo", speedStr, nullptr, filterGraph);
        if (ret < 0) return false;

        // 4. 创建 abuffersink 滤镜（获取处理后的帧）
        const AVFilter* abuffersink = avfilter_get_by_name("abuffersink");
        if (!abuffersink) return false;

        ret = avfilter_graph_create_filter(&outFilterCtx, abuffersink, "out", nullptr, nullptr, filterGraph);
        if (ret < 0) return false;

        // 5. 连接滤镜：abuffer -> atempo -> abuffersink
        ret = avfilter_link(inFilterCtx, 0, atempoCtx, 0);
        if (ret < 0) return false;

        ret = avfilter_link(atempoCtx, 0, outFilterCtx, 0);
        if (ret < 0) return false;

        // 6. 配置滤镜图
        ret = avfilter_graph_config(filterGraph, nullptr);
        if (ret < 0) return false;

        return true;
    }

    // 发送音频帧到滤镜
    bool sendFrame(AVFrame* frame) {
        int ret = av_buffersrc_add_frame(inFilterCtx, frame);
        return ret >= 0;
    }

    // 从滤镜获取处理后的帧
    AVFrame* receiveFrame() {
        AVFrame* filteredFrame = av_frame_alloc();
        int ret = av_buffersink_get_frame(outFilterCtx, filteredFrame);

        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            av_frame_free(&filteredFrame);
            return nullptr; // 需要更多输入或已经结束
        }
        if (ret < 0) {
            av_frame_free(&filteredFrame);
            return nullptr; // 错误
        }

        return filteredFrame; // 返回倍速后的音频帧
    }

    // 清理资源
    void destroy() {
        avfilter_graph_free(&filterGraph); // 会自动释放所有滤镜上下文
    }
};

#include <fstream>
static std::ofstream g_pcmFilter;
void processAudioFrame(AVFrame* decodedFrame, AudioSpeedFilter& filter) {
    // 1. 发送解码帧到滤镜
    filter.sendFrame(decodedFrame);

    // 2. 循环获取倍速后的帧（可能1输入对应多输出或少输出）
    while (true) {
        AVFrame* filteredFrame = filter.receiveFrame();
        if (!filteredFrame) break; // 没有更多输出

        // 3. 将 filteredFrame 送入音频播放设备
        // audioDevice.play(filteredFrame);

        if (g_pcmFilter.is_open())
        {
            size_t bufferSize = av_samples_get_buffer_size(filteredFrame->linesize, filteredFrame->channels, filteredFrame->nb_samples, (AVSampleFormat)filteredFrame->format, 1);
            g_pcmFilter.write(reinterpret_cast<const char*>(filteredFrame->data[0]), bufferSize);
            g_pcmFilter.flush();
        }

        // 4. 释放处理后的帧
        av_frame_free(&filteredFrame);
    }
}

struct CmdData
{
    bool isDebug = false;
    bool isStop = false;
};

using DecodeCallback = std::function<void(uint8_t* data, uint64_t len, bool isEnd)>;
bool AudioDecode(const std::string& filePath, const DecodeCallback& callback, CmdData &cmd)
{
    LOG_INFO << "start decode audio:" << filePath;
    avformat_network_init();

    g_pcmFilter.open("audio_convert_filter.pcm", std::ios::binary);
    if (!g_pcmFilter.is_open())
    {
        LOG_ERROR << "open failed";
    }

    // 打开文件
    AVFormatContext* formatContext = nullptr;
    if (avformat_open_input(&formatContext, filePath.c_str(), nullptr, nullptr) != 0)
    {
        LOG_ERROR << "avformat_open_input failed";
        // 处理打开文件失败的情况
        avformat_network_deinit();
        return false;
    }
    // 获取音频流信息
    if (avformat_find_stream_info(formatContext, nullptr) < 0)
    {
        LOG_ERROR << "avformat_find_stream_info failed";
        // 处理获取流信息失败的情况
        avformat_close_input(&formatContext);
        avformat_network_deinit();
        return false;
    }

    const AVCodec* codec            = nullptr;
    int      audioStreamIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, &codec, 0);
    if (audioStreamIndex < 0)
    {
        LOG_ERROR << "av_find_best_stream failed";
        // 处理无法找到音频流的情况
        avformat_close_input(&formatContext);
        avformat_network_deinit();
        return false;
    }

    AVCodecParameters* codecParameters = formatContext->streams[audioStreamIndex]->codecpar;
    LOG_INFO << "sample_fmt:" << av_get_sample_fmt_name((AVSampleFormat) codecParameters->format) << ", sample_rate:" << codecParameters->sample_rate
             << ", channels:" << codecParameters->channels;

    AVCodecContext* codecContext = avcodec_alloc_context3(codec);
    if (avcodec_parameters_to_context(codecContext, codecParameters) < 0)
    {
        LOG_ERROR << "avcodec_parameters_to_context failed";
        // 处理无法将参数设置到解码器上下文的情况
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        avformat_network_deinit();
        return false;
    }
    codecContext->thread_count = 2;
    if (avcodec_open2(codecContext, codec, nullptr) != 0)
    {
        LOG_ERROR << "avcodec_open2 failed";
        // 处理无法打开解码器的情况
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        avformat_network_deinit();
        return false;
    }

    const int      in_sample_rate     = codecContext->sample_rate;
    AVSampleFormat in_sfmt            = codecContext->sample_fmt;
    uint64_t       in_channel_layout  = codecContext->channel_layout;
    int            in_channels        = codecContext->channels;
    const int      out_sample_rate    = 16000;
    AVSampleFormat out_sfmt           = AV_SAMPLE_FMT_S16;
    uint64_t       out_channel_layout = AV_CH_LAYOUT_STEREO;
    int            out_channels       = av_get_channel_layout_nb_channels(out_channel_layout);
    int            out_spb            = av_get_bytes_per_sample(out_sfmt);

    bool needResample = false;
    if ((in_sample_rate != out_sample_rate) || (in_sfmt != out_sfmt) || (in_channels != out_channels))
        needResample = true;

    // 创建重采样上下文
    SwrContext* swrContext = nullptr;
    if (needResample)
    {
        swrContext = swr_alloc_set_opts(nullptr, out_channel_layout, out_sfmt, out_sample_rate, in_channel_layout, in_sfmt, in_sample_rate, 0, NULL);
        if (!swrContext)
        {
            // 处理重采样上下文创建失败的情况
            LOG_ERROR << "swr_alloc_set_opts failed";
            return false;
        }
        if (swr_init(swrContext) != 0)
        {
            LOG_ERROR << "swr_alloc_set_opts failed";
            return false;
        }
    }

    std::ofstream ofs1;
    std::ofstream ofs2;
    if (cmd.isDebug)
    {
        std::string outPath = R"(E:\code\media\temp\)";
        auto        fmtName = av_get_sample_fmt_name(in_sfmt);
        std::string pcmPath = outPath + "_" + fmtName + "_" + std::to_string(in_channels) + "_" + std::to_string(in_sample_rate) + ".pcm";
        ofs1.open(pcmPath, std::ios::binary);
        if (!ofs1.is_open())
        {
            LOG_ERROR << "open file failed:" << pcmPath;
        }
        if (needResample)
        {
            auto        outfmtName = av_get_sample_fmt_name(out_sfmt);
            std::string resamplePcmPath =
                outPath + "_" + outfmtName + "_" + std::to_string(out_channels) + "_" + std::to_string(out_sample_rate) + ".pcm";
            ofs2.open(resamplePcmPath, std::ios::binary);
            if (!ofs2.is_open())
            {
                LOG_ERROR << "open file failed:" << resamplePcmPath;
            }
        }
    }

    AudioSpeedFilter filter;
    filter.init(codecParameters->sample_rate, (AVSampleFormat)codecParameters->format, codecParameters->channel_layout, 2.0); // 设置1.5倍速

    int       frameIndex  = -1;
    int       packetIndex = -1;
    AVPacket* packet      = av_packet_alloc();
    AVFrame*  frame       = av_frame_alloc();
    while (!cmd.isStop && av_read_frame(formatContext, packet) >= 0)
    {
        if (packet->stream_index == audioStreamIndex)
        {
            packetIndex++;
            if (avcodec_send_packet(codecContext, packet) < 0)
            {
                // 处理发送数据包到解码器失败的情况
                av_packet_unref(packet);
                break;
            }
            while (avcodec_receive_frame(codecContext, frame) >= 0)
            {
                processAudioFrame(frame, filter);

                frameIndex++;

                if (needResample)
                {
                    // 计算重采样输出采样点数
                    int max_out_nb_samples = av_rescale_rnd(
                        swr_get_delay(swrContext, frame->sample_rate) + frame->nb_samples, out_sample_rate, frame->sample_rate, AV_ROUND_UP
                    );
                    AVFrame* frameResample = av_frame_alloc();
                    //使用av_samples_alloc时,结束后需要调用av_freep(&audio_data[0])释放内存,否则会内存泄漏
                    av_samples_alloc(frameResample->data, frameResample->linesize, out_channels, max_out_nb_samples, out_sfmt, 1);

                    int out_nb_samples =
                        swr_convert(swrContext, frameResample->data, max_out_nb_samples, (const uint8_t**) frame->data, frame->nb_samples);
                    LOG_INFO << "succeed to convert frame " << frameIndex++ << " samples[" << frame->nb_samples << "]->[" << out_nb_samples << "]";

                    // 将重采样后的音频数据写入输出文件
                    if (ofs2.is_open())
                        ofs2.write(reinterpret_cast<const char*>(frameResample->data[0]), out_spb * out_channels * out_nb_samples);

                    if (callback)
                        callback(frameResample->data[0], out_spb * out_channels * out_nb_samples, false);

                    av_freep(&frameResample->data[0]);
                    av_frame_unref(frameResample);
                    av_frame_free(&frameResample);
                }
                else
                {
                    if (callback)
                        callback(frame->data[0], frame->linesize[0], false);
                }
                if (ofs1.is_open())
                {
                    ofs1.write(reinterpret_cast<const char*>(frame->data[0]), frame->linesize[0]);
                }
                av_frame_unref(frame);
            }
            av_frame_unref(frame);
        }
        av_packet_unref(packet);
    }

    if (needResample)
    {
        int      max_cache_out_nb_samples = 2048;
        AVFrame* frameResample            = av_frame_alloc();
        //使用av_samples_alloc时,结束后需要调用av_freep(&audio_data[0])释放内存,否则会内存泄漏
        av_samples_alloc(frameResample->data, frameResample->linesize, out_channels, max_cache_out_nb_samples, out_sfmt, 1);

        int out_cache_nb_samples = swr_convert(swrContext, frameResample->data, max_cache_out_nb_samples, nullptr, 0);
        LOG_INFO << "get cache convert samples " << out_cache_nb_samples;

        // 将重采样后的音频数据写入输出文件
        if (ofs2.is_open())
            ofs2.write(reinterpret_cast<const char*>(frameResample->data[0]), out_spb * out_channels * out_cache_nb_samples);

        if (callback)
            callback(frameResample->data[0], out_spb * out_channels * out_cache_nb_samples, false);

        av_freep(&frameResample->data[0]);
        av_frame_unref(frameResample);
        av_frame_free(&frameResample);
    }

    av_packet_unref(packet);
    av_packet_free(&packet);
    av_frame_unref(frame);
    av_frame_free(&frame);

    swr_free(&swrContext);
    avcodec_free_context(&codecContext);
    avformat_close_input(&formatContext);
    avformat_network_deinit();

    // 关闭输出文件
    if (ofs1.is_open())
    {
        ofs1.close();
    }
    if (ofs2.is_open())
    {
        ofs2.close();
    }
    LOG_INFO << "end decode audio:" << filePath;
    return true;
}
#include <fstream>
#include <thread>

int main()
{
    //InitLogger();
    LOG_INFO << "==================================";
    //test();
    std::string output    = "E:\\res\\mca\\output";
    std::string filePath  = R"(rtsp://172.16.19.40:554/rtp/34020000001180000195_34020000001310000004_3?token=8mHubssH9NKUXevp)";
    std::string filePath1 = "E:\\res\\mca\\dvrStorage1231\\media\\edulyse-edge-windows\\1703591015324_6\\1703591015324_6_0_13348064620365.ts";
    std::string filePath2 = "E:\\res\\mca\\1704177600496_4_0_13348651242807.ts";
    std::string filePath3 = "E:\\res\\mca\\1703762903540_2\\1703762903540_2_0_13348236794018.ts";
    std::string filePath4 =
        R"(E:\res\mca\EdulyseEdgeWindows\dvrStorage\media\edulyse-edge-windows\1704196735104_2\1704196735104_2_0_13348670347631.ts)";
    std::string filePath5 = R"(E:\code\media\BaiduSyncdisk.mp4)";
    std::string filePath6 = R"(E:\res\mca\d6bda0290395c01e874326aa364426c3_SK_3999470_4003600.wav)";
    //std::string filePath = "E:\\res\\mca\\1701047259978_141\\1701047259978_141_0_1701047320961.ts";
    //std::string filePath = "E:\\res\\mca\\0.wav";
    //std::string filePath = "E:\\res\\HOYO-MiX-DaCapo.flac";
    //std::string filePath5 = "E:\\res\\output.flac";
    //std::string filePath1 = "E:\\res\\mca\\dvrStorage1231\\media\\edulyse-edge-windows\\1703591015324_6\\1703591015324_6_0_13348064620365.ts";
    //std::string filePath2 = R"(E:\res\mca\test.mp4)";

    CmdData data;
    data.isDebug = true;
    std::thread th([&]() {
        AudioDecode(filePath5, NULL, data);
        });
    getchar();
    data.isStop = true;
    th.join();

    return 0;
    //AudioDecode(filePath, NULL, true);
    //getchar();

    std::string dir1 = R"(E:\res\mca\dvrStorage1231\media\edulyse-edge-windows\1703591015324_6)";
    std::string dir3 = R"(E:\res\mca\1703762903540_2)";
    std::string dir4 = R"(E:\res\mca\EdulyseEdgeWindows\dvrStorage\media\edulyse-edge-windows\1704196735104_2)";
    std::string dir5 = R"(E:\res\mca\1701047259978_141)";

    std::vector<std::string> fileList;
    foundation::FileUtils::GetFileList2(dir5, fileList, "*.ts");

    int index = 0;
    for (size_t i = 0; i < fileList.size(); i++)
    {
        index++;
        auto startTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        bool ret       = AudioDecode(fileList[i], NULL, data);
        if (!ret)
        {
            std::cout << "err index=" << index << std::endl;
        }
        auto endTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        std::cout << "duration=" << endTime - startTime << std::endl;
    }

    getchar();
    return 0;
}

#include <iostream>
#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <fstream>

std::string SecondsToStr(uint64_t time)
{
    int hours   = time / 3600;
    int minutes = (time - hours * 3600) / 60;
    int seconds = (time - hours * 3600 - minutes * 60);

    std::stringstream ss;
    ss << std::setfill('0') << std::setw(2) << hours << ":" << std::setfill('0') << std::setw(2) << minutes << ":" << std::setfill('0')
       << std::setw(2) << seconds;
    return ss.str();
}

int main2()
{
    std::string   filePath = "E:/res/mca/student.mp4";
    std::string   destDir  = "E:/res/mca/student";
    std::string   scpPath  = "E:/res/mca/student/wav.scp";
    std::ofstream ofs(scpPath);

    uint64_t st = 0;
    for (size_t i = 0; i < 100; i++)
    {
        std::stringstream ss;
        ss << std::setfill('0') << std::setw(3) << i + 1;
        std::string destPath = destDir + "/" + ss.str() + ".wav";

        std::string start = SecondsToStr(st);
        st += 10;
        std::string command = "ffmpeg -i " + filePath + " -ss " + start + " -t 00:00:10 -vn -acodec pcm_s16le -ar 16000 -ac 1 -f wav " + destPath;
        int         result  = std::system(command.c_str());
        if (result == 0)
        {
            std::cout << "Command executed successfully" << std::endl;
            std::string data = "1   " + destPath + "\n";
            ofs.write(data.c_str(), data.length());
        }
        else
        {
            std::cerr << "Command execution failed" << std::endl;
        }
    }
    //ffmpeg -i input.mp4 -ss 00:02:00 -t 00:01:00 -vn -acodec pcm_s16le -ar 16000 -ac 1 -f wav output.wav
    return 0;
}
