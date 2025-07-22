#include <iostream>
#include <string>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <functional>
#include <logger/logger.h>
#include <foundation/file/file_utils.h>
#include "examples.h"


int main()
{
    InitLogger();
    LOG_INFO << "==================================";
    std::string output    = R"(E:\code\media\temp)";
    std::string filePath  = R"(E:\code\media\BaiduSyncdisk.mp4)";
    std::string filePath2 = R"(E:\code\media\temp\dump.h265)";
    std::string filePath3 = R"(E:\code\media\temp\dump.h264)";
    //FindEncoders();
    VideoTranscode2(filePath, filePath3, "libx264");

    getchar();
    return 0;
}
