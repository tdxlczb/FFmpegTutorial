#include <iostream>
#include <string>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <functional>
#include <logger/logger.h>
#include <foundation/file/file_utils.h>
#include "examples.h"

int multi_thread_test()
{
    std::string output   = "E:\\res\\mca\\output";
    std::string filePath = "E:\\res\\mca\\dump.265";

    int threadNum = 1;
    std::cin >> threadNum;
    std::vector<std::shared_ptr<std::thread>> threads;
    for (size_t i = 0; i < threadNum; i++)
    {
        auto th = std::make_shared<std::thread>(VideoToImages, filePath, output);
        threads.push_back(th);
    }
    getchar();
    for (size_t i = 0; i < threadNum; i++)
    {
        threads[i]->join();
    }
    return 0;
}

int video_to_image()
{
    std::string output   = "E:\\res\\mca\\output";
    std::string filePath = "E:\\res\\mca\\dump.265";

    std::string dir  = R"(E:\res\mca\1703762903540_2)";
    std::string dir1 = R"(E:\res\mca\1701047259978_141)";

    std::vector<std::string> fileList;
    foundation::FileUtils::GetFileList2(dir1, fileList, "*.ts");

    int index = 0;
    for (size_t i = 0; i < fileList.size(); i++)
    {
        index++;
        auto startTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        bool ret       = VideoToImages(fileList[i], output);
        if (!ret)
        {
            std::cout << "err index=" << index << std::endl;
        }
        auto endTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        std::cout << "duration=" << endTime - startTime << std::endl;
    }
    getchar();
    return 0;
}

int main()
{
    InitLogger();
    LOG_INFO << "==================================";
    std::string output    = R"(E:\code\media\temp)";
    std::string filePath  = R"(E:\code\media\BaiduSyncdisk.mp4)";
    std::string filePath2 = R"(E:\code\media\temp\dump.h264)";
    VideoToImages(filePath, output);
    //multi_thread_test();
    //video_to_image();

    getchar();
    return 0;
}
