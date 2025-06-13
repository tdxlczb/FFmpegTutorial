#include "examples.h"
#include <iostream>
#include <fstream>
#include <logger/logger.h>

//#include "E:\code\demo\FFmpegTutorial\public\third_party\libyuv\include\libyuv.h"
//#pragma comment(lib, "E:/code/demo/FFmpegTutorial/public/third_party/libyuv/lib/yuv.lib")

static uint8_t* g_buffer = nullptr;

#define PRINT_FUNC_ERROR(FUNC, ERROR) LOG_ERROR << ""#FUNC" error: " << ERROR << ", " << av_error_string(ERROR) << std::endl;

std::string av_error_string(int errnum) {
    char buf[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(errnum, buf, sizeof(buf));
    return std::string(buf);
}

// 函数：将视频转换为图片序列 ，只对mp4做了测试，其他格式未测试。
bool VideoToImages(const std::string& filePath, const std::string& outputFolder)
{
    LOG_INFO << "start decode:" << filePath << std::endl;
    avformat_network_init();

    // 打开文件
    AVFormatContext* formatContext = nullptr;
    int ret = avformat_open_input(&formatContext, filePath.c_str(), nullptr, nullptr);
    if (AVERROR(ret))
    {
        PRINT_FUNC_ERROR(avformat_open_input, ret);
        // 处理打开视频失败的情况
        avformat_network_deinit();
        return false;
    }

    // 获取流信息
    ret = avformat_find_stream_info(formatContext, nullptr);
    if (ret < 0)
    {
        PRINT_FUNC_ERROR(avformat_find_stream_info, ret);
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
        LOG_ERROR << "find stream failed" << std::endl;
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

    ret = avcodec_parameters_to_context(codecContext, codecParameters);
    if (AVERROR(ret))
    {
        PRINT_FUNC_ERROR(avcodec_parameters_to_context, ret);
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        avformat_network_deinit();
        return false;
    }

    codecContext->thread_count = 2;
    ret = avcodec_open2(codecContext, codec, nullptr);
    if (ret != 0)
    {
        PRINT_FUNC_ERROR(avcodec_open2, ret);
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        avformat_network_deinit();
        return false;
    }

    if (codecContext->width <= 0 || codecContext->height <= 0 || codecContext->pix_fmt == AV_PIX_FMT_NONE)
    {
        LOG_ERROR << "codecContext data error" << std::endl;
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
    auto      dstFormat  = AV_PIX_FMT_BGR24;

    LOG_INFO << "fps=" << (int) fps << ", frameCount=" << frameCount << std::endl;

    SwsContext* sws_ctx = sws_getContext(
        codecContext->width, codecContext->height, codecContext->pix_fmt, codecContext->width, codecContext->height, AV_PIX_FMT_BGR24, SWS_BILINEAR,
        NULL, NULL, NULL
    );
    //使用sws_getContext会出现内存泄漏，使用sws_getCachedContext代替
    //SwsContext* sws_ctx = sws_getCachedContext(
    //    NULL, codecContext->width, codecContext->height, codecContext->pix_fmt, codecContext->width, codecContext->height, dstFormat,
    //    SWS_BILINEAR, NULL, NULL, NULL
    //);
    if (!sws_ctx)
    {
        LOG_ERROR << "sws_getCachedContext failed" << std::endl;
        avcodec_free_context(&codecContext);
        avformat_close_input(&formatContext);
        avformat_network_deinit();
        return false;
    }
    // 创建RGB视频帧
    AVFrame* frameRGB = av_frame_alloc();
    int      numBytes = av_image_get_buffer_size(dstFormat, codecContext->width, codecContext->height, AV_INPUT_BUFFER_PADDING_SIZE);
    uint8_t* buffer   = (uint8_t*) av_malloc(numBytes * sizeof(uint8_t)); //注意，这里给frameRGB申请的buffer，需要单独释放
    av_image_fill_arrays(
        frameRGB->data, frameRGB->linesize, buffer, dstFormat, codecContext->width, codecContext->height, AV_INPUT_BUFFER_PADDING_SIZE
    );

    //上面创建RGB视频帧的操作，可以用av_image_alloc替代
    //av_image_alloc(frameRGB->data, frameRGB->linesize, codecContext->width, codecContext->height, destFormat, AV_INPUT_BUFFER_PADDING_SIZE);

    // 读取视频帧并保存为图片
    int       frameIndex  = -1;
    int       packetIndex = -1;
    AVPacket* packet      = av_packet_alloc();
    AVFrame*  frame       = av_frame_alloc();

    while (true)
    {
        av_packet_unref(packet);
        int ret = av_read_frame(formatContext, packet);
        if (ret < 0)
        {
            PRINT_FUNC_ERROR(av_read_frame, ret);
            break;
        }
        if (packet->stream_index == videoStreamIndex)
        {
            packetIndex++;
            {
                auto duration = std::chrono::system_clock::now().time_since_epoch();
                auto ts       = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
                LOG_INFO << "packetIndex:" << packetIndex << ", packetFlags:" << packet->flags << ", ts:" << ts
                         << std::endl;
            }
            //if (packet.flags != AV_PKT_FLAG_KEY)
            //{
            //    continue;
            //}

            ret = avcodec_send_packet(codecContext, packet);
            av_packet_unref(packet);
            if (ret < 0)
            {
                PRINT_FUNC_ERROR(avcodec_send_packet, ret);
                // 处理发送数据包到解码器失败的情况
                continue;
            }
            while (true)
            {
                ret = avcodec_receive_frame(codecContext, frame);
                if (ret < 0) 
                {
                    PRINT_FUNC_ERROR(avcodec_receive_frame, ret);
                    break;
                }
                frameIndex++;
                {
                    auto duration = std::chrono::system_clock::now().time_since_epoch();
                    auto ts       = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
                    LOG_INFO << "packetIndex:" << packetIndex << ", frameIndex:" << frameIndex
                             << ", keyFrame:" << frame->key_frame << ", ts:" << ts << std::endl;
                }
                //if (frameIndex % 5 != 0)
                //{
                //    av_frame_unref(frame);
                //    continue;
                //}

                //int ret = libyuv::I420ToRGBA(
                //    frame->data[0], frame->linesize[0], frame->data[1], frame->linesize[1], frame->data[2], frame->linesize[2], frameRGB->data[0],
                //    frameRGB->linesize[0], codecContext->width, codecContext->height
                //);
                //delete[] g_buffer;
                //g_buffer                            = nullptr;
                //g_buffer                            = new uint8_t[frame->width * frame->width];
                //const uint8_t* const inData[3]      = {frame->data[0], frame->data[1], frame->data[2]};
                //int                  inLinesize[3]  = {frame->width, frame->width / 2, frame->width / 2};
                //auto                 buf            = std::make_unique<std::uint8_t[]>(frame->width * frame->height * 4); // rgba
                //int                  outLinesize[1] = {frame->width * 4};
                //auto*                rgb            = buf.get();
                //int                  ret            = sws_scale(sws_ctx, inData, inLinesize, 0, frame->height, &rgb, outLinesize);

                int ret = sws_scale(sws_ctx, frame->data, frame->linesize, 0, codecContext->height, frameRGB->data, frameRGB->linesize);

                // 保存图片
                //char filename[100];
                //sprintf(filename, "%s/%d.jpg", outputFolder.c_str(), frameIndex);
                //cv::Mat mat = cv::Mat(codecContext->height, codecContext->width, CV_8UC3, frameRGB->data[0], frameRGB->linesize[0]);
                //cv::imwrite(filename, mat);

                {
                    auto duration = std::chrono::system_clock::now().time_since_epoch();
                    auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
                    LOG_INFO << "packetIndex:" << packetIndex << ", frameIndex:" << frameIndex << ", convert rgba" << ", ts:" << ts << std::endl;
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
    sws_freeContext(sws_ctx);
    avcodec_free_context(&codecContext);
    avformat_close_input(&formatContext);
    avformat_network_deinit();

    LOG_INFO << "end decode:" << filePath << std::endl;
    return true;
}

bool H265TranscodeH264(const std::string& inputPath, const std::string& outputPath)
{
    LOG_INFO << "input path：" << inputPath << std::endl;
    avformat_network_init();

    // 打开视频文件
    AVFormatContext* formatContext = nullptr;
    if (avformat_open_input(&formatContext, inputPath.c_str(), nullptr, nullptr) != 0)
    {
        LOG_ERROR << "avformat_open_input failed" << std::endl;
        // 处理打开视频失败的情况
        avformat_network_deinit();
        return false;
    }

    // 获取视频流信息
    if (avformat_find_stream_info(formatContext, nullptr) < 0)
    {
        LOG_ERROR << "avformat_find_stream_info failed" << std::endl;
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
        LOG_ERROR << "find stream failed" << std::endl;
        // 处理未找到视频流的情况
        avformat_close_input(&formatContext);
        avformat_network_deinit();
        return false;
    }

    // 获取视频流解码器参数
    AVCodecParameters* codecParameters = formatContext->streams[videoStreamIndex]->codecpar;
    // 获取视频解码器
    const AVCodec*     decoder         = avcodec_find_decoder(codecParameters->codec_id);
    if (!decoder)
    {
        LOG_ERROR << "avcodec_find_decoder failed" << std::endl;
        avformat_close_input(&formatContext);
        avformat_network_deinit();
        return false;
    }
    // 创建解码器上下文
    AVCodecContext* decoderContext = avcodec_alloc_context3(decoder);
    if (avcodec_parameters_to_context(decoderContext, codecParameters) < 0)
    {
        LOG_ERROR << "avcodec_parameters_to_context failed" << std::endl;
        avcodec_free_context(&decoderContext);
        avformat_close_input(&formatContext);
        avformat_network_deinit();
        return false;
    }

    decoderContext->thread_count = 2;
    if (avcodec_open2(decoderContext, decoder, nullptr) != 0)
    {
        LOG_ERROR << "avcodec_open2 failed decoder" << std::endl;
        avcodec_free_context(&decoderContext);
        avformat_close_input(&formatContext);
        avformat_network_deinit();
        return false;
    }

    if (decoderContext->width <= 0 || decoderContext->height <= 0 || decoderContext->pix_fmt == AV_PIX_FMT_NONE)
    {
        LOG_ERROR << "decoderContext data error" << std::endl;
        avcodec_free_context(&decoderContext);
        avformat_close_input(&formatContext);
        avformat_network_deinit();
        return false;
    }

    // 创建编码器上下文
    const AVCodec*  encoder        = avcodec_find_encoder(AV_CODEC_ID_H264);
    AVCodecContext* encoderContext = avcodec_alloc_context3(encoder);
    encoderContext->width          = codecParameters->width;
    encoderContext->height         = codecParameters->height;
    //encoderContext->pix_fmt        = AV_PIX_FMT_YUV420P;
    //encoderContext->time_base      = {1, 25};
    //encoderContext->framerate      = {25, 1};
    encoderContext->pix_fmt        = decoderContext->pix_fmt;
    encoderContext->time_base      = decoderContext->time_base;
    encoderContext->framerate      = decoderContext->framerate;

    // 打开编码器
    if (avcodec_open2(encoderContext, encoder, nullptr) < 0)
    {
        LOG_ERROR << "avcodec_open2 failed encoder" << std::endl;
        avcodec_free_context(&decoderContext);
        avcodec_free_context(&encoderContext);
        avformat_close_input(&formatContext);
        avformat_network_deinit();
        return false;
    }

    // 分配编码帧内存
    AVFrame* encodeFrame = av_frame_alloc();
    encodeFrame->format  = encoderContext->pix_fmt;
    encodeFrame->width   = encoderContext->width;
    encodeFrame->height  = encoderContext->height;

    // 计算编码帧内存大小
    int      encodeFrameBufferSize = av_image_get_buffer_size(encoderContext->pix_fmt, encoderContext->width, encoderContext->height, 1);
    uint8_t* encodeFrameBuffer     = static_cast<uint8_t*>(av_malloc(encodeFrameBufferSize));

    // 关联编码帧内存和帧数据
    av_image_fill_arrays(
        encodeFrame->data, encodeFrame->linesize, encodeFrameBuffer, encoderContext->pix_fmt, encoderContext->width, encoderContext->height, 1
    );

    // 初始化编码器数据包
    AVPacket*     encodePacket = av_packet_alloc();
    //av_init_packet(encodePacket);
    // 创建输出文件
    std::ofstream outputFile(outputPath, std::ios::binary);

    SwsContext* sws_ctx = sws_getCachedContext(
        NULL, decoderContext->width, decoderContext->height, decoderContext->pix_fmt, encoderContext->width, encoderContext->height,
        encoderContext->pix_fmt, SWS_BILINEAR, NULL, NULL, NULL
    );
    if (!sws_ctx)
    {
        LOG_ERROR << "sws_getCachedContext failed" << std::endl;
        avcodec_free_context(&decoderContext);
        avcodec_free_context(&encoderContext);
        avformat_close_input(&formatContext);
        avformat_network_deinit();
        return false;
    }

    int       frameIndex  = -1;
    int       packetIndex = -1;
    AVPacket* packet      = av_packet_alloc();
    AVFrame*  frame       = av_frame_alloc();
    while (av_read_frame(formatContext, packet) >= 0)
    {
        if (packet->stream_index == videoStreamIndex)
        {
            packetIndex++;
            LOG_INFO << "packetIndex:" << packetIndex << ", packetFlags:" << packet->flags << std::endl;
            //if (packet.flags != AV_PKT_FLAG_KEY)
            //{
            //    continue;
            //}

            if (avcodec_send_packet(decoderContext, packet) < 0)
            {
                // 处理发送数据包到解码器失败的情况
                av_packet_unref(packet);
                break;
            }
            while (avcodec_receive_frame(decoderContext, frame) >= 0)
            {
                frameIndex++;
                LOG_INFO << "frameIndex:" << frameIndex << ", keyFrame:" << frame->key_frame << std::endl;
                //sws_scale(sws_ctx, frame->data, frame->linesize, 0, frame->height, encodeFrame->data, encodeFrame->linesize);
                // 编码帧
                if (avcodec_send_frame(encoderContext, frame) == 0)
                {
                    while (avcodec_receive_packet(encoderContext, encodePacket) == 0)
                    {
                        LOG_INFO << "packet size:" << encodePacket->size << std::endl;
                        // 写入输出文件
                        outputFile.write(reinterpret_cast<char*>(encodePacket->data), encodePacket->size);
                        // 释放编码包资源
                        av_packet_unref(encodePacket);
                    }
                }
                av_frame_unref(frame);
            }
            av_frame_unref(frame);
        }
        av_packet_unref(packet);
    }

    // 刷新编码器
    avcodec_send_frame(encoderContext, nullptr);
    while (avcodec_receive_packet(encoderContext, encodePacket) == 0)
    {
        // 写入输出文件
        outputFile.write(reinterpret_cast<char*>(encodePacket->data), encodePacket->size);
        // 释放编码包资源
        av_packet_unref(encodePacket);
    }

    // 关闭输出文件
    outputFile.close();

    // 释放资源
    av_packet_unref(packet);
    av_packet_free(&packet);
    av_packet_unref(encodePacket);
    av_packet_free(&encodePacket);
    av_frame_free(&frame);
    av_frame_free(&encodeFrame);
    av_free(encodeFrameBuffer);

    sws_freeContext(sws_ctx);
    avcodec_free_context(&decoderContext);
    avcodec_free_context(&encoderContext);
    avformat_close_input(&formatContext);
    avformat_network_deinit();
    return true;
}
