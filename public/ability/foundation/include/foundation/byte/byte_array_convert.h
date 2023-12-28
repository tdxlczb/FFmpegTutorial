#pragma once
#include "int24_t.h"

void src_int24_to_float_array(const int24_t* in, float* out, int len);
void src_float_to_int24_array(const float* in, int24_t* out, int len);
void src_int24_to_int32_array(const int24_t* in, int32_t* out, int len);
void src_int32_to_int24_array(const int32_t* in, int24_t* out, int len);