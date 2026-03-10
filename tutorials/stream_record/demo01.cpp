//参考文档:https://www.cnblogs.com/zhijun1996/p/18628530

#include <iostream>
#include <cstdio>
#include <cstring>
#include <memory>

// FFmpeg 5.x 头文件（无需额外注册组件）
extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/time.h>
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
}

char errMsg[256];
AVFormatContext* av_inputCtx;
AVFormatContext* av_outputCtx;

// 初始化ffmpeg
int ffmpeg_init(void)
{
    // 初始化FFmpeg的网络组件
    avformat_network_init();
    // 设置FFmpeg内部日志等级
    av_log_set_level(AV_LOG_INFO);

    return 0;
}

//打开输入源
int open_input(std::string iurl)
{
    // 错误日志
    char errMsg[1024] = { 0 };

    // 输入格式上下文
    av_inputCtx = avformat_alloc_context();
    // 打开输入源
    int ret = avformat_open_input(&av_inputCtx, iurl.c_str(), nullptr, nullptr);
    if (ret < 0) {
        av_strerror(ret, errMsg, sizeof(errMsg));
        fprintf(stderr, "%s errMsg:%s\n", __func__, errMsg);
        return -1;
    }

    // 从输入源中查找流
    ret = avformat_find_stream_info(av_inputCtx, nullptr);
    if (ret < 0)
    {
        // 关闭输入源
        avformat_close_input(&av_inputCtx);
        // 释放输入上下文
        avformat_free_context(av_inputCtx);

        av_strerror(ret, errMsg, sizeof(errMsg));
        fprintf(stderr, "%s errMsg:%s\n", __func__, errMsg);
        return -2;
    }

    fprintf(stdout, "%s open %s success!", __func__, iurl);

    return 0;
}

// 创建输出流
int open_output(std::string ourl)
{
    // 错误日志
    char errMsg[1024] = { 0 };

    // 输出上下文，格式："mpegts"(ts)，"matroska"(mkv)，"mov"，"avi"，"mp4"
    int ret = avformat_alloc_output_context2(&av_outputCtx, nullptr, "mov", ourl.c_str());
    if (ret < 0) {
        av_strerror(ret, errMsg, sizeof(errMsg));
        fprintf(stderr, "%s avformat_alloc_output_context2 failed! errMsg:%s\n", __func__, errMsg);
        return -1;
    }

    // 创建输出流,写入文件调用avio_open2
    ret = avio_open2(&av_outputCtx->pb, ourl.c_str(), AVIO_FLAG_READ_WRITE, nullptr, nullptr);
    if (ret < 0) {
        av_strerror(ret, errMsg, sizeof(errMsg));
        fprintf(stderr, "%s avio_open2 failed! errMsg:%s\n", __func__, errMsg);
        goto ERR1;
    }

    // 遍历输入流，拷贝输入流的编码参数到输出流
    for (int i = 0; i < av_inputCtx->nb_streams; i++) {

        // 创建输出流
        AVStream* out_stream = avformat_new_stream(av_outputCtx, nullptr);
        // 拷贝输入流的编码参数到输出流
        ret = avcodec_parameters_copy(out_stream->codecpar, av_inputCtx->streams[i]->codecpar);
        if (ret < 0) {
            av_strerror(ret, errMsg, sizeof(errMsg));
            fprintf(stderr, "%s avcodec_parameters_copy failed! errMsg:%s\n", __func__, errMsg);
            goto ERR2;
        }

    }

    // 初始化媒体文件的头部信息
    ret = avformat_write_header(av_outputCtx, nullptr);
    if (ret < 0) {
        av_strerror(ret, errMsg, sizeof(errMsg));
        fprintf(stderr, "%s avformat_write_header failed! errMsg:%s\n", __func__, errMsg);
        goto ERR2;
    }

    fprintf(stdout, "%s open %s success!", __func__, ourl.c_str());

    return 0;

ERR2:
    if (av_outputCtx) {
        avio_closep(&av_outputCtx->pb);
    }

ERR1:
    avformat_free_context(av_outputCtx);

    return ret;
}

//从输入流中读取一个包
std::shared_ptr<AVPacket> read_packet_from_source()
{
    // av packet, std::shared_ptr
    std::shared_ptr<AVPacket> packet(av_packet_alloc(), [](AVPacket* p) {av_packet_free(&p); });
    if (!packet) {
        fprintf(stderr, "%s av_packet_alloc failed!", __func__);
        return nullptr;
    }

    // 从输入流中获取一个编码后的包 PES包
    int ret = av_read_frame(av_inputCtx, packet.get());
    if (ret < 0) {
        av_strerror(ret, errMsg, sizeof(errMsg));
        fprintf(stderr, "%s avformat_write_header failed! errMsg:%s\n", __func__, errMsg);
        return nullptr;
    }

    return packet;
}

// 向输出流中写包
int write_packet_to_target(std::shared_ptr<AVPacket> packet)
{
    auto inputStream = av_inputCtx->streams[packet->stream_index];
    auto outputStream = av_outputCtx->streams[packet->stream_index];
    int stream_index = packet->stream_index;
    int64_t pts = packet->pts;
    // 时间基转换
    av_packet_rescale_ts(packet.get(), inputStream->time_base, outputStream->time_base);
    if (stream_index == 0)
        fprintf(stdout, "%s stream %d write packet pts:%ld, rescale pts:%ld\n", __func__, stream_index, pts, packet->pts);
    int ret = av_interleaved_write_frame(av_outputCtx, packet.get());
    if (ret < 0) {
        av_strerror(ret, errMsg, sizeof(errMsg));
        fprintf(stderr, "%s stream %d av_interleaved_write_frame failed! errMsg:%s\n", __func__, stream_index, errMsg);
    }
    return ret;
}

// 获取RTSP网络流，保存到文件;也可以获取其它协议的网络流，如RTMP,SRT
int rtsp_save_to_file(void)
{
    std::string url = "rtsp://admin:admin@123@172.16.25.11:554/c9/b1772726400/e1772726430/replay/s0/";
    std::string filepath = "E:/code/media/temp/dump.mp4";
    // 初始化
    ffmpeg_init();
    // 打开输入流
    int ret = open_input(url);
    if (ret < 0) {
        return -1;
    }

    // 打开输出流
    ret = open_output(filepath);

    const int64_t max_duration = 30 * AV_TIME_BASE;
    const int64_t start_time = av_gettime();
    // 循环从输入流中读包，写入到输出流中
    while (true) {
        // 超时控制
        if (av_gettime() - start_time > max_duration) {
            fprintf(stdout, "%s reach max duration, stop pulling\n", __func__);
            break;
        }
        auto packet = read_packet_from_source();
        if (packet) {
            write_packet_to_target(packet);
        }
    }

    //写入文件尾，不使用该函数可能会导致最后写入的一些packet无法读取
    av_write_trailer(av_outputCtx);
    return 0;
}

// 释放资源
void release(void)
{
    // 关闭输入源
    avformat_close_input(&av_inputCtx);
    // 释放输入上下文

    if (av_inputCtx)
        avformat_free_context(av_inputCtx);

    // 关闭输出源
    if (av_outputCtx->pb)
        avio_close(av_outputCtx->pb);
    // 释放输出格式上下文
    if (av_outputCtx)
        avformat_free_context(av_outputCtx);

    // 释放ffmpeg网络资源
    avformat_network_deinit();
}

int main01(int argc, char* argv[]) {
    
    rtsp_save_to_file();
    release();
    return 0;
}