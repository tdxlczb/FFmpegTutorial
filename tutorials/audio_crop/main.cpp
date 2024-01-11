#include <iostream>
#include <string>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <functional>
#include <logger/logger.h>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}

using DecodeCallback = std::function<void(uint8_t* data, uint64_t len, bool isEnd)>;
int AudioDecode(const std::string& filePath, const DecodeCallback& callback, bool isDebug)
{
    LOG_INFO << "start decode audio:" << filePath;
    int ret = 0;
    avformat_network_init();

    AVFormatContext* formatContext = nullptr;
    ret                            = avformat_open_input(&formatContext, filePath.c_str(), nullptr, nullptr);
    if (ret != 0)
    {
        // 处理打开文件失败的情况
        LOG_ERROR << "avformat_open_input failed:" << ret;
        return -1;
    }
    ret = avformat_find_stream_info(formatContext, nullptr);
    if (ret < 0)
    {
        // 处理无法找到流信息的情况
        LOG_ERROR << "avformat_find_stream_info failed:" << ret;
        return -1;
    }

    const AVCodec*     codec            = nullptr;
    AVCodecParameters* codecParameters  = nullptr;
    int                audioStreamIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, &codec, 0);
    if (audioStreamIndex < 0)
    {
        // 处理无法找到音频流的情况
        LOG_ERROR << "av_find_best_stream failed:" << audioStreamIndex;
        return -1;
    }

    codecParameters = formatContext->streams[audioStreamIndex]->codecpar;
    LOG_INFO << "sample_fmt:" << av_get_sample_fmt_name((AVSampleFormat) codecParameters->format) << ", sample_rate:" << codecParameters->sample_rate
             << ", channels:" << codecParameters->channels;

    AVCodecContext* codecContext = avcodec_alloc_context3(codec);
    ret                          = avcodec_parameters_to_context(codecContext, codecParameters);
    if (ret < 0)
    {
        // 处理无法将参数设置到解码器上下文的情况
        LOG_ERROR << "avcodec_parameters_to_context failed:" << ret;
        return -1;
    }
    ret = avcodec_open2(codecContext, codec, nullptr);
    if (ret != 0)
    {
        // 处理无法打开解码器的情况
        LOG_ERROR << "avcodec_open2 failed:" << ret;
        return -1;
    }

    const int      in_sample_rate     = codecContext->sample_rate;
    AVSampleFormat in_sfmt            = codecContext->sample_fmt;
    uint64_t       in_channel_layout  = codecContext->channel_layout;
    int            in_channels        = codecContext->ch_layout.nb_channels;
    const int      out_sample_rate    = 16000;
    AVSampleFormat out_sfmt           = AV_SAMPLE_FMT_S16;
    uint64_t       out_channel_layout = AV_CH_LAYOUT_MONO;
    int            out_channels       = av_get_channel_layout_nb_channels(out_channel_layout);
    int            out_spb            = av_get_bytes_per_sample(out_sfmt);

    bool needResample = false;
    if (in_sample_rate != out_sample_rate)
        needResample = true;
    if (in_sfmt != out_sfmt)
        needResample = true;
    if (in_channels != out_channels)
        needResample = true;

    // 创建重采样上下文
    SwrContext* swrContext;
    if (needResample)
    {
        swrContext = swr_alloc_set_opts(nullptr, out_channel_layout, out_sfmt, out_sample_rate, in_channel_layout, in_sfmt, in_sample_rate, 0, NULL);
        if (!swrContext)
        {
            // 处理重采样上下文创建失败的情况
            LOG_ERROR << "swr_alloc_set_opts failed";
            return -1;
        }

        ret = swr_init(swrContext);
        if (ret != 0)
        {
            LOG_ERROR << "swr_alloc_set_opts failed:" << ret;
            return -1;
        }
    }
    std::ofstream ofs1;
    std::ofstream ofs2;
    if (isDebug)
    {
        auto        fmtName = av_get_sample_fmt_name(in_sfmt);
        std::string pcmPath = filePath + "_" + fmtName + "_" + std::to_string(in_channels) + "_" + std::to_string(in_sample_rate) + ".pcm";
        ofs1.open(pcmPath, std::ios::binary);
        if (!ofs1.is_open())
        {
            LOG_ERROR << "open file failed:" << pcmPath;
        }
        if (needResample)
        {
            auto        outfmtName = av_get_sample_fmt_name(out_sfmt);
            std::string resamplePcmPath =
                filePath + "_" + outfmtName + "_" + std::to_string(out_channels) + "_" + std::to_string(out_sample_rate) + ".pcm";
            ofs2.open(resamplePcmPath, std::ios::binary);
            if (!ofs2.is_open())
            {
                LOG_ERROR << "open file failed:" << resamplePcmPath;
            }
        }
    }

    int       frameCnt = 0;
    AVPacket* packet   = av_packet_alloc();
    AVFrame*  frame    = av_frame_alloc();
    while (av_read_frame(formatContext, packet) >= 0)
    {
        if (packet->stream_index == audioStreamIndex)
        {
            if (avcodec_send_packet(codecContext, packet) < 0)
            {
                // 处理发送数据包到解码器失败的情况
                break;
            }

            while (avcodec_receive_frame(codecContext, frame) >= 0)
            {
                if (needResample)
                {
                    // 重采样
                    int out_nb_samples = av_rescale_rnd(
                        swr_get_delay(swrContext, frame->sample_rate) + frame->nb_samples, out_sample_rate, frame->sample_rate, AV_ROUND_UP
                    );
                    AVFrame* resampledFrame = av_frame_alloc();
                    av_samples_alloc(resampledFrame->data, resampledFrame->linesize, out_channels, out_nb_samples, out_sfmt, 1);

                    int out_samples = swr_convert(swrContext, resampledFrame->data, out_nb_samples, (const uint8_t**) frame->data, frame->nb_samples);
                    // LOG_INFO << "succeed to convert frame " << frameCnt++ << " samples[" << frame->nb_samples << "]->[" << out_samples << "]";

                    // 将重采样后的音频数据写入输出文件
                    if (ofs2.is_open())
                        ofs2.write(reinterpret_cast<const char*>(resampledFrame->data[0]), out_spb * out_channels * out_samples);

                    if (callback)
                        callback(resampledFrame->data[0], out_spb * out_channels * out_samples, false);

                    av_freep(&resampledFrame->data[0]);
                    av_frame_free(&resampledFrame);
                }
                else
                {
                    if (callback)
                        callback(frame->data[0], frame->linesize[0], false);
                }
                if (ofs1.is_open())
                    ofs1.write(reinterpret_cast<const char*>(frame->data[0]), frame->linesize[0]);
            }
        }
        av_packet_unref(packet);
    }

    if (needResample)
    {
        AVFrame* cache_out_frame = av_frame_alloc();
        av_samples_alloc(cache_out_frame->data, cache_out_frame->linesize, out_channels, 2048, out_sfmt, 1);
        int cache_out_samples = swr_convert(swrContext, cache_out_frame->data, 2048, NULL, 0);

        if (ofs2.is_open())
            ofs2.write(reinterpret_cast<const char*>(cache_out_frame->data[0]), out_spb * out_channels * cache_out_samples);

        if (callback)
            callback(cache_out_frame->data[0], out_spb * out_channels * cache_out_samples, true);

        av_freep(&cache_out_frame->data[0]);
        av_frame_free(&cache_out_frame);
        swr_free(&swrContext);
    }

    av_packet_free(&packet);
    av_frame_free(&frame);
    avcodec_free_context(&codecContext);
    avformat_close_input(&formatContext);

    // 关闭输出文件
    if (ofs1.is_open())
    {
        ofs1.close();
    }
    if (ofs2.is_open())
    {
        ofs2.close();
    }

    return 0;
}
#include <fstream>

int main()
{
    //std::string filePath = "E:\\res\\test-wav\\1701047259978_141\\1701047259978_141_0_1701047320961.ts";
    //std::string filePath = "E:\\res\\test-wav\\0.wav";
    //std::string filePath = "E:\\res\\HOYO-MiX-DaCapo.flac";
    std::string filePath5 = "E:\\res\\output.flac";
    std::string filePath1 = "E:\\res\\test-wav\\dvrStorage1231\\media\\edulyse-edge-windows\\1703591015324_6\\1703591015324_6_0_13348064620365.ts";
    std::string filePath2 = R"(E:\res\test-wav\test.mp4)";
    for (size_t i = 0; i < 10; i++)
    {
        AudioDecode(filePath2, nullptr, false);
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
    std::string   filePath = "E:/res/test-wav/student.mp4";
    std::string   destDir  = "E:/res/test-wav/student";
    std::string   scpPath  = "E:/res/test-wav/student/wav.scp";
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
