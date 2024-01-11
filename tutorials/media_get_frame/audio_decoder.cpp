#include "audio_decoder.h"
#include <fstream>
#include <logger/logger.h>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}

/*
std::string filePath    = "E:\\res\\test-wav\\output.pcm";
FILE*       file        = fopen(filePath.c_str(), "rb");
int         readSamples = 16000 * 10;
int16_t*    readBuffer  = new int16_t[readSamples];
float*      inputBuffer = new float[readSamples];
while (true)
{
    memset(readBuffer, 0, readSamples);
    memset(inputBuffer, 0, readSamples);
    int readSize = fread(readBuffer, sizeof(int16_t), readSamples, file);
    if (readSize <= 0)
        break;
    for (size_t i = 0; i < readSize; i++)
    {
        inputBuffer[i] = (float) readBuffer[i];
    }
}
*/

bool AudioDecode(const std::string& filePath, const DecodeCallback& callback, bool isDebug)
{
    LOG_INFO << "start decode audio:" << filePath;
    avformat_network_init();

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
    int            audioStreamIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, &codec, 0);
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
    int            in_channels        = codecContext->ch_layout.nb_channels;
    const int      out_sample_rate    = 16000;
    AVSampleFormat out_sfmt           = AV_SAMPLE_FMT_S16;
    uint64_t       out_channel_layout = AV_CH_LAYOUT_MONO;
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

    int       frameIndex  = -1;
    int       packetIndex = -1;
    AVPacket* packet      = av_packet_alloc();
    AVFrame*  frame       = av_frame_alloc();
    while (av_read_frame(formatContext, packet) >= 0)
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