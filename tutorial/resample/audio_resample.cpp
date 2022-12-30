#include "audio_resample.h"
#include "audio_resample_impl.h"

libav::audio_resample::AudioResample::AudioResample()
{
    _impl = new AudioResampleImpl();
}

libav::audio_resample::AudioResample::~AudioResample()
{
    delete _impl;
}

bool libav::audio_resample::AudioResample::OpenResample(const ResampleCfg & parameters)
{
    return _impl->OpenResample(parameters);
}

bool libav::audio_resample::AudioResample::DoResample(
    const uint8_t * data, const uint32_t size,
    uint8_t *& outData, uint32_t & outSize, bool isend /* = false*/
)
{
    if (!data)
    {
        return false;
    }

    if (size == 0)
    {
        return false;
    }

    return _impl->DoResample(data, size, outData, outSize, isend);
}
