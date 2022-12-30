#include "audio_resample_impl.h"
#include <string>
#include "flexible_buffer.h"
#include "int24_t.h"
#include "array_convert.h"

#define INT24_CONVERT_INT32
//#define INT24_CONVERT_FLOAT

AudioResampleImpl::AudioResampleImpl()
{
    _inBuffer = new libav::utils::FlexibleBuffer();
    _outBuffer = new libav::utils::FlexibleBuffer();
}

AudioResampleImpl::~AudioResampleImpl()
{
    delete _inBuffer;
    delete _outBuffer;
}

AVSampleFormat GetAvFormat(libav::audio_resample::LibavAudioFormat format)
{
    AVSampleFormat avFormat = AV_SAMPLE_FMT_NONE;
    switch (format)
    {
    case libav::audio_resample::AUDIO_SINT16:
        avFormat = AV_SAMPLE_FMT_S16;
        break;
    case libav::audio_resample::AUDIO_SINT24:
#ifdef INT24_CONVERT_INT32
        avFormat = AV_SAMPLE_FMT_S32;
#elif defined INT24_CONVERT_FLOAT
        avFormat = AV_SAMPLE_FMT_FLT;
#endif 
        break;
    case libav::audio_resample::AUDIO_SINT32:
        avFormat = AV_SAMPLE_FMT_S32;
        break;
    case libav::audio_resample::AUDIO_FLOAT32:
        avFormat = AV_SAMPLE_FMT_FLT;
        break;
    default:
        break;
    }
    return avFormat;
}

std::string GetFFMPEGErrMsg(int err)
{
    static char ERR_MSG_BUFFER[1024];
    return av_make_error_string(ERR_MSG_BUFFER, 1024, err);
}

bool AudioResampleImpl::OpenResample(const libav::audio_resample::ResampleCfg& parameters)
{
    _config = parameters;
    _outSampleRate = parameters.dstSampleRate;
    _outChannelLayout = av_get_default_channel_layout(parameters.dstChannel);
    _outNbChannels = parameters.dstChannel;
    _outSfmt = GetAvFormat(parameters.dstBitsPerSample);
    _outBytesPerSample = av_get_bytes_per_sample(_outSfmt);
    _inSampleRate = parameters.srcSampleRate;
    _inChannelLayout = av_get_default_channel_layout(parameters.srcChannel);
    _inNbChannels = parameters.srcChannel;
    _inSfmt = GetAvFormat(parameters.srcBitsPerSample);
    _inBytesPerSample = av_get_bytes_per_sample(_inSfmt);
    _swrCtx = swr_alloc_set_opts(_swrCtx, _outChannelLayout, _outSfmt, _outSampleRate, _inChannelLayout, _inSfmt, _inSampleRate, 0, NULL);
    int err = swr_init(_swrCtx);
    if (err < 0)
    {
        auto errmsg = GetFFMPEGErrMsg(err);
        return false;
    }

    _inFrame = av_frame_alloc();
    _outFrame = av_frame_alloc();
    return true;
}

bool AudioResampleImpl::DoResample(const uint8_t* data, const  uint32_t inSize, uint8_t*& outData, uint32_t& outSize, bool isend)
{
    outData = nullptr;
    outSize = 0;

    int in_nb_samples = SetInFrameData(data, inSize);

    //获取输入in_nb_samples采样数后输出的最大采样数，用于设置缓冲区
    int max_out_samples = swr_get_out_samples(_swrCtx, in_nb_samples);
    int outBufferSize = max_out_samples * _outNbChannels * _outBytesPerSample;
    if (_config.dstBitsPerSample == libav::audio_resample::AUDIO_SINT24)
    {
        outBufferSize = outBufferSize * 3 / _outBytesPerSample;
    }
    _outBuffer->ResetBuffer(outBufferSize);
    memset(_outBuffer->Buffer(), 0, outBufferSize);

    auto usedOutBufferSize = TakeConvertData(in_nb_samples);
    if (isend)
    {
        usedOutBufferSize += TakeCacheConvertData(usedOutBufferSize);
    }
    outSize = usedOutBufferSize;
    outData = _outBuffer->Buffer();
    return true;
}

