#pragma once

#include <cstdint>

#include "libav_audio_resample.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include "libswresample/swresample.h"
#include "libavutil/opt.h"

#ifdef __cplusplus
}
#endif

namespace libav
{
namespace utils
{
class FlexibleBuffer;
}
}

class AudioResampleImpl
{
public:
    AudioResampleImpl();

    ~AudioResampleImpl();

    bool OpenResample(const libav::audio_resample::ResampleCfg& parameters);

    bool DoResample(const uint8_t* data, const uint32_t size, uint8_t*& outData, uint32_t& outSize, bool isend);

private:
    int    SetInFrameData(const uint8_t* data, const uint32_t size);
    size_t TakeConvertData(int inNbSamples);
    size_t TakeCacheConvertData(size_t usedBufferSize);
    size_t FillOutBuffer(const uint8_t* fillIn, const uint32_t fillInSize, size_t usedBufferSize);
private:
    libav::utils::FlexibleBuffer*      _inBuffer  = nullptr;
    libav::utils::FlexibleBuffer*      _outBuffer = nullptr;
    libav::audio_resample::ResampleCfg _config;
    SwrContext*                        _swrCtx = nullptr;
    int                                _inSampleRate;
    int                                _outSampleRate;
    int64_t                            _inChannelLayout;
    int64_t                            _outChannelLayout;
    int                                _inNbChannels;
    int                                _outNbChannels;
    AVSampleFormat                     _inSfmt;
    AVSampleFormat                     _outSfmt;
    int                                _inBytesPerSample;
    int                                _outBytesPerSample;
    AVFrame*                           _inFrame;
    AVFrame*                           _outFrame;
};
