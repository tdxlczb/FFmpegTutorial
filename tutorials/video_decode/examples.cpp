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

static AVBufferRef* hw_device_ctx = NULL;
static enum AVPixelFormat hw_pix_fmt;

static enum AVPixelFormat get_hw_format(AVCodecContext* ctx, const enum AVPixelFormat* pix_fmts)
{
    const enum AVPixelFormat* p;

    for (p = pix_fmts; *p != -1; p++) {
        if (*p == hw_pix_fmt)
            return *p;
    }

    fprintf(stderr, "Failed to get HW surface format.\n");
    return AV_PIX_FMT_NONE;
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
    std::string hwdevice_name = "dxva2";
    AVHWDeviceType hwDeviceType = AV_HWDEVICE_TYPE_NONE;
    if (!hwdevice_name.empty()) {
        hwDeviceType = av_hwdevice_find_type_by_name(hwdevice_name.c_str());
        if (hwDeviceType == AV_HWDEVICE_TYPE_NONE) {
            LOG_ERROR << "device type is not supported:" << hwdevice_name << std::endl;
            return false;
        }
        //查找硬解码器
        for (int i = 0;; ++i) {
            const AVCodecHWConfig* codecHWConfig = avcodec_get_hw_config(codec, i);
            if (!codecHWConfig) {
                //没有硬解码器了
                //LOG_ERROR << QString("decoder %1 is not support device type:%2").arg(codec->name).arg(QString(av_hwdevice_get_type_name(m_hwDeviceType)));
                break;
            }
            if (codecHWConfig->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX &&
                codecHWConfig->device_type == hwDeviceType) {
                //找到了指定的解码器，记录对应的AVPixelFormat，把硬件支持的像素格式设置进去，后面get_format需要使用;
                hw_pix_fmt = codecHWConfig->pix_fmt;
                break;
            }
        }
        //如果上下文已经有了硬解码，那么将其取消引用，准备重新创建
        if (codecContext->hw_device_ctx) {
            av_buffer_unref(&codecContext->hw_device_ctx);
            codecContext->hw_device_ctx = nullptr;
        }
        // 创建硬解码器
        ret = av_hwdevice_ctx_create(&codecContext->hw_device_ctx, hwDeviceType, nullptr, nullptr, 0);
        if (AVERROR(ret)) {
            //qDebug() << "av_hwdevice_ctx_create failed," << QString("%1:%2").arg(ret).arg(av_error_qstring(ret));
            hwDeviceType = AV_HWDEVICE_TYPE_NONE;
        }
        codecContext->get_format = get_hw_format;
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
    SwsContext* sws_ctx = nullptr;
    //SwsContext* sws_ctx = sws_getContext(
    //    codecContext->width, codecContext->height, AV_PIX_FMT_NV12, codecContext->width, codecContext->height, dstFormat, SWS_BILINEAR,
    //    NULL, NULL, NULL
    //);
    //使用sws_getContext会出现内存泄漏，使用sws_getCachedContext代替
    //SwsContext* sws_ctx = sws_getCachedContext(
    //    NULL, codecContext->width, codecContext->height, codecContext->pix_fmt, codecContext->width, codecContext->height, dstFormat,
    //    SWS_BILINEAR, NULL, NULL, NULL
    //);
    //if (!sws_ctx)
    //{
    //    LOG_ERROR << "sws_getCachedContext failed" << std::endl;
    //    avcodec_free_context(&codecContext);
    //    avformat_close_input(&formatContext);
    //    avformat_network_deinit();
    //    return false;
    //}
    // 创建RGB视频帧
    //AVFrame* frameRGB = av_frame_alloc();
    //int      numBytes = av_image_get_buffer_size(dstFormat, codecContext->width, codecContext->height, AV_INPUT_BUFFER_PADDING_SIZE);
    //uint8_t* buffer   = (uint8_t*) av_malloc(numBytes * sizeof(uint8_t)); //注意，这里给frameRGB申请的buffer，需要单独释放
    //av_image_fill_arrays(
    //    frameRGB->data, frameRGB->linesize, buffer, dstFormat, codecContext->width, codecContext->height, AV_INPUT_BUFFER_PADDING_SIZE
    //);

    //上面创建RGB视频帧的操作，可以用av_image_alloc替代
    //av_image_alloc(frameRGB->data, frameRGB->linesize, codecContext->width, codecContext->height, destFormat, AV_INPUT_BUFFER_PADDING_SIZE);

    // 读取视频帧并保存为图片
    int       frameIndex  = -1;
    int       packetIndex = -1;
    AVPacket* packet      = av_packet_alloc();
    //AVFrame*  frame       = av_frame_alloc();
    //AVFrame*  hwframe     = av_frame_alloc();
    //AVFrame*  tempframe   = av_frame_alloc();
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
                AVFrame* frame = nullptr;
                AVFrame* hwframe = av_frame_alloc();
                AVFrame* tempframe = av_frame_alloc();
                //av_frame_unref(hwframe);
                //av_frame_unref(tempframe);
                ret = avcodec_receive_frame(codecContext, tempframe);
                if (ret < 0) 
                {
                    PRINT_FUNC_ERROR(avcodec_receive_frame, ret);
                    av_frame_free(&hwframe);
                    av_frame_free(&tempframe);
                    break;
                }
                if (tempframe->format == hw_pix_fmt) {
                    /* retrieve data from GPU to CPU */
                    if ((ret = av_hwframe_transfer_data(hwframe, tempframe, 0)) < 0) {
                        av_frame_free(&hwframe);
                        av_frame_free(&tempframe);
                        break;
                    }
                    frame = hwframe;
                }
                else {
                    frame = tempframe;
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

                if (!sws_ctx) {
                    sws_ctx = sws_getContext(frame->width, frame->height, (AVPixelFormat)frame->format, frame->width, frame->height, dstFormat, SWS_BILINEAR, NULL, NULL, NULL);
                }
                // 创建RGB视频帧
                AVFrame* frameRGB = av_frame_alloc();
                int numBytes = av_image_get_buffer_size(dstFormat, frame->width, frame->height, AV_INPUT_BUFFER_PADDING_SIZE);
                uint8_t* bufferRGB = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t)); // 注意，这里给frameRGB申请的buffer，需要单独释放
                av_image_fill_arrays(frameRGB->data, frameRGB->linesize, bufferRGB, dstFormat, frame->width, frame->height, AV_INPUT_BUFFER_PADDING_SIZE);
                int ret = sws_scale(sws_ctx, frame->data, frame->linesize, 0, frame->height, frameRGB->data, frameRGB->linesize);
                if (ret)
                {
                    // 保存图片
                    //char filename[100];
                    //sprintf(filename, "%s/%d.jpg", outputFolder.c_str(), frameIndex);
                    //cv::Mat mat = cv::Mat(frame->height, frame->width, CV_8UC3, frameRGB->data[0], frameRGB->linesize[0]);
                    //cv::imwrite(filename, mat);
                }
                {
                    auto duration = std::chrono::system_clock::now().time_since_epoch();
                    auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
                    LOG_INFO << "packetIndex:" << packetIndex << ", frameIndex:" << frameIndex << ", convert rgba" << ", ts:" << ts << std::endl;
                }

                av_frame_free(&hwframe);
                av_frame_free(&tempframe);
                av_frame_free(&frameRGB);
                av_freep(&bufferRGB);
            }
        }
    }
    av_packet_free(&packet);
    //av_frame_free(&frame);
    //av_frame_free(&hwframe);
    //av_frame_free(&frameRGB);
    //av_freep(&buffer); //释放av_malloc申请的buffer，需要单独释放
    sws_freeContext(sws_ctx);
    avcodec_free_context(&codecContext);
    avformat_close_input(&formatContext);
    avformat_network_deinit();

    LOG_INFO << "end decode:" << filePath << std::endl;
    return true;
}