int AudioResampleImpl::SetInFrameData(const uint8_t* data, const uint32_t inSize)
{
    int in_nb_samples = 0;
    if (_config.srcBitsPerSample == libav::audio_resample::AUDIO_SINT24)
    {//24位数据转成32位数据，此时的_inBytesPerSample=4
        in_nb_samples = inSize / (_inNbChannels * 3);
        av_samples_alloc(_inFrame->data, _inFrame->linesize, _inNbChannels, in_nb_samples, _inSfmt, 1);

        auto bufferSize = inSize * _inBytesPerSample / sizeof(int24_t);
        _inBuffer->ResetBuffer(bufferSize);
#ifdef INT24_CONVERT_INT32
        src_int24_to_int32_array((int24_t*)(data), (int32_t*)_inBuffer->Buffer(), inSize / sizeof(int24_t));
#elif defined INT24_CONVERT_FLOAT
        src_int24_to_float_array((int24_t*)(data), (float*)_inBuffer->Buffer(), inSize / sizeof(int24_t));
#endif 
        _inFrame->data[0] = _inBuffer->Buffer();
    }
    else
    {
        //计算输入的采样数
        in_nb_samples = inSize / (_inNbChannels * _inBytesPerSample);
        av_samples_alloc(_inFrame->data, _inFrame->linesize, _inNbChannels, in_nb_samples, _inSfmt, 1);

        _inFrame->data[0] = (uint8_t*)data;
    }
    return in_nb_samples;
}

size_t AudioResampleImpl::TakeConvertData(int inNbSamples)
{
    int out_nb_samples = av_rescale_rnd(inNbSamples, _outSampleRate, _inSampleRate, AV_ROUND_UP);
    av_samples_alloc(_outFrame->data, _outFrame->linesize, _outNbChannels, out_nb_samples, _outSfmt, 1);

    int out_samples = swr_convert(_swrCtx, _outFrame->data, out_nb_samples, (const uint8_t**)_inFrame->data, inNbSamples);
    int convertOutSize = out_samples * _outNbChannels * _outBytesPerSample;
    return FillOutBuffer(_outFrame->data[0], convertOutSize, 0);
}

size_t AudioResampleImpl::TakeCacheConvertData(size_t usedBufferSize)
{
    int max_out_samples = swr_get_out_samples(_swrCtx, 0);
    av_frame_unref(_outFrame);
    av_samples_alloc(_outFrame->data, _outFrame->linesize, _outNbChannels, max_out_samples, _outSfmt, 1);

    int out_samples = swr_convert(_swrCtx, _outFrame->data, max_out_samples, NULL, 0);

    int convertOutSize = out_samples * _outNbChannels * _outBytesPerSample;
    return FillOutBuffer(_outFrame->data[0], convertOutSize, usedBufferSize);
}

size_t AudioResampleImpl::FillOutBuffer(const uint8_t* fillIn, const uint32_t fillInSize, size_t usedBufferSize)
{
    if (_config.dstBitsPerSample == libav::audio_resample::AUDIO_SINT24)
    {//输出的32位数据转成24位数据，此时的_outBytesPerSample=4

#ifdef INT24_CONVERT_INT32
        src_int32_to_int24_array((int32_t*)(_outFrame->data[0]), (int24_t*)(_outBuffer->Buffer() + usedBufferSize), fillInSize / sizeof(int32_t));
#elif defined INT24_CONVERT_FLOAT
        src_float_to_int24_array((float*)(fillIn), (int24_t*)(_outBuffer->Buffer() + usedBufferSize), fillInSize / sizeof(float));
#endif 
        return fillInSize * 3 / _outBytesPerSample;
    }
    else
    {
        memcpy(_outBuffer->Buffer() + usedBufferSize, fillIn, fillInSize);
        return fillInSize;
    }
}