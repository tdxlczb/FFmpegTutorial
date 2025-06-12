#pragma once
#include <string>
#include <mutex>
#include <thread>
#include <queue>
#include <condition_variable>
#include <opencv2/opencv.hpp>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/frame.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

bool VideoToImages(const std::string& filePath, const std::string& outputFolder, int threadIndex = 1, bool useRgba = true, int threadCount = 2);

bool H265TranscodeH264(const std::string& inputPath, const std::string& outputPath);