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
    std::string output    = "E:\\res\\test-wav\\output";
    std::string filePath  = "E:\\res\\test-wav\\test.mp4";
    std::string filePath1 = "E:\\res\\test-wav\\dvrStorage1231\\media\\edulyse-edge-windows\\1703591015324_6\\1703591015324_6_0_13348064620365.ts";
    std::string filePath2 = "E:\\res\\test-wav\\1704177600496_4_0_13348651242807.ts";
    std::string filePath3 = "E:\\res\\test-wav\\1703762903540_2\\1703762903540_2_0_13348236794018.ts";
    std::string filePath4 =
        R"(E:\res\test-wav\EdulyseEdgeWindows\dvrStorage\media\edulyse-edge-windows\1704196735104_2\1704196735104_2_0_13348670347631.ts)";
    std::string filePath5 = R"(E:\res\test-wav\1704368128747_2\1704368128747_2_0_13348841750212.ts)";
    std::string filePath6 = R"(E:\res\test-wav\d6bda0290395c01e874326aa364426c3_SK_3999470_4003600.wav)";

    //VideoToImages(filePath5, output);
    //VideoToImages2(filePath1, output);
    //AudioDecode(filePath6, NULL, true);
    //getchar();

    std::string dir1 = R"(E:\res\test-wav\dvrStorage1231\media\edulyse-edge-windows\1703591015324_6)";
    std::string dir3 = R"(E:\res\test-wav\1703762903540_2)";
    std::string dir4 = R"(E:\res\test-wav\EdulyseEdgeWindows\dvrStorage\media\edulyse-edge-windows\1704196735104_2)";
    std::string dir5 = R"(E:\res\test-wav\1701047259978_141)";

    std::vector<std::string> fileList;
    foundation::FileUtils::GetFileList2(dir5, fileList, "*.ts");

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
