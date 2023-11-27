#include <stdio.h>
#include <Windows.h>
#include <iostream>
#include <concurrent_queue.h>
#ifdef __cplusplus
extern "C"
{
#endif

#include "libswresample/swresample.h"
#include "libavutil/opt.h"

#ifdef __cplusplus
}
#endif

#include "audio_resample.h"

static void audio_resample_test()
{
    //输入文件和参数
    FILE* in_file = fopen("E:/res/pcm_s32le_1_48000_3.pcm", "rb");

    // 输出文件和参数
    FILE* out_file = fopen("E:/res/out12311.pcm", "wb");

    libav::audio_resample::AudioResample resample;

    libav::audio_resample::ResampleCfg cfg;
    //cfg.srcBitsPerSample = libav::audio_resample::AUDIO_SINT24;
    cfg.srcBitsPerSample = libav::audio_resample::AUDIO_SINT32;
    cfg.srcChannel       = 1;
    cfg.srcSampleRate    = 48000;
    //cfg.dstBitsPerSample = libav::audio_resample::AUDIO_SINT24;
    cfg.dstBitsPerSample = libav::audio_resample::AUDIO_SINT16;
    cfg.dstChannel       = 1;
    cfg.dstSampleRate    = 16000;

    resample.OpenResample(cfg);

    uint32_t      bufferSize = 4096; //cfg.srcSampleRate * cfg.srcChannel * 3;
    uint8_t*      buffer     = new uint8_t[bufferSize];
    LARGE_INTEGER t1;
    LARGE_INTEGER t2;
    for (;;)
    {
        QueryPerformanceCounter(&t1);
        bool   isend    = false;
        size_t readSize = fread(buffer, 1, bufferSize, in_file);
        if (readSize == 0)
        {
            break;
        }
        else if (readSize < bufferSize)
        {
            isend = true;
        }

        uint8_t* outdata;
        uint32_t outsize;
        resample.DoResample(buffer, readSize, outdata, outsize, isend);

        if (outsize > 0)
        {
            fwrite(outdata, 1, outsize, out_file);
        }
        QueryPerformanceCounter(&t2);
        std::cout << "dt =" << t2.QuadPart - t1.QuadPart << std::endl;
    }

    fclose(in_file);
    fclose(out_file);
}

