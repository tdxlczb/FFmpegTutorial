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


int main()
{
    InitLogger();
    LOG_INFO << "==================================";
    std::string output    = R"(E:\code\media\temp)";
    std::string filePath  = R"(rtsp://172.16.47.126:554/rtp/34020000001320000012_34020000001320000001_2?token=AlGg10D7JK6oDNZP)";
    std::string filePath2 = R"(rtsp://172.16.19.40:554/rtp/34020000001110000001_34020000001320000001_4?token=HsVQhrh4RQ5U53am)";
    std::string filePath3 = R"(E:\code\media\temp\dump1.mp4)";
    //FindEncoders();
    CmdData data;
    std::thread th([&]() {
        VideoRecord(filePath2, filePath3, data);
        //VideoTranscode2(filePath2, filePath3, "", data);
        });
    getchar();
    data.isStop = true;
    th.join();
    return 0;
}
