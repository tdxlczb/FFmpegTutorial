#include <iostream>
#include <string>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <functional>
#include <logger/logger.h>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
//#include <libswresample/swresample.h>
}

#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>

// 函数：将视频转换为图片序列 ，只对mp4做了测试，其他格式未测试。
bool VideoToImages(const std::string& videoPath, const std::string& outputFolder)
{
    avformat_network_init();

    // 打开视频文件
    AVFormatContext* formatContext = nullptr;
    if (avformat_open_input(&formatContext, videoPath.c_str(), nullptr, nullptr) != 0)
    {
        // 处理打开视频失败的情况
        avformat_network_deinit();
        return false;
    }

    // 获取视频流信息
    if (avformat_find_stream_info(formatContext, nullptr) < 0)
    {
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

    if (videoStreamIndex == -1)
    {
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
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        avformat_network_deinit();
        return false;
    }

    codecContext->thread_count = 2;
    if (avcodec_open2(codecContext, codec, nullptr) != 0)
    {
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        avformat_network_deinit();
        return false;
    }

    if (codecContext->width <= 0 || codecContext->height <= 0 || codecContext->pix_fmt == AV_PIX_FMT_NONE)
    {
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

    //SwsContext* sws_ctx = sws_getContext(
    //    codecContext->width, codecContext->height, codecContext->pix_fmt, codecContext->width, codecContext->height, AV_PIX_FMT_BGR24, SWS_BILINEAR,
    //    NULL, NULL, NULL
    //);
    ////使用sws_getContext会出现内存泄漏，使用sws_getCachedContext代替
    SwsContext* sws_ctx = sws_getCachedContext(
        NULL, codecContext->width, codecContext->height, codecContext->pix_fmt, codecContext->width, codecContext->height, AV_PIX_FMT_BGR24,
        SWS_BILINEAR, NULL, NULL, NULL
    );

    // 创建视频帧
    AVFrame* frame    = av_frame_alloc();
    AVFrame* frameRGB = av_frame_alloc();

    int      numBytes = av_image_get_buffer_size(AV_PIX_FMT_BGR24, codecContext->width, codecContext->height, AV_INPUT_BUFFER_PADDING_SIZE);
    uint8_t* buffer   = (uint8_t*) av_malloc(numBytes * sizeof(uint8_t)); //注意，这里给frameRGB申请的buffer，需要单独释放
    av_image_fill_arrays(
        frameRGB->data, frameRGB->linesize, buffer, AV_PIX_FMT_BGR24, codecContext->width, codecContext->height, AV_INPUT_BUFFER_PADDING_SIZE
    );

    // 读取视频帧并保存为图片
    int       frameIndex  = -1;
    int       packetIndex = -1;
    AVPacket* packet      = av_packet_alloc();
    while (av_read_frame(formatContext, packet) >= 0)
    {
        if (packet->stream_index == videoStreamIndex)
        {
            packetIndex++;
            std::cout << "packetIndex:" << packetIndex << ", packetFlags:" << packet->flags << std::endl;
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
                std::cout << "frameIndex:" << frameIndex << ", keyFrame:" << frame->key_frame << std::endl;
                //if (frameIndex % 5 != 0)
                //{
                //    av_frame_unref(frame);
                //    continue;
                //}

                int  ret = sws_scale(sws_ctx, frame->data, frame->linesize, 0, codecContext->height, frameRGB->data, frameRGB->linesize);
                // 保存图片
                char filename[100];
                sprintf(filename, "%s/%d.jpg", outputFolder.c_str(), frameIndex);
                cv::Mat mat = cv::Mat(codecContext->height, codecContext->width, CV_8UC3, frameRGB->data[0], frameRGB->linesize[0]);
                cv::imwrite(filename, mat);

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

    sws_freeContext(sws_ctx);
    avcodec_free_context(&codecContext);
    avformat_close_input(&formatContext);
    avformat_network_deinit();
}

#include "video_decoder.h"
#include "tools.h"

void VideoToImages2(const std::string& videoPath, const std::string& outputFolder)
{
    auto reader = std::make_shared<VideoReader>(videoPath, 5, 3);

    auto fps        = reader->GetFPS();
    auto frameCount = reader->GetTotalFrameCount();
    std::cout << "fps=" << fps << ", frameCount=" << frameCount << std::endl;
    VideoFrame frame;
    int        frameIndex = -1;
    while (true)
    {
        bool isDecoding = reader->IsDecoding();
        bool hasFrame   = reader->GetFrame(frame);
        if (!isDecoding && !hasFrame)
            break;
        if (hasFrame)
        {
            frameIndex++;
            cv::Mat rgbFrame;
            cv::cvtColor(frame.mat, rgbFrame, CV_BGR2RGB);
            //保存图片
            std::string fileName = outputFolder + "\\" + std::to_string(frameIndex) + ".jpg";
            cv::imwrite(fileName.c_str(), frame.mat);
            std::cout << "frameIndex=" << frameIndex << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main()
{
    //test();
    std::string output    = "E:\\res\\test-wav\\output";
    std::string filePath  = "E:\\res\\test-wav\\test.mp4";
    std::string filePath1 = "E:\\res\\test-wav\\dvrStorage1231\\media\\edulyse-edge-windows\\1703591015324_6\\1703591015324_6_0_13348064620365.ts";
    std::string filePath2 = "E:\\res\\test-wav\\1704177600496_4_0_13348651242807.ts";
    std::string filePath3 = "E:\\res\\test-wav\\1703762903540_2\\1703762903540_2_0_13348236794018.ts";
    std::string filePath4 =
        R"(E:\res\test-wav\EdulyseEdgeWindows\dvrStorage\media\edulyse-edge-windows\1704196735104_2\1704196735104_2_0_13348670347631.ts)";
    std::string filePath5 = R"(E:\res\test-wav\1704368128747_2\1704368128747_2_0_13348841750212.ts)";
    //VideoToImages(filePath5, output);
    //VideoToImages2(filePath1, output);

    std::string dir1 = R"(E:\res\test-wav\dvrStorage1231\media\edulyse-edge-windows\1703591015324_6)";
    std::string dir3 = R"(E:\res\test-wav\1703762903540_2)";
    std::string dir4 = R"(E:\res\test-wav\EdulyseEdgeWindows\dvrStorage\media\edulyse-edge-windows\1704196735104_2)";
    std::string dir5 = R"(E:\res\test-wav\1704368128747_2)";

    std::vector<std::string> fileList;
    GetFileList(dir5, fileList, "*.ts");

    for (size_t i = 0; i < fileList.size(); i++)
    {
        auto startTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        VideoToImages2(fileList[i], output);
        auto endTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        std::cout << " duration=" << endTime - startTime << std::endl;
    }

    getchar();
    return 0;
}