static void resample_test()
{
    //输入文件和参数
    FILE*          in_file           = fopen("E:/res/pcm_s32le_2_48000_1.pcm", "rb");
    const int      in_sample_rate    = 48000;
    AVSampleFormat in_sfmt           = AV_SAMPLE_FMT_S32; // 输入数据交错存放，非plannar
    uint64_t       in_channel_layout = AV_CH_LAYOUT_STEREO;
    int            in_channels       = av_get_channel_layout_nb_channels(in_channel_layout);
    const int      in_nb_samples     = 1024;

    int in_spb = av_get_bytes_per_sample(in_sfmt);

    // 输出文件和参数
    FILE*          out_file           = fopen("E:/res/out123.pcm", "wb");
    const int      out_sample_rate    = 16000;
    AVSampleFormat out_sfmt           = AV_SAMPLE_FMT_S16;
    uint64_t       out_channel_layout = AV_CH_LAYOUT_MONO;
    int            out_channels       = av_get_channel_layout_nb_channels(out_channel_layout);
    int            out_nb_samples     = av_rescale_rnd(in_nb_samples, out_sample_rate, in_sample_rate, AV_ROUND_UP);

    int out_spb = av_get_bytes_per_sample(out_sfmt);

    //使用AVFrame分配缓存音频pcm数据，用于转换
    AVFrame* in_frame = av_frame_alloc();
    av_samples_alloc(in_frame->data, in_frame->linesize, in_channels, in_nb_samples, in_sfmt, 1);

    AVFrame* out_frame = av_frame_alloc();
    av_samples_alloc(out_frame->data, out_frame->linesize, out_channels, out_nb_samples, out_sfmt, 1);

    // swr上下文
    //SwrContext *swr_ctx = swr_alloc();
    //av_opt_set_channel_layout(swr_ctx, "in_channel_layout", in_channel_layout, 0);
    //av_opt_set_channel_layout(swr_ctx, "out_channel_layout", out_channel_layout, 0);
    //av_opt_set_int(swr_ctx, "in_sample_rate", in_sample_rate, 0);
    //av_opt_set_int(swr_ctx, "out_sample_rate", out_sample_rate, 0);
    //av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", in_sfmt, 0);
    //av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", out_sfmt, 0);
    //swr_init(swr_ctx);

    SwrContext* swr_ctx = NULL;
    swr_ctx = swr_alloc_set_opts(swr_ctx, out_channel_layout, out_sfmt, out_sample_rate, in_channel_layout, in_sfmt, in_sample_rate, 0, NULL);
    swr_init(swr_ctx);

    //修改参数
    //av_opt_set_int(swr_ctx, "in_sample_rate", in_sample_rate, 0);
    //swr_init(swr_ctx);

    // 用于读取的缓冲数据
    //int buf_len = in_spb * in_channels * in_nb_samples;
    //void* buf = malloc(buf_len);

    // 转换保存
    int           frameCnt = 0;
    LARGE_INTEGER t1;
    LARGE_INTEGER t2;
    while (1)
    {
        // read samples
        QueryPerformanceCounter(&t1);
        int read_samples = fread(in_frame->data[0], in_spb * in_channels, in_nb_samples, in_file);
        if (read_samples <= 0)
            break;
        if (read_samples < in_nb_samples)
        {
            int a = 0;
        }
        // convert prepare
        int dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx, in_sample_rate) + in_nb_samples, out_sample_rate, in_sample_rate, AV_ROUND_UP);

        if (dst_nb_samples > out_nb_samples)
        {
            av_frame_unref(out_frame);

            out_nb_samples = dst_nb_samples;

            av_samples_alloc(out_frame->data, out_frame->linesize, out_channels, out_nb_samples, out_sfmt, 1);
        }

        // convert
        int out_samples = swr_convert(swr_ctx, out_frame->data, out_nb_samples, (const uint8_t**) in_frame->data, read_samples);

        // write
        if (av_sample_fmt_is_planar(out_sfmt))
        { // plannar
            for (int i = 0; i < out_samples; i++)
            {
                for (int c = 0; c < out_channels; c++)
                    fwrite(out_frame->data[c] + i * out_spb, 1, out_spb, out_file);
            }
        }
        else
        { // packed
            fwrite(out_frame->data[0], out_spb * out_channels, out_samples, out_file);
        }
        QueryPerformanceCounter(&t2);
        std::cout << "dt =" << t2.QuadPart - t1.QuadPart << std::endl;
        printf("Succeed to convert frame %4d, samples [%d]->[%d]\n", frameCnt++, read_samples, out_samples);
    }

    // flush swr
    printf("Flush samples \n");
    int out_samples;
    do
    {
        // convert
        out_samples = swr_convert(swr_ctx, out_frame->data, out_nb_samples, NULL, 0);

        // write
        if (av_sample_fmt_is_planar(out_sfmt))
        {
            for (int i = 0; i < out_samples; i++)
            {
                for (int c = 0; c < out_channels; c++)
                    fwrite(out_frame->data[c] + i * out_spb, 1, out_spb, out_file);
            }
        }
        else
        {
            fwrite(out_frame->data[0], out_spb * out_channels, out_samples, out_file);
        }

        printf("Succeed to convert frame %d samples %d\n", frameCnt++, out_samples);
    }
    while (out_samples);

    // free
    av_frame_free(&in_frame);
    av_frame_free(&out_frame);

    swr_free(&swr_ctx);

    //free(buf);
    fclose(in_file);
    fclose(out_file);
}

int main()
{
    //audio_resample_test();
    resample_test();
    return 0;
}