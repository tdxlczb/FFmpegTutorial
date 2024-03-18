#include <iostream>
#include <string>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <functional>
#include <logger/logger.h>
#include <foundation/file/file_utils.h>
#include "examples.h"

// 函数：将视频转换为图片序列 ，只对mp4做了测试，其他格式未测试。
bool VideoToImages2(const std::string& filePath, const std::string& outputFolder, int threadIndex, bool useRgba, int threadCount)
{
    std::cout << "start decode:" << filePath << std::endl;
    avformat_network_init();

    // 打开视频文件
    AVFormatContext* formatContext = nullptr;
    if (avformat_open_input(&formatContext, filePath.c_str(), nullptr, nullptr) != 0)
    {
        std::cout << "avformat_open_input failed" << std::endl;
        // 处理打开视频失败的情况
        avformat_network_deinit();
        return false;
    }

    // 获取视频流信息
    if (avformat_find_stream_info(formatContext, nullptr) < 0)
    {
        std::cout << "avformat_find_stream_info failed" << std::endl;
        // 处理获取视频流信息失败的情况
        avformat_close_input(&formatContext);
        avformat_network_deinit();
        return false;
    }

    // 寻找视频流
    int videoStreamIndex = -1;
    for (unsigned int i = 0; i < formatContext->nb_streams; ++i)
    {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoStreamIndex = i;
            break;
        }
    }

    if (videoStreamIndex < 0)
    {
        std::cout << "find stream failed" << std::endl;
        // 处理未找到视频流的情况
        avformat_close_input(&formatContext);
        avformat_network_deinit();
        return false;
    }

    // 获取视频流解码器参数
    AVCodecParameters* codecParameters = formatContext->streams[videoStreamIndex]->codecpar;
    // 获取视频解码器
    const AVCodec*     codec           = avcodec_find_decoder(codecParameters->codec_id);
    // 创建解码器上下文
    AVCodecContext*    codecContext    = avcodec_alloc_context3(codec);
    if (avcodec_parameters_to_context(codecContext, codecParameters) < 0)
    {
        std::cout << "avcodec_parameters_to_context failed" << std::endl;
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        avformat_network_deinit();
        return false;
    }

    codecContext->thread_count = threadCount;
    if (avcodec_open2(codecContext, codec, nullptr) != 0)
    {
        std::cout << "avcodec_open2 failed" << std::endl;
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        avformat_network_deinit();
        return false;
    }

    if (codecContext->width <= 0 || codecContext->height <= 0 || codecContext->pix_fmt == AV_PIX_FMT_NONE)
    {
        std::cout << "codecContext data error" << std::endl;
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        avformat_network_deinit();
        return false;
    }

    AVStream* videoStream = formatContext->streams[videoStreamIndex];
    auto      timeBase    = videoStream->time_base;
    // 获取FPS
    double    fps         = av_q2d(videoStream->avg_frame_rate);
    // 获取总的帧数量
    auto      frameCount  = videoStream->nb_frames;
    int       gopSize     = codecContext->gop_size;

    std::cout << "fps=" << (int) fps << ", frameCount=" << frameCount << std::endl;
#ifdef USE_SCALE
    //SwsContext* sws_ctx = sws_getContext(
    //    codecContext->width, codecContext->height, codecContext->pix_fmt, codecContext->width, codecContext->height, AV_PIX_FMT_BGR24, SWS_BILINEAR,
    //    NULL, NULL, NULL
    //);
    //使用sws_getContext会出现内存泄漏，使用sws_getCachedContext代替
    SwsContext* sws_ctx = sws_getCachedContext(
        NULL, codecContext->width, codecContext->height, codecContext->pix_fmt, codecContext->width, codecContext->height, AV_PIX_FMT_BGR24,
        SWS_BILINEAR, NULL, NULL, NULL
    );
    if (!sws_ctx)
    {
        std::cout << "sws_getCachedContext failed" << std::endl;
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        avformat_network_deinit();
        return false;
    }
    int  inLinesize[3]  = {codecContext->width, codecContext->width / 2, codecContext->width / 2};
    auto bufSize        = codecContext->width * codecContext->height * 4;
    // auto  buf            = std::make_unique<std::uint8_t[]>(codecContext->width * codecContext->height * 4); // rgba
    // auto* rgb            = buf.get();
    auto rgb            = new uint8_t[bufSize];
    int  outLinesize[1] = {codecContext->width * 4};

