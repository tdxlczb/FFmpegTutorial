﻿#pragma once
#include <string>
#include <mutex>
#include <thread>
#include <queue>
#include <condition_variable>
#include <opencv2/opencv.hpp>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/error.h>
#include <libavutil/imgutils.h>
#include <libavutil/frame.h>
#include <libavutil/time.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

bool VideoToImages(const std::string& filePath, const std::string& outputFolder);