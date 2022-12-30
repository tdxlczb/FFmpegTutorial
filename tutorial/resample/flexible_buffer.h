#pragma once

#include <cstdint>

namespace libav
{
namespace utils
{
class FlexibleBuffer
{
public:
    explicit FlexibleBuffer();

    ~FlexibleBuffer();

    uint8_t* Buffer();

    uint32_t RealSize();

    uint32_t UseSize();

    //只增大不减小
    bool FlexBuffer(const uint32_t& bufLen);

    //强制设定为指定大小
    bool ResetBuffer(const uint32_t& bufLen);

    //弹出前len长度的数据，后面数据前移
    bool PopData(const uint32_t len);

private:
    uint32_t _useLen = 0;
    uint32_t _realBufLen = 0;
    uint8_t* _buf = nullptr;
};
}
}
