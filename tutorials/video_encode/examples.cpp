#include "examples.h"
#include <iostream>
#include <iomanip>
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
    LOG_INFO << "start transcode：" << inputPath << std::endl;
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
    LOG_INFO << "end transcode：" << inputPath << std::endl;
    return true;
}

int VideoTranscode2(const std::string& inputPath, const std::string& outputPath, const std::string& codecName, CmdData& cmd)
{
    // 打开输入文件
    AVFormatContext* input_ctx = nullptr;
    
    //配置该流的ffmpeg设置
    AVDictionary* pOptDict = NULL;
    av_dict_set(&pOptDict, "stimeout", "5000000", 0);//适应延迟网络，设置5s的等待链接时间
    av_dict_set(&pOptDict, "timeout", "5000000", 0);//适应延迟网络，设置5s的等待链接时间
    av_dict_set(&pOptDict, "buffer_size", "8192000", 0);//控制解码器或编码器的内部缓冲区大小,配置8M缓冲以适应高分辨率视频
    av_dict_set(&pOptDict, "recv_buffer_size", "4096000", 0);     // 防止花屏, max 4M.:用于控制网络接收缓冲区大小，适用于高带宽或高延迟的网络环境
    av_dict_set(&pOptDict, "tune", "stillimage,fastdecode,zerolatency", 0);//优化静态图像编码,快速解码和低延时传输
    av_dict_set(&pOptDict, "rtsp_transport", "tcp", 0);//tcp拉流，尽量保证不丢包
    int ret = avformat_open_input(&input_ctx, inputPath.c_str(), nullptr, &pOptDict);
    av_dict_free(&pOptDict);
    pOptDict = nullptr;
    if (ret < 0) {
        std::cerr << "Could not open input file\n";
        return -1;
    }

    //if (avformat_open_input(&input_ctx, inputPath.c_str(), nullptr, nullptr) < 0) {
    //    std::cerr << "Could not open input file\n";
    //    return -1;
    //}

    // 打印输入信息
    av_dump_format(input_ctx, 0, inputPath.c_str(), 0);

    // 获取流信息
    if (avformat_find_stream_info(input_ctx, nullptr) < 0) {
        std::cerr << "Could not find stream info\n";
        return -1;
    }

    // 查找视频流
    int video_stream_idx = -1;
    AVCodecParameters* codec_params = nullptr;
    for (unsigned int i = 0; i < input_ctx->nb_streams; i++) {
        if (input_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_idx = i;
            codec_params = input_ctx->streams[i]->codecpar;
            break;
        }
    }

    if (video_stream_idx == -1) {
        std::cerr << "No video stream found\n";
        return -1;
    }
    AVStream* videoStream = input_ctx->streams[video_stream_idx];

    // 初始化解码器
    const AVCodec* decoder = avcodec_find_decoder(codec_params->codec_id);
    if (!decoder) {
        std::cerr << "Unsupported codec\n";
        return -1;
    }

    AVCodecContext* decoder_ctx = avcodec_alloc_context3(decoder);
    if (avcodec_parameters_to_context(decoder_ctx, codec_params) < 0) {
        std::cerr << "Failed to copy codec params\n";
        return -1;
    }

    if (avcodec_open2(decoder_ctx, decoder, nullptr) < 0) {
        std::cerr << "Failed to open decoder\n";
        return -1;
    }

    // 准备输出文件
    // 分配输出格式上下文
    AVFormatContext* output_ctx = nullptr;
    if (avformat_alloc_output_context2(&output_ctx, nullptr, nullptr, outputPath.c_str()) < 0) {
        std::cerr << "Failed to create output context\n";
        return -1;
    }

    // 创建视频流
    AVStream* output_stream = avformat_new_stream(output_ctx, nullptr);
    if (!output_stream) {
        std::cerr << "Failed to create output stream\n";
        return -1;
    }

    // 初始化编码器
    const AVCodec* encoder = avcodec_find_encoder(AV_CODEC_ID_HEVC);
    if (!encoder) {
        std::cerr << "H.264 encoder not found\n";
        return -1;
    }

    // 设置编码器上下文
    AVCodecContext* encoder_ctx = avcodec_alloc_context3(encoder);
    // 配置编码参数
    encoder_ctx->codec_id = AV_CODEC_ID_HEVC;
    encoder_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
    encoder_ctx->pix_fmt = AV_PIX_FMT_YUVJ420P;
    encoder_ctx->width = decoder_ctx->width;
    encoder_ctx->height = decoder_ctx->height;
    encoder_ctx->time_base = videoStream->time_base;
    encoder_ctx->framerate = { 25, 1 };
    encoder_ctx->gop_size = 10;
    encoder_ctx->max_b_frames = 1;
    encoder_ctx->bit_rate = 400000;

    // 启用 CRF 质量模式
    //encoder_ctx->bit_rate = 0;
    //encoder_ctx->flags |= AV_CODEC_FLAG_QSCALE;
    //encoder_ctx->global_quality = 23 * FF_QP2LAMBDA; // CRF=23
    //encoder_ctx->gop_size = 30;
    //encoder_ctx->max_b_frames = 2;

    // 打开编码器
    if (avcodec_open2(encoder_ctx, encoder, nullptr) < 0) {
        std::cerr << "Failed to open encoder\n";
        return -1;
    }

    // 将编码器参数复制到流
    if (avcodec_parameters_from_context(output_stream->codecpar, encoder_ctx) < 0) {
        std::cerr << "Failed to copy encoder params\n";
        return -1;
    }

    // 打开输出文件
    if (!(output_ctx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&output_ctx->pb, outputPath.c_str(), AVIO_FLAG_WRITE) < 0) {
            std::cerr << "Failed to open output file\n";
            return -1;
        }
    }

    // 写入文件头
    if (avformat_write_header(output_ctx, nullptr) < 0) {
        std::cerr << "Failed to write header\n";
        return -1;
    }

    // 解码→编码循环
    AVPacket* input_packet = av_packet_alloc();
    AVFrame* decoded_frame = av_frame_alloc();
    AVPacket* output_packet = av_packet_alloc();
    int firstPts = 0;
    int frameIndex = 0;

    SwsContext* sws_ctx = nullptr;

    // 创建RGB视频帧
    AVFrame* frameRGB = av_frame_alloc();
    av_image_alloc(frameRGB->data, frameRGB->linesize, decoder_ctx->width, decoder_ctx->height, AV_PIX_FMT_BGR24, AV_INPUT_BUFFER_PADDING_SIZE);

    AVFrame* frameWrite = av_frame_alloc();
    frameWrite->format = decoder_ctx->pix_fmt;
    frameWrite->width = decoder_ctx->width;
    frameWrite->height = decoder_ctx->height;
    if (av_frame_get_buffer(frameWrite, 32) < 0) {
        std::cerr << "Could not allocate frame data" << std::endl;
        return -1;
    }

    auto      timeBase = videoStream->time_base;
    // 获取FPS
    double    fps = av_q2d(videoStream->avg_frame_rate);
    // 获取总的帧数量
    auto      frameCount = videoStream->nb_frames;
    int       gopSize = decoder_ctx->gop_size;
    double    perFrameTime = av_q2d(videoStream->time_base);
    int       lastFramePts = 0;
    while (!cmd.isStop)
    { 
        int ret = av_read_frame(input_ctx, input_packet);
        if (ret < 0) {
            std::cerr << "av_read_frame error:" << ret << "\n";
            continue;
        }
        if (input_packet->stream_index == video_stream_idx) {
            std::cout << "packet pts:" << input_packet->pts << std::endl;
            // 解码 H.265
            if (avcodec_send_packet(decoder_ctx, input_packet) < 0) {
                std::cerr << "Error sending packet to decoder\n";
                break;
            }

            while (avcodec_receive_frame(decoder_ctx, decoded_frame) == 0) {
                {
                    if (!sws_ctx) {
                        sws_ctx = sws_getContext(decoder_ctx->width, decoder_ctx->height, decoder_ctx->pix_fmt, decoder_ctx->width, decoder_ctx->height, AV_PIX_FMT_BGR24, SWS_BILINEAR, NULL, NULL, NULL);
                    }
                    ret = sws_scale(sws_ctx, decoded_frame->data, decoded_frame->linesize, 0, decoder_ctx->height, frameRGB->data, frameRGB->linesize);
                    if (ret)
                    {
                        // 保存图片
                        char filename[100];
                        sprintf(filename, "E:\\code\\media\\temp\\%d.jpg", frameIndex);
                        cv::Mat mat = cv::Mat(decoder_ctx->height, decoder_ctx->width, CV_8UC3, frameRGB->data[0], frameRGB->linesize[0]);
                        cv::imwrite(filename, mat);
                    }
                }
                std::cout << "frame index:" << frameIndex << ", pts:" << decoded_frame->pts << std::endl;
                // 准备帧数据
                //if (av_frame_make_writable(frameWrite) < 0) {
                //    std::cerr << "Frame not writable" << std::endl;
                //    return -1;
                //}
                //av_image_copy(frameWrite->data, frameWrite->linesize,
                //    (const uint8_t**)decoded_frame->data, decoded_frame->linesize,
                //    (AVPixelFormat)decoded_frame->format, decoded_frame->width, decoded_frame->height);

                if (frameIndex == 0) {
                    if (decoded_frame->pts < 0) {
                        frameWrite->pts = 0;
                        decoded_frame->pts = 0;
                    }
                }
                frameIndex++;

                // 编码 H.264
                if (avcodec_send_frame(encoder_ctx, decoded_frame) < 0) {
                    std::cerr << "Error sending frame to encoder\n";
                    break;
                }

                while (ret >= 0) {
                    ret = avcodec_receive_packet(encoder_ctx, output_packet);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                        break;
                    }
                    else if (ret < 0) {
                        std::cerr << "Error during encoding" << std::endl;
                        break;
                    }

                    // 调整时间戳
                    av_packet_rescale_ts(output_packet, encoder_ctx->time_base, output_stream->time_base);
                    output_packet->stream_index = output_stream->index;

                    // 写入压缩帧
                    if (av_interleaved_write_frame(output_ctx, output_packet) < 0) {
                        std::cerr << "Error writing packet\n";
                        break;
                    }
                    av_packet_unref(output_packet);
                }
            }
        }
        av_packet_unref(input_packet);
    }

    // 8. 冲刷编码器缓冲区
    if (avcodec_send_frame(encoder_ctx, nullptr) == 0) {
        while (avcodec_receive_packet(encoder_ctx, output_packet) == 0) {
            av_packet_rescale_ts(output_packet, encoder_ctx->time_base, output_stream->time_base);
            output_packet->stream_index = output_stream->index;
            av_interleaved_write_frame(output_ctx, output_packet);
            av_packet_unref(output_packet);
        }
    }

    // 9. 清理资源
    av_write_trailer(output_ctx);
    avformat_close_input(&input_ctx);
    if (output_ctx && !(output_ctx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&output_ctx->pb);
    }
    avformat_free_context(output_ctx);
    avcodec_free_context(&decoder_ctx);
    avcodec_free_context(&encoder_ctx);
    av_frame_free(&decoded_frame);
    av_packet_free(&input_packet);
    av_packet_free(&output_packet);

    std::cout << "Transcoding completed successfully!\n";
    return 0;
}

