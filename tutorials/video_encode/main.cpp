#include <iostream>
#include <string>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <functional>
#include <thread>
#include <logger/logger.h>
#include <foundation/file/file_utils.h>
#include "examples.h"

const char* common_alaw_containers[] = {
    "wav",  // WAV格式
    "mov",  // QuickTime
    "mp4",  // MP4容器
    "ogg",  // Ogg容器
    "flac", // FLAC容器(虽然主要用于FLAC编码)
    "mkv",  // Matroska
    "avi",  // AVI
    "au",   // Sun AU格式
    nullptr
};

void test_alaw_support() {
    std::cout << "Testing common containers for PCM ALAW support:" << std::endl;

    for (int i = 0; common_alaw_containers[i] != nullptr; i++) {
        const char* fmt_name = common_alaw_containers[i];
        const AVOutputFormat* fmt = av_guess_format(fmt_name, nullptr, nullptr);

        if (fmt && avformat_query_codec(fmt, AV_CODEC_ID_PCM_ALAW, FF_COMPLIANCE_NORMAL) == 1) {
            std::cout << fmt_name << " - Supported" << std::endl;
        }
        else {
            std::cout << fmt_name << " - Not supported" << std::endl;
        }
    }
}

void test_h265_support() {
    std::cout << "Testing common containers for h265 support:" << std::endl;

    for (int i = 0; common_alaw_containers[i] != nullptr; i++) {
        const char* fmt_name = common_alaw_containers[i];
        const AVOutputFormat* fmt = av_guess_format(fmt_name, nullptr, nullptr);

        if (fmt && avformat_query_codec(fmt, AV_CODEC_ID_H265, FF_COMPLIANCE_NORMAL) == 1) {
            std::cout << fmt_name << " - Supported" << std::endl;
        }
        else {
            std::cout << fmt_name << " - Not supported" << std::endl;
        }
    }
}

int main()
{
    //test_h265_support();
    //test_alaw_support();
    //return 0;
    InitLogger();
    LOG_INFO << "==================================";
    std::string output    = R"(E:\code\media\temp)";
    std::string filePath  = R"(rtsp://admin:admin@123@172.16.25.11:554/c9/b1772726400/e1772727119/replay/s0/)";
    std::string filePath2 = R"(rtsp://172.16.19.40:554/rtp/34020000001110000001_34020000001320000001_4?token=HsVQhrh4RQ5U53am)";
    std::string filePath3 = R"(E:\code\media\temp\dump1.mp4)";
    std::string filePath4 = R"(rtsp://127.0.0.1/live/test)";
    //FindEncoders();
    CmdData data;
    std::thread th([&]() {
        //VideoRecord(filePath4, filePath3, data);
        VideoRecordNoEncode(filePath, filePath3, data);
        //VideoTranscode2(filePath2, filePath3, "", data);
        });
    getchar();
    data.isStop = true;
    th.join();
    return 0;
}
