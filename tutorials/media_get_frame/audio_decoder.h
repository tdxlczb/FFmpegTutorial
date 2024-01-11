#pragma once
#include <string>
#include <functional>

using DecodeCallback = std::function<void(uint8_t* data, uint64_t len, bool isEnd)>;

bool AudioDecode(const std::string& filePath, const DecodeCallback& callback, bool isDebug);