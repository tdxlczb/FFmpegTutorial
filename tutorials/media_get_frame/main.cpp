#include <iostream>
#include <string>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <functional>
#include <logger/logger.h>
#include <foundation/file/file_utils.h>

#include "video_decoder.h"
#include "audio_decoder.h"
#include "tools.h"

int main()
{
    //InitLogger();
    LOG_INFO << "==================================";
    //test();
    std::string output   = "E:\\res\\mca\\output";
    std::string filePath = "E:\\res\\mca\\test.mp4";

    //VideoToImages(filePath, output);
    //VideoToImages2(filePath, output);
    //AudioDecode(filePath6, NULL, true);
    //getchar();

    std::string dir  = R"(E:\res\mca\1703762903540_2)";
    std::string dir1 = R"(E:\res\mca\1701047259978_141)";

    std::vector<std::string> fileList;
    foundation::FileUtils::GetFileList2(dir1, fileList, "*.ts");

    int index = 0;
    for (size_t i = 0; i < fileList.size(); i++)
    {
        index++;
        auto startTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        //bool ret       = VideoToImages(fileList[i], output);
        bool ret       = AudioDecode(fileList[i], NULL, false);
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
