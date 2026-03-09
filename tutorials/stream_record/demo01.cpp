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

    // 输出上下文, ts流
    int ret = avformat_alloc_output_context2(&av_outputCtx, nullptr, "mpegts", ourl.c_str());
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

    // 时间基转换
    av_packet_rescale_ts(packet.get(), inputStream->time_base, outputStream->time_base);
    return av_interleaved_write_frame(av_outputCtx, packet.get());
}

// 获取RTSP网络流，保存到文件;也可以获取其它协议的网络流，如RTMP,SRT
int rtsp_save_to_file(void)
{
    // 初始化
    ffmpeg_init();
    // 打开输入流
    int ret = open_input(std::string("rtsp://192.168.16.230/live/test"));
    if (ret < 0) {
        return -1;
    }

    // 打开输出流
    ret = open_output(std::string("/tmp/test.ts"));

    // 循环从输入流中读包，写入到输出流中
    while (true) {
        auto packet = read_packet_from_source();
        if (packet) {
            write_packet_to_target(packet);
            fprintf(stdout, "%s writePacket success", __func__);
        } else {
            fprintf(stderr, "%s writePacket failed!\n", __func__);
            return -1;
        }
    }

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

int main(int argc, char* argv[]) {
    
    rtsp_save_to_file();
    release();
    return 0;
}