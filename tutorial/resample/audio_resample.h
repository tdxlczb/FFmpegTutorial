#pragma once

#include <cstdint>

#include "libav_audio_resample.h"

class AudioResampleImpl;

namespace libav
{
namespace audio_resample
{
class AUDIO_RESAMPLE_API AudioResample
{
public:
    AudioResample();

    ~AudioResample();

    bool OpenResample(const ResampleCfg& parameters);

    bool DoResample(const uint8_t* data, const  uint32_t size, uint8_t*& outData, uint32_t& outSize, bool isend = false);

private:
    AudioResampleImpl* _impl = nullptr;
};
}
}
