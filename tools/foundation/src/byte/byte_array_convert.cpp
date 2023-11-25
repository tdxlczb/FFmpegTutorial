#include "foundation/byte/byte_array_convert.h"

void src_int24_to_float_array(const int24_t* in, float* out, int len)
{
    for (int i = 0; i < len; i++)
    {
        out[i] = int(in[i]) / (1.0 * 0x800000);
    };
}

void src_float_to_int24_array(const float* in, int24_t* out, int len)
{
    for (int i = 0; i < len; i++)
    {
        float scaled_value;
        scaled_value = in[i] * 8388608.f;
        if (scaled_value >= 8388607.f)
            out[i] = 8388607;
        else if (scaled_value <= -8388608.f)
            out[i] = -8388608;
        else
            out[i] = (int24_t) (lrintf(scaled_value));
    }
}

void src_int24_to_int32_array(const int24_t* in, int32_t* out, int len)
{
    for (int i = 0; i < len; i++)
    {
        out[i] = (int32_t) (in[i]);
    };
}

void src_int32_to_int24_array(const int32_t* in, int24_t* out, int len)
{
    for (int i = 0; i < len; i++)
    {
        if (in[i] > INT24_MAX)
        {
            out[i] = INT24_MAX;
        }
        else if (in[i] < INT24_MIN)
        {
            out[i] = INT24_MIN;
        }
        else
        {
            out[i] = (int24_t) (in[i]);
        }
    }
}
