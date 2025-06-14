#include "examples.h"
#include <iostream>
#include <fstream>
#include <logger/logger.h>

#define PRINT_FUNC_ERROR(FUNC, ERROR) LOG_ERROR << ""#FUNC" error: " << ERROR << ", " << av_error_string(ERROR) << std::endl;

std::string av_error_string(int errnum) {
    char buf[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(errnum, buf, sizeof(buf));
    return std::string(buf);
}

bool VideoTranscode(const std::string& inputPath, const std::string& outputPath, const std::string& codecName)
{
    LOG_INFO << "input path：" << inputPath << std::endl;
    avformat_network_init();

    // 打开视频文件
    AVFormatContext* formatContext = nullptr;
    int ret = avformat_open_input(&formatContext, inputPath.c_str(), nullptr, nullptr);
    if (AVERROR(ret))
    {
        PRINT_FUNC_ERROR(avformat_open_input, ret);
        // 处理打开视频失败的情况
        avformat_network_deinit();
        return false;
    }

    // 获取视频流信息
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
    ret = avcodec_parameters_to_context(decoderContext, codecParameters);
    if (ret < 0)
    {
        PRINT_FUNC_ERROR(avcodec_parameters_to_context, ret);
        avcodec_free_context(&decoderContext);
        avformat_close_input(&formatContext);
        avformat_network_deinit();
        return false;
    }

    decoderContext->thread_count = 2;
    ret = avcodec_open2(decoderContext, decoder, nullptr);
    if (ret != 0)
    {
        PRINT_FUNC_ERROR("avcodec_open2 decoder", ret);
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
    const AVCodec*  encoder        = avcodec_find_encoder_by_name(codecName.c_str());
    //const AVCodec*  encoder        = avcodec_find_encoder(AV_CODEC_ID_HEVC);
    AVCodecContext* encoderContext = avcodec_alloc_context3(encoder);
    encoderContext->width          = decoderContext->width;
    encoderContext->height         = decoderContext->height;
    encoderContext->pix_fmt        = decoderContext->pix_fmt;
    //encoderContext->pix_fmt        = AV_PIX_FMT_YUV420P;
    encoderContext->time_base      = {1, 25};
    encoderContext->framerate      = {25, 1};
    encoderContext->bit_rate       = 400000;
    //encoderContext->time_base      = decoderContext->time_base;//decoderContext里的time_base、framerate等可能是异常的，不可使用
    //encoderContext->framerate      = decoderContext->framerate;

    // 打开编码器
    ret = avcodec_open2(encoderContext, encoder, nullptr);
    if (ret < 0)
    {
        PRINT_FUNC_ERROR("avcodec_open2 encoder", ret);
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
