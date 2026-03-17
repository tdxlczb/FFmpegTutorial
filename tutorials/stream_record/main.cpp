//参考文档:https://www.cnblogs.com/zhijun1996/p/18628530

#include <iostream>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>
#include <thread>
#include "mp4_recorder.h"

// FFmpeg 5.x 头文件（无需额外注册组件）
extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/time.h>
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
}


int rtsp_save_to_file(const std::string& url, const std::string& filepath)
{
    // 错误日志
    char errMsg[1024] = { 0 };

    // 输入格式上下文
    AVFormatContext* av_inputCtx = avformat_alloc_context();
    // 打开输入源
    int ret = avformat_open_input(&av_inputCtx, url.c_str(), nullptr, nullptr);
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

    fprintf(stdout, "%s open %s success!", __func__, url);
    if (ret < 0) {
        return -1;
    }

    std::vector<AVCodecParameters*> codecpars;
    // 遍历输入流，拷贝输入流的编码参数到输出流
    for (int i = 0; i < av_inputCtx->nb_streams; i++) {

        codecpars.push_back(av_inputCtx->streams[i]->codecpar);
    }
    auto recorder = std::make_shared<MP4Recorder>();
    recorder->Init(codecpars, filepath);

    const int64_t max_duration = 20 * AV_TIME_BASE;
    const int64_t start_time = av_gettime();
    // 循环从输入流中读包，写入到输出流中
    while (true) {
        // 超时控制
        if (av_gettime() - start_time > max_duration) {
            fprintf(stdout, "%s reach max duration, stop pulling\n", __func__);
            break;
        }
        std::shared_ptr<AVPacket> packet(av_packet_alloc(), [](AVPacket* p) {av_packet_free(&p); });
        // 从输入流中获取一个编码后的包 PES包
        int ret = av_read_frame(av_inputCtx, packet.get());
        if (ret < 0) {
            av_strerror(ret, errMsg, sizeof(errMsg));
            fprintf(stderr, "%s avformat_write_header failed! errMsg:%s\n", __func__, errMsg);
            continue;
        }
        if (packet) {
            //packet->time_base = av_inputCtx->streams[packet->stream_index]->time_base;
            recorder->SaveOneFrame((AVMediaType)packet->stream_index, av_inputCtx->streams[packet->stream_index]->time_base, packet.get());
        }
    }

    // 关闭输入源
    avformat_close_input(&av_inputCtx);
    // 释放输入上下文

    if (av_inputCtx)
        avformat_free_context(av_inputCtx);

    recorder->DeInit();
}

int main(int argc, char* argv[])
{
    std::string url = "rtsp://admin:admin@123@172.16.25.11:554/c9/b1772640000/e1772726399/replay/s0/";
    std::string filepath = "E:/code/media/temp/dump.mp4";

    // 初始化FFmpeg的网络组件
    avformat_network_init();
    // 设置FFmpeg内部日志等级
    av_log_set_level(AV_LOG_INFO);


    std::vector<std::string> urls;
    urls.push_back("rtsp://admin:admin!123@172.16.45.35:554/media/chn/1?transportmode=unicast&profile=Profile_1");
    //urls.push_back("rtsp://admin:admin@123@172.16.45.172:554/c2/b1773302457/e1773302577/replay/s0/");
    //urls.push_back("rtsp://admin:admin@123@172.16.45.172:554/c3/b1773302457/e1773302577/replay/s0/");

    std::vector<std::thread> threads;
    
    for (size_t i = 0; i < urls.size(); i++)
    {
        std::string filepath = "E:/code/media/temp/dump" + std::to_string(i) + ".mp4";
        auto th = std::thread([=]() {
            rtsp_save_to_file(urls[i], filepath);
            });
        threads.push_back(std::move(th));
    }
    getchar();
    for (size_t i = 0; i < urls.size(); i++)
    {
        threads[i].join();
    }

    // 释放ffmpeg网络资源
    avformat_network_deinit();

    return 0;
}
