#include "flexible_buffer.h"

#include <memory>

libav::utils::FlexibleBuffer::FlexibleBuffer()
{
}

libav::utils::FlexibleBuffer::~FlexibleBuffer()
{
    if (_buf)
        std::free(_buf);
}

uint8_t * libav::utils::FlexibleBuffer::Buffer()
{
    return _buf;
}

uint32_t libav::utils::FlexibleBuffer::RealSize()
{
    return _realBufLen;
}

uint32_t libav::utils::FlexibleBuffer::UseSize()
{
    return _useLen;
}

bool libav::utils::FlexibleBuffer::FlexBuffer(const uint32_t & bufLen)
{
    if (bufLen > _realBufLen)
    {
        uint8_t* newBuf = (uint8_t*)std::realloc(_buf, bufLen);
        if (!newBuf)
        {
            return false;
        }

        _buf = newBuf;
        _realBufLen = bufLen;
    }

    _useLen = bufLen;
    return true;
}

bool libav::utils::FlexibleBuffer::ResetBuffer(const uint32_t & bufLen)
{
    if (bufLen < 1)
    {
        return false;
    }

    uint8_t* newBuf = (uint8_t*)std::realloc(_buf, bufLen);
    if (!newBuf)
    {
        return false;
    }

    _buf = newBuf;
    _realBufLen = bufLen;
    _useLen = bufLen;
    return false;
}

bool libav::utils::FlexibleBuffer::PopData(const uint32_t len)
{
    if (len > _realBufLen)
    {
        return false;
    }
    else if (len == _realBufLen)
    {
        return true;
    }
    else if (len == 0)
    {
        return true;
    }

    std::memmove(_buf, _buf + len, _realBufLen - len);
    _useLen = _realBufLen - len;
    return true;
}