#endif
    // 创建RGB视频帧
    AVFrame* frameRGB = av_frame_alloc();

    int      numBytes = av_image_get_buffer_size(AV_PIX_FMT_BGR24, codecContext->width, codecContext->height, AV_INPUT_BUFFER_PADDING_SIZE);
    uint8_t* buffer   = (uint8_t*) av_malloc(numBytes * sizeof(uint8_t)); //注意，这里给frameRGB申请的buffer，需要单独释放
    av_image_fill_arrays(
        frameRGB->data, frameRGB->linesize, buffer, AV_PIX_FMT_BGR24, codecContext->width, codecContext->height, AV_INPUT_BUFFER_PADDING_SIZE
    );

    //av_image_alloc(frameRGB->data, frameRGB->linesize, codecContext->width, codecContext->height, AV_PIX_FMT_BGR24, AV_INPUT_BUFFER_PADDING_SIZE);

    // 读取视频帧并保存为图片
    int       frameIndex  = -1;
    int       packetIndex = -1;
    AVPacket* packet      = av_packet_alloc();
    AVFrame*  frame       = av_frame_alloc();
    while (av_read_frame(formatContext, packet) >= 0)
    {
        if (packet->stream_index == videoStreamIndex)
        {
            packetIndex++;
            {
                auto duration = std::chrono::system_clock::now().time_since_epoch();
                auto ts       = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
                std::cout << "threadIndex:" << threadIndex << ", packetIndex:" << packetIndex << ", packetFlags:" << packet->flags << ", ts:" << ts
                          << std::endl;
            }
            //if (packet.flags != AV_PKT_FLAG_KEY)
            //{
            //    continue;
            //}

            if (avcodec_send_packet(codecContext, packet) < 0)
            {
                // 处理发送数据包到解码器失败的情况
                av_packet_unref(packet);
                break;
            }
            while (avcodec_receive_frame(codecContext, frame) >= 0)
            {
                frameIndex++;
                {
                    auto duration = std::chrono::system_clock::now().time_since_epoch();
                    auto ts       = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
                    std::cout << "threadIndex:" << threadIndex << ", packetIndex:" << packetIndex << ", frameIndex:" << frameIndex
                              << ", keyFrame:" << frame->key_frame << ", ts:" << ts << std::endl;
                }
                // std::this_thread::sleep_for(std::chrono::milliseconds(25));
                if (useRgba)
                {
#ifdef USE_SCALE

                    memset(rgb, 0, bufSize);
                    const uint8_t* const inData[3] = {frame->data[0], frame->data[1], frame->data[2]};
                    // int                  inLinesize[3] = {frame->width, frame->width / 2, frame->width / 2};

                    // auto  buf            = std::make_unique<std::uint8_t[]>(frame->width * frame->height * 4); // rgba
                    // int   outLinesize[1] = {frame->width * 4};
                    // auto* rgb            = buf.get();
                    int ret = sws_scale(sws_ctx, inData, inLinesize, 0, frame->height, &rgb, outLinesize);
                    // int ret = sws_scale(sws_ctx, frame->data, frame->linesize, 0, codecContext->height, frameRGB->data, frameRGB->linesize);
                    std::cout << "sws_scale ret=" << ret << std::endl;
#endif
                    //// 保存图片
                    //char filename[100];
                    //sprintf(filename, "%s/%d.jpg", outputFolder.c_str(), frameIndex);
                    //cv::Mat mat = cv::Mat(codecContext->height, codecContext->width, CV_8UC3, frameRGB->data[0], frameRGB->linesize[0]);
                    //cv::imwrite(filename, mat);
                    {
                        auto duration = std::chrono::system_clock::now().time_since_epoch();
                        auto ts       = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
                        std::cout << "threadIndex:" << threadIndex << ", packetIndex:" << packetIndex << ", frameIndex:" << frameIndex
                                  << ", convert rgba"
                                  << ", ts:" << ts << std::endl;
                    }
                }
                av_frame_unref(frame);
            }
            av_frame_unref(frame);
        }
        av_packet_unref(packet);
    }
    av_packet_unref(packet);
    av_packet_free(&packet);
    av_frame_free(&frame);
    av_frame_free(&frameRGB);
    av_freep(&buffer); //释放av_malloc申请的buffer，需要单独释放
#ifdef USE_SCALE
    sws_freeContext(sws_ctx);
#endif
    avcodec_free_context(&codecContext);
    avformat_close_input(&formatContext);
    avformat_network_deinit();

    std::cout << "end decode:" << filePath << std::endl;
    return true;
}

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

int main2()
{
    InitLogger();
    LOG_INFO << "==================================";
    std::string output    = "E:\\res\\mca\\output";
    std::string filePath  = "E:\\res\\mca\\test.h265";
    std::string filePath2 = "E:\\res\\mca\\dump.264";
    HWDeviceDecode(filePath, output, "cuda", 4);
    getchar();
    //VideoToImages(filePath, output);
    //getchar();
    //H265TranscodeH264(filePath, filePath2);

    //multi_thread_test();
    //video_to_image();

    //getchar();
    return 0;
}

#include <iostream>
#include <exception>
#include <string>
using namespace std;

class MyException : public exception
{
public:
    MyException() : message("Error.") {}
    MyException(string str) : message("Error : " + str) {}
    ~MyException() throw() {}

    virtual const char* what() const throw() { return message.c_str(); }

private:
    string message;
};
void test()
{
    throw MyException();

    try
    {
        throw MyException();
    }
    catch (MyException& e)
    {
        std::cout << e.what() << std::flush;
    }
    catch (std::exception& e)
    {
        //其他的错误
    }
}
int main()
{
    try
    {
        test();
    }
    catch (MyException& e)
    {
        std::cout << e.what() << std::flush;
    }
    catch (std::exception& e)
    {
        //其他的错误
    }

    return 0;
}
