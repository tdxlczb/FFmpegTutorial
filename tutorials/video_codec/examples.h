#pragma once
#include <string>
#include <mutex>
#include <thread>
#include <queue>
#include <condition_variable>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/frame.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

bool VideoToImages(const std::string& filePath, const std::string& outputFolder);

bool H265TranscodeH264(const std::string& inputPath, const std::string& outputPath);

bool HWDeviceDecode(const std::string& filePath, const std::string& outputFolder, const std::string& deviceName, int threadCount);