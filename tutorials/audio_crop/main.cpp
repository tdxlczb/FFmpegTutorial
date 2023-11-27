#include <iostream>
#include <string>
#include <cstdint>
#include <cstdio>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}

int main()
{
    //std::string testWav = "E:\\res\\test_wav\\1701047259978_141\\1701047259978_141_0_1701047320961.ts";
    //std::string testWav = "E:\\res\\test_wav\\0.wav";
    //std::string testWav = "E:\\res\\HOYO-MiX-DaCapo.flac";
    std::string testWav = "E:\\res\\output.flac";

    avformat_network_init();

    AVFormatContext* formatContext = nullptr;
    if (avformat_open_input(&formatContext, testWav.c_str(), nullptr, nullptr) != 0)
    {
        // 处理打开文件失败的情况
        return -1;
    }

    if (avformat_find_stream_info(formatContext, nullptr) < 0)
    {
        // 处理无法找到流信息的情况
        return -1;
    }

    const AVCodec*     codec            = nullptr;
    AVCodecParameters* codecParameters  = nullptr;
    int                audioStreamIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, &codec, 0);
    if (audioStreamIndex < 0)
    {
        // 处理无法找到音频流的情况
        return -1;
    }

    codecParameters = formatContext->streams[audioStreamIndex]->codecpar;
    std::cout << "sample_fmt:" << av_get_sample_fmt_name((AVSampleFormat) codecParameters->format) << std::endl;
    std::cout << "sample_rate:" << codecParameters->sample_rate << std::endl;
    std::cout << "channels:" << codecParameters->channels << std::endl;
    AVCodecContext* codecContext = avcodec_alloc_context3(codec);
    if (avcodec_parameters_to_context(codecContext, codecParameters) < 0)
    {
        // 处理无法将参数设置到解码器上下文的情况
        return -1;
    }

    if (avcodec_open2(codecContext, codec, nullptr) < 0)
    {
        // 处理无法打开解码器的情况
        return -1;
    }

    const int      in_sample_rate     = codecContext->sample_rate;
    AVSampleFormat in_sfmt            = codecContext->sample_fmt;
    uint64_t       in_channel_layout  = codecContext->channel_layout;
    const int      out_sample_rate    = 16000;
    AVSampleFormat out_sfmt           = AV_SAMPLE_FMT_S16;
    uint64_t       out_channel_layout = AV_CH_LAYOUT_MONO;
    int            out_channels       = av_get_channel_layout_nb_channels(out_channel_layout);
    int            out_spb            = av_get_bytes_per_sample(out_sfmt);

    // 创建重采样上下文
    SwrContext* swrContext =
        swr_alloc_set_opts(nullptr, out_channel_layout, out_sfmt, out_sample_rate, in_channel_layout, in_sfmt, in_sample_rate, 0, NULL);
    if (!swrContext)
    {
        // 处理重采样上下文创建失败的情况
        return -1;
    }
    int ret = swr_init(swrContext);

    // 打开输出文件
    FILE* outputFile = fopen("E:\\res\\test_wav\\output.pcm", "wb");
    if (!outputFile)
    {
        // 处理无法打开输出文件的情况
        return -1;
    }
    FILE* outputS16File = fopen("E:\\res\\test_wav\\output_s16le_1_16000.pcm", "wb");
    if (!outputS16File)
    {
        // 处理无法打开输出文件的情况
        return -1;
    }
    int frameCnt = 0;

    AVPacket* packet = av_packet_alloc();
    AVFrame*  frame  = av_frame_alloc();
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
                // 重采样
                int out_nb_samples = av_rescale_rnd(
                    swr_get_delay(swrContext, frame->sample_rate) + frame->nb_samples, out_sample_rate, frame->sample_rate, AV_ROUND_UP
                );
                AVFrame* resampledFrame = av_frame_alloc();
                av_samples_alloc(resampledFrame->data, resampledFrame->linesize, out_channels, out_nb_samples, out_sfmt, 1);

                int out_samples = swr_convert(swrContext, resampledFrame->data, out_nb_samples, (const uint8_t**) frame->data, frame->nb_samples);
                printf("Succeed to convert frame %4d, samples [%d]->[%d]\n", frameCnt++, frame->nb_samples, out_samples);

                if (frameCnt >= 467)
                {
                    printf("last frames\n");
                }
                // 将重采样后的音频数据写入输出文件
                fwrite(resampledFrame->data[0], out_spb * out_channels, out_samples, outputS16File);

                av_frame_free(&resampledFrame);

                fwrite(frame->data[0], 1, frame->linesize[0], outputFile);
            }
        }

        av_packet_unref(packet);
    }

    AVFrame* cache_out_frame = av_frame_alloc();
    av_samples_alloc(cache_out_frame->data, cache_out_frame->linesize, out_channels, 1024, out_sfmt, 1);
    int cache_out_samples = swr_convert(swrContext, cache_out_frame->data, 1024, NULL, 0);
    fwrite(cache_out_frame->data[0], out_spb * out_channels, cache_out_samples, outputS16File);

    swr_free(&swrContext);
    av_packet_free(&packet);
    av_frame_free(&frame);
    avcodec_free_context(&codecContext);
    avformat_close_input(&formatContext);

    // 关闭输出文件
    fclose(outputFile);
    fclose(outputS16File);

    return 0;
}
