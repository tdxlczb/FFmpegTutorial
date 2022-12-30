#pragma once

#ifdef LIBAV_AUDIORESAMPLE_EXPORTS
#define AUDIO_RESAMPLE_API __declspec(dllexport)
#else
#define AUDIO_RESAMPLE_API __declspec(dllimport)
#endif

//#include <mlog/mlog_def.hpp>

namespace libav
{
//AUDIO_RESAMPLE_API void SetLibAvAudioResampleLogCallback(MLogCallBack logCallback);

namespace audio_resample
{
typedef unsigned long LibavAudioFormat;
//static const LibavAudioFormat AUDIO_SINT8 = 0x1;      // 8-bit signed integer.
static const LibavAudioFormat AUDIO_SINT16 = 0x2;       // 16-bit signed integer.
static const LibavAudioFormat AUDIO_SINT24 = 0x4;     // 24-bit signed integer.
static const LibavAudioFormat AUDIO_SINT32 = 0x8;       // 32-bit signed integer.
static const LibavAudioFormat AUDIO_FLOAT32 = 0x10;     // 32-bit Normalized between plus/minus 1.0.
//static const LibavAudioFormat AUDIO_FLOAT64 = 0x20;   // 64-bit Normalized between plus/minus 1.0.

struct ResampleCfg
{
    int srcChannel = 0;                                 //源通道数    1 / 2
    int srcSampleRate = 0;                              //源采样频率  xxx Hz
    LibavAudioFormat srcBitsPerSample = AUDIO_SINT16;   //源采样深度

    int dstChannel = 0;                                 //目标通道数   1 / 2
    int dstSampleRate = 0;                              //目标采样频率 xxx Hz
    LibavAudioFormat dstBitsPerSample = AUDIO_SINT16;   //目标采样深度

    int quality = 1;                                    //质量 0 - BEST_QUALITY 1 - MEDIUM_QUALITY  2 - FASTEST
};
}
}
