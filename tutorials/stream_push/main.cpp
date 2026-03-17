#include <iostream>
#include <cstdio>
#include <cstring>

// FFmpeg 5.x 头文件
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/time.h>
#include <libavutil/mathematics.h>
}

// 错误处理宏
#define ERR_EXIT(msg) { \
    char err_buf[AV_ERROR_MAX_STRING_SIZE] = {0}; \
    av_strerror(ret, err_buf, sizeof(err_buf)); \
    fprintf(stderr, "%s: %s (code: %d)\n", msg, err_buf, ret); \
    exit(1); \
}

// 流状态结构体（简化管理音视频流）
typedef struct {
    int video_stream_idx;  // 视频流索引（-1表示无）
    int audio_stream_idx;  // 音频流索引（-1表示无）
    AVStream* in_video_stream;  // 输入视频流
    AVStream* in_audio_stream;  // 输入音频流
    AVStream* out_video_stream; // 输出视频流
    AVStream* out_audio_stream; // 输出音频流
    int64_t video_pts;     // 视频累计时间戳
    int64_t audio_pts;     // 音频累计时间戳
} StreamState;

int main(int argc, char* argv[]) {
    //if (argc != 3) {
    //    fprintf(stderr, "Usage: %s <input_mp4_path> <rtsp_url>\n", argv[0]);
    //    fprintf(stderr, "Example: %s test.mp4 rtsp://127.0.0.1:554/stream/test\n", argv[0]);
    //    return -1;
    //}

    //const char* input_path = "rtsp://admin:admin@123@172.16.45.172:554/unicast/c1/s0/live";
    const char* input_path = "E:/code/media/BaiduSyncdisk.mp4";
    const char* rtsp_url = "rtsp://127.0.0.1:554/live/test";

    // ===================== 1. 初始化FFmpeg =====================
    avformat_network_init();

    AVFormatContext* in_fmt_ctx = nullptr, * out_fmt_ctx = nullptr;
    AVPacket* pkt = av_packet_alloc();

    StreamState stream_state = { -1, -1, nullptr, nullptr, nullptr, nullptr, 0, 0 };
    int ret = 0;

    // ===================== 2. 打开输入MP4文件并检测流类型 =====================
    ret = avformat_open_input(&in_fmt_ctx, input_path, nullptr, nullptr);
    if (ret < 0) ERR_EXIT("Failed to open input MP4 file");

    // 获取流信息
    ret = avformat_find_stream_info(in_fmt_ctx, nullptr);
    if (ret < 0) ERR_EXIT("Failed to get stream info");

    // 查找视频流
    stream_state.video_stream_idx = av_find_best_stream(in_fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    // 查找音频流
    stream_state.audio_stream_idx = av_find_best_stream(in_fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);

    // 打印流类型信息
    printf("=== Stream Detection ===\n");
    if (stream_state.video_stream_idx >= 0) {
        stream_state.in_video_stream = in_fmt_ctx->streams[stream_state.video_stream_idx];
        printf("Video stream found: codec=%s, width=%d, height=%d, fps=%.2f\n",
            avcodec_get_name(stream_state.in_video_stream->codecpar->codec_id),
            stream_state.in_video_stream->codecpar->width,
            stream_state.in_video_stream->codecpar->height,
            av_q2d(stream_state.in_video_stream->r_frame_rate));
    } else {
        printf("No video stream found\n");
    }

    if (stream_state.audio_stream_idx >= 0) {
        stream_state.in_audio_stream = in_fmt_ctx->streams[stream_state.audio_stream_idx];
        printf("Audio stream found: codec=%s, sample rate=%d, channels=%d\n",
            avcodec_get_name(stream_state.in_audio_stream->codecpar->codec_id),
            stream_state.in_audio_stream->codecpar->sample_rate,
            stream_state.in_audio_stream->codecpar->ch_layout.nb_channels);
    } else {
        printf("No audio stream found\n");
    }

    // 检查是否有有效流
    if (stream_state.video_stream_idx < 0 && stream_state.audio_stream_idx < 0) {
        fprintf(stderr, "Error: No video/audio stream found in input file\n");
        avformat_close_input(&in_fmt_ctx);
        return -1;
    }

    // ===================== 3. 创建RTSP输出上下文 =====================
    ret = avformat_alloc_output_context2(&out_fmt_ctx, nullptr, "rtsp", rtsp_url);
    if (ret < 0) ERR_EXIT("Failed to create RTSP output context");

    // 设置RTSP传输参数（TCP更稳定）
    AVDictionary* opts = nullptr;
    av_dict_set(&opts, "rtsp_transport", "tcp", 0);
    av_dict_set(&opts, "stimeout", "5000000", 0); // 5秒超时

    // ===================== 4. 创建输出流（视频+音频） =====================
    // 复制视频流参数
    if (stream_state.video_stream_idx >= 0) {
        stream_state.out_video_stream = avformat_new_stream(out_fmt_ctx, nullptr);
        if (!stream_state.out_video_stream) ERR_EXIT("Failed to create RTSP video stream");

        // 复制编码参数到输出流
        ret = avcodec_parameters_copy(stream_state.out_video_stream->codecpar,
            stream_state.in_video_stream->codecpar);
        if (ret < 0) ERR_EXIT("Failed to copy video codec parameters");

        // 配置时间基
        stream_state.out_video_stream->time_base = stream_state.in_video_stream->time_base;
        stream_state.out_video_stream->avg_frame_rate = stream_state.in_video_stream->avg_frame_rate;
        stream_state.out_video_stream->r_frame_rate = stream_state.in_video_stream->r_frame_rate;
    }

    // 复制音频流参数
    if (stream_state.audio_stream_idx >= 0) {
        stream_state.out_audio_stream = avformat_new_stream(out_fmt_ctx, nullptr);
        if (!stream_state.out_audio_stream) ERR_EXIT("Failed to create RTSP audio stream");

        // 复制编码参数到输出流
        ret = avcodec_parameters_copy(stream_state.out_audio_stream->codecpar,
            stream_state.in_audio_stream->codecpar);
        if (ret < 0) ERR_EXIT("Failed to copy audio codec parameters");

        // 配置时间基
        stream_state.out_audio_stream->time_base = stream_state.in_audio_stream->time_base;
    }

    // 打印RTSP推流信息
    av_dump_format(out_fmt_ctx, 0, rtsp_url, 1);

    // ===================== 5. 打开RTSP IO并写入头部 =====================
    if (!(out_fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open2(&out_fmt_ctx->pb, rtsp_url, AVIO_FLAG_WRITE, nullptr, &opts);
        if (ret < 0) ERR_EXIT("Failed to open RTSP IO");
    }
    av_dict_free(&opts);

    // 写入RTSP头部
    ret = avformat_write_header(out_fmt_ctx, nullptr);
    if (ret < 0) ERR_EXIT("Failed to write RTSP header");

    // ===================== 6. 循环读取数据包并推流 =====================
    int video_frame_count = 0, audio_frame_count = 0;
    int64_t start_time = av_gettime();
    int64_t step_time = av_gettime();

    while (av_read_frame(in_fmt_ctx, pkt) >= 0) {
        // 跳过非音视频流
        if (pkt->stream_index != stream_state.video_stream_idx &&
            pkt->stream_index != stream_state.audio_stream_idx) {
            av_packet_unref(pkt);
            continue;
        }

        // ===================== 适配音视频流参数 =====================
        if (pkt->stream_index == stream_state.video_stream_idx) {
            // 视频流：调整索引和时间戳
            pkt->stream_index = stream_state.out_video_stream->index;
            av_packet_rescale_ts(pkt,
                stream_state.in_video_stream->time_base,
                stream_state.out_video_stream->time_base);
            video_frame_count++;
        } else if (pkt->stream_index == stream_state.audio_stream_idx) {
            // 音频流：调整索引和时间戳
            pkt->stream_index = stream_state.out_audio_stream->index;
            av_packet_rescale_ts(pkt,
                stream_state.in_audio_stream->time_base,
                stream_state.out_audio_stream->time_base);
            audio_frame_count++;
        }
        int stream_index = pkt->stream_index;
        // ===================== 推流数据包 =====================
        ret = av_interleaved_write_frame(out_fmt_ctx, pkt);
        if (ret < 0) {
            fprintf(stderr, "Failed to push packet (video:%d, audio:%d): %d\n",
                video_frame_count, audio_frame_count, ret);
            av_packet_unref(pkt);
            continue;
        }
        av_packet_unref(pkt);
        
        // 打印推流进度（每100帧）
        int64_t now = av_gettime();
        if (now - step_time >= 1000000) {
            int64_t elapsed = av_gettime() - start_time;
            double video_fps = video_frame_count / (elapsed / 1000000.0);
            double audio_fps = audio_frame_count / (elapsed / 1000000.0);
            printf("Pushed - Video: %d, Audio: %d, Video FPS: %.2f, Audio FPS: %.2f\n",
                video_frame_count, audio_frame_count, video_fps, audio_fps);
            step_time = now;
        }

        // 控制推流速度（以视频帧率为基准，无视频则以音频采样率为准）
        // 这里可能会导致帧率不够，需要优化调整睡眠时间
        if (stream_state.video_stream_idx >= 0) {
            if (stream_index == stream_state.video_stream_idx) {
                AVRational fps = stream_state.in_video_stream->r_frame_rate;
                if (fps.num > 0 && fps.den > 0) {
                    int64_t frame_delay = (1000000 * fps.den) / fps.num;
                    av_usleep(frame_delay);
                }
                //av_usleep(30000);
            }
        } else if (stream_state.audio_stream_idx >= 0) {
            if (stream_index == stream_state.audio_stream_idx) {
                // 音频流速度控制（按采样率）
                int sample_rate = stream_state.in_audio_stream->codecpar->sample_rate;
                int64_t audio_delay = (1024 * 1000000) / sample_rate; // 每1024采样点延时
                av_usleep(audio_delay);
            }
        }
    }

    // ===================== 7. 收尾工作 =====================
    // 写入RTSP尾部
    av_write_trailer(out_fmt_ctx);

    // 打印最终统计
    printf("\n=== Push Finished ===\n");
    printf("Total Video Frames: %d\n", video_frame_count);
    printf("Total Audio Frames: %d\n", audio_frame_count);

    // 释放资源
    if (in_fmt_ctx) avformat_close_input(&in_fmt_ctx);
    if (out_fmt_ctx) {
        if (!(out_fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&out_fmt_ctx->pb);
        }
        avformat_free_context(out_fmt_ctx);
    }
    av_packet_free(&pkt);
    avformat_network_deinit();
    return 0;
}