void FindEncoders()
{
    std::cout << "Available encoders:\n";
    std::cout << "========================================\n";

    const AVCodec* codec = nullptr;
    void* iter = nullptr;

    // 遍历所有编解码器
    while ((codec = av_codec_iterate(&iter))) {
        if (!av_codec_is_encoder(codec)) {
            continue;  // 只关注编码器
        }

        //// 格式化打印
        //// 输出宽度10个字符，左对齐，不足补空格，输出3
        //std::cout << std::setw(10) << std::setfill(' ') << std::left << 2 << std::endl;
        //// 输出宽度14，右对齐，不足补0，输出10
        //std::cout << std::setw(14) << std::setfill('0') << std::right << 10 << std::endl;

        std::cout << "Name: " << codec->name << "\n";
        std::cout << "Long Name: " << codec->long_name << "\n";
        std::cout << "Type: ";

        switch (codec->type) {
        case AVMEDIA_TYPE_VIDEO:
            std::cout << "Video";
            break;
        case AVMEDIA_TYPE_AUDIO:
            std::cout << "Audio";
            break;
        case AVMEDIA_TYPE_SUBTITLE:
            std::cout << "Subtitle";
            break;
        default:
            std::cout << "Other";
        }

        std::cout << "\n";
        std::cout << "ID: " << codec->id << "\n";

        // 检查支持的像素格式（视频编码器）
        if (codec->type == AVMEDIA_TYPE_VIDEO && codec->pix_fmts) {
            std::cout << "Supported pixel formats: ";
            for (const enum AVPixelFormat* p = codec->pix_fmts; *p != AV_PIX_FMT_NONE; p++) {
                std::cout << av_get_pix_fmt_name(*p) << " ";
            }
            std::cout << "\n";
        }

        // 检查支持的采样格式（音频编码器）
        if (codec->type == AVMEDIA_TYPE_AUDIO && codec->sample_fmts) {
            std::cout << "Supported sample formats: ";
            for (const enum AVSampleFormat* p = codec->sample_fmts; *p != AV_SAMPLE_FMT_NONE; p++) {
                std::cout << av_get_sample_fmt_name(*p) << " ";
            }
            std::cout << "\n";
        }

        // 检查支持的采样率（音频编码器）
        if (codec->type == AVMEDIA_TYPE_AUDIO && codec->supported_samplerates) {
            std::cout << "Supported sample rates: ";
            for (const int* p = codec->supported_samplerates; *p != 0; p++) {
                std::cout << *p << "Hz ";
            }
            std::cout << "\n";
        }

        //// 检查支持的声道布局（音频编码器）
        //if (codec->type == AVMEDIA_TYPE_AUDIO && codec->channel_layouts) {
        //    std::cout << "Supported channel layouts: ";
        //    for (const uint64_t* p = codec->channel_layouts; *p != 0; p++) {
        //        char buf[256];
        //        av_get_channel_layout_string(buf, sizeof(buf), -1, *p);
        //        std::cout << buf << " ";
        //    }
        //    std::cout << "\n";
        //}

        std::cout << "----------------------------------------\n";
    }

    std::cout << "========================================\n";
    std::cout << "Available encoders end\n";
}