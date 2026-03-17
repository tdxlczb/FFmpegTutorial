#include <iostream>
#include <cstdio>
#include <cstring>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <stdexcept>

// FFmpeg 5.x 头文件
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavutil/time.h>
}

// 线程安全队列（解耦核心）
template <typename T>
class SafeQueue {
public:
    SafeQueue(int max_size = 30) : max_size_(max_size), is_running_(true) {}

    ~SafeQueue() {
        stop();
    }

    // 入队（满则阻塞）
    bool push(const T& data) {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_not_full_.wait(lock, [this]() {
            return !is_running_ || queue_.size() < max_size_;
            });

        if (!is_running_) return false;

        queue_.push(data);
        cv_not_empty_.notify_one();
        return true;
    }

    // 出队（空则阻塞）
    bool pop(T& data) {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_not_empty_.wait(lock, [this]() {
            return !is_running_ || !queue_.empty();
            });

        if (!is_running_ && queue_.empty()) return false;

        data = queue_.front();
        queue_.pop();
        cv_not_full_.notify_one();
        return true;
    }

    // 停止队列
    void stop() {
        is_running_ = false;
        cv_not_full_.notify_all();
        cv_not_empty_.notify_all();
    }

    // 获取队列大小
    size_t size() {
        std::lock_guard<std::mutex> lock(mtx_);
        return queue_.size();
    }

private:
    std::queue<T> queue_;
    std::mutex mtx_;
    std::condition_variable cv_not_full_;
    std::condition_variable cv_not_empty_;
    int max_size_;
    std::atomic<bool> is_running_;
};

// 帧数据结构（传递原始YUV帧）
struct FrameData {
    AVFrame* frame = nullptr;
    int64_t pts = 0;  // 显示时间戳
    bool is_eof = false; // 是否文件读取完成

    FrameData() = default;
    ~FrameData() {
        //if (frame) {
        //    av_frame_free(&frame);
        //}
    }

    //// 禁止拷贝，避免重复释放
    //FrameData(const FrameData&) = delete;
    //FrameData& operator=(const FrameData&) = delete;

    //// 允许移动
    //FrameData(FrameData&& other) noexcept {
    //    frame = other.frame;
    //    pts = other.pts;
    //    is_eof = other.is_eof;
    //    other.frame = nullptr;
    //}

    //FrameData& operator=(FrameData&& other) noexcept {
    //    if (this != &other) {
    //        if (frame) av_frame_free(&frame);
    //        frame = other.frame;
    //        pts = other.pts;
    //        is_eof = other.is_eof;
    //        other.frame = nullptr;
    //    }
    //    return *this;
    //}
};

// MP4文件读取解码类（生产者）
class VideoReader {
public:
    VideoReader(const std::string& input_path, SafeQueue<FrameData>& queue)
        : input_path_(input_path), queue_(queue), is_running_(false) {}

    ~VideoReader() {
        stop();
        cleanup();
    }

    // 启动读取线程
    bool start() {
        if (is_running_) return false;

        // 初始化FFmpeg上下文
        if (!init()) {
            cleanup();
            return false;
        }

        is_running_ = true;
        read_thread_ = std::thread(&VideoReader::read_loop, this);
        return true;
    }

    // 停止读取
    void stop() {
        is_running_ = false;
        if (read_thread_.joinable()) {
            read_thread_.join();
        }
    }

    // 获取视频参数（给推流器用）
    void get_video_params(int& width, int& height, AVRational& time_base, AVRational& frame_rate) {
        width = codec_ctx_->width;
        height = codec_ctx_->height;
        time_base = stream_->time_base;
        frame_rate = codec_ctx_->framerate;
    }

private:
    // 初始化读取上下文
    bool init() {
        // 1. 打开输入文件
        int ret = avformat_open_input(&fmt_ctx_, input_path_.c_str(), nullptr, nullptr);
        if (ret < 0) {
            print_error("avformat_open_input failed", ret);
            return false;
        }

        // 2. 查找流信息
        ret = avformat_find_stream_info(fmt_ctx_, nullptr);
        if (ret < 0) {
            print_error("avformat_find_stream_info failed", ret);
            return false;
        }

        // 3. 查找视频流
        video_stream_idx_ = av_find_best_stream(fmt_ctx_, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        if (video_stream_idx_ < 0) {
            fprintf(stderr, "No video stream found in %s\n", input_path_.c_str());
            return false;
        }
        stream_ = fmt_ctx_->streams[video_stream_idx_];

        // 4. 查找解码器
        const AVCodec* codec = avcodec_find_decoder(stream_->codecpar->codec_id);
        if (!codec) {
            fprintf(stderr, "Failed to find decoder\n");
            return false;
        }

        // 5. 初始化解码器上下文
        codec_ctx_ = avcodec_alloc_context3(codec);
        if (!codec_ctx_) {
            fprintf(stderr, "Failed to alloc codec context\n");
            return false;
        }

        ret = avcodec_parameters_to_context(codec_ctx_, stream_->codecpar);
        if (ret < 0) {
            print_error("avcodec_parameters_to_context failed", ret);
            return false;
        }

        // 6. 打开解码器
        ret = avcodec_open2(codec_ctx_, codec, nullptr);
        if (ret < 0) {
            print_error("avcodec_open2 failed", ret);
            return false;
        }

        // 7. 初始化像素格式转换上下文（转为YUV420P）
        sws_ctx_ = sws_getContext(
            codec_ctx_->width, codec_ctx_->height, codec_ctx_->pix_fmt,
            codec_ctx_->width, codec_ctx_->height, AV_PIX_FMT_YUV420P,
            SWS_BILINEAR, nullptr, nullptr, nullptr
        );
        if (!sws_ctx_) {
            fprintf(stderr, "Failed to create sws context\n");
            return false;
        }

        // 8. 初始化输出帧
        output_frame_ = av_frame_alloc();
        if (!output_frame_) {
            fprintf(stderr, "Failed to alloc output frame\n");
            return false;
        }
        output_frame_->format = AV_PIX_FMT_YUV420P;
        output_frame_->width = codec_ctx_->width;
        output_frame_->height = codec_ctx_->height;
        ret = av_frame_get_buffer(output_frame_, 32);
        if (ret < 0) {
            print_error("av_frame_get_buffer failed", ret);
            return false;
        }

        av_dump_format(fmt_ctx_, 0, input_path_.c_str(), 0);
        return true;
    }

    // 读取循环（线程函数）
    void read_loop() {
        AVPacket* pkt = av_packet_alloc();

        while (is_running_) {
            // 读取数据包
            int ret = av_read_frame(fmt_ctx_, pkt);
            if (ret < 0) {
                if (ret == AVERROR_EOF) {
                    // 文件读取完成，发送结束标记
                    FrameData eof_frame;
                    eof_frame.is_eof = true;
                    queue_.push(std::move(eof_frame));
                    break;
                }
                print_error("av_read_frame failed", ret);
                continue;
            }

            // 只处理视频流
            if (pkt->stream_index != video_stream_idx_) {
                av_packet_unref(pkt);
                continue;
            }

            // 发送数据包到解码器
            ret = avcodec_send_packet(codec_ctx_, pkt);
            av_packet_unref(pkt);
            if (ret < 0) {
                print_error("avcodec_send_packet failed", ret);
                continue;
            }

            // 接收解码后的帧
            while (ret >= 0) {
                AVFrame* frame = av_frame_alloc();
                ret = avcodec_receive_frame(codec_ctx_, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    av_frame_free(&frame);
                    break;
                } else if (ret < 0) {
                    print_error("avcodec_receive_frame failed", ret);
                    av_frame_free(&frame);
                    break;
                }

                //// 转换为YUV420P
                //av_frame_make_writable(output_frame_);
                //sws_scale(
                //    sws_ctx_, frame->data, frame->linesize, 0, codec_ctx_->height,
                //    output_frame_->data, output_frame_->linesize
                //);

                // 封装帧数据并推入队列
                FrameData frame_data;
                //frame_data.frame = av_frame_clone(output_frame_);
                frame_data.frame = frame;
                frame_data.pts = frame->pts;
                queue_.push(std::move(frame_data));

                //av_frame_free(&frame);
            }
        }
        av_packet_free(&pkt);
    }

    // 清理资源
    void cleanup() {
        if (sws_ctx_) sws_freeContext(sws_ctx_);
        if (codec_ctx_) avcodec_free_context(&codec_ctx_);
        if (fmt_ctx_) avformat_close_input(&fmt_ctx_);
        if (output_frame_) av_frame_free(&output_frame_);
        sws_ctx_ = nullptr;
        codec_ctx_ = nullptr;
        fmt_ctx_ = nullptr;
        output_frame_ = nullptr;
        video_stream_idx_ = -1;
    }

    // 打印错误信息
    void print_error(const char* msg, int err_code) {
        char err_buf[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        av_strerror(err_code, err_buf, sizeof(err_buf));
        fprintf(stderr, "%s: %s (code: %d)\n", msg, err_buf, err_code);
    }

private:
    std::string input_path_;
    SafeQueue<FrameData>& queue_;
    std::thread read_thread_;
    std::atomic<bool> is_running_;

    // FFmpeg上下文
    AVFormatContext* fmt_ctx_ = nullptr;
    AVCodecContext* codec_ctx_ = nullptr;
    AVStream* stream_ = nullptr;
    struct SwsContext* sws_ctx_ = nullptr;
    AVFrame* output_frame_ = nullptr;
    int video_stream_idx_ = -1;
};

// RTSP推流编码类（消费者）
class RTSPSender {
public:
    RTSPSender(const std::string& rtsp_url, SafeQueue<FrameData>& queue)
        : rtsp_url_(rtsp_url), queue_(queue), is_running_(false) {}

    ~RTSPSender() {
        stop();
        cleanup();
    }

    // 启动推流线程
    bool start(int width, int height, AVRational time_base, AVRational frame_rate, int bitrate = 4000000) {
        if (is_running_) return false;

        // 初始化推流上下文
        if (!init(width, height, time_base, frame_rate, bitrate)) {
            cleanup();
            return false;
        }

        is_running_ = true;
        send_thread_ = std::thread(&RTSPSender::send_loop, this);
        return true;
    }

    // 停止推流
    void stop() {
        is_running_ = false;
        if (send_thread_.joinable()) {
            send_thread_.join();
        }
    }

private:
    // 初始化推流上下文
    bool init(int width, int height, AVRational time_base, AVRational frame_rate, int bitrate) {
        // 1. 初始化网络
        avformat_network_init();

        // 2. 创建输出上下文
        int ret = avformat_alloc_output_context2(&fmt_ctx_, nullptr, "rtsp", rtsp_url_.c_str());
        if (ret < 0) {
            print_error("avformat_alloc_output_context2 failed", ret);
            return false;
        }

        // 3. 设置RTSP参数
        AVDictionary* opts = nullptr;
        av_dict_set(&opts, "rtsp_transport", "tcp", 0);
        av_dict_set(&opts, "stimeout", "5000000", 0);

        // 4. 查找H.264编码器
        const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
        if (!codec) {
            fprintf(stderr, "Failed to find H.264 encoder\n");
            return false;
        }

        // 5. 创建视频流
        stream_ = avformat_new_stream(fmt_ctx_, nullptr);
        if (!stream_) {
            print_error("avformat_new_stream failed", ret);
            return false;
        }
        stream_->id = fmt_ctx_->nb_streams - 1;

        // 6. 初始化编码器上下文
        codec_ctx_ = avcodec_alloc_context3(codec);
        if (!codec_ctx_) {
            fprintf(stderr, "Failed to alloc codec context\n");
            return false;
        }

        // 7. 配置编码器参数
        codec_ctx_->codec_id = AV_CODEC_ID_H264;
        codec_ctx_->width = width;
        codec_ctx_->height = height;
        codec_ctx_->pix_fmt = AV_PIX_FMT_YUV420P;
        codec_ctx_->bit_rate = bitrate;
        codec_ctx_->time_base = time_base;
        codec_ctx_->framerate = frame_rate;
        codec_ctx_->gop_size = 25;
        codec_ctx_->max_b_frames = 0;
        codec_ctx_->thread_count = 4;
        codec_ctx_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

        // 8. 编码器优化参数（实时推流）
        AVDictionary* codec_opts = nullptr;
        av_dict_set(&codec_opts, "preset", "fast", 0);
        av_dict_set(&codec_opts, "tune", "zerolatency", 0);
        av_dict_set(&codec_opts, "profile", "baseline", 0);

        // 9. 打开编码器
        ret = avcodec_open2(codec_ctx_, codec, &codec_opts);
        av_dict_free(&codec_opts);
        if (ret < 0) {
            print_error("avcodec_open2 failed", ret);
            return false;
        }

        // 10. 复制编码器参数到流
        ret = avcodec_parameters_from_context(stream_->codecpar, codec_ctx_);
        if (ret < 0) {
            print_error("avcodec_parameters_from_context failed", ret);
            return false;
        }
        stream_->time_base = codec_ctx_->time_base;

        // 11. 打开RTSP IO
        if (!(fmt_ctx_->oformat->flags & AVFMT_NOFILE)) {
            ret = avio_open2(&fmt_ctx_->pb, rtsp_url_.c_str(), AVIO_FLAG_WRITE, nullptr, &opts);
            if (ret < 0) {
                print_error("avio_open2 failed", ret);
                av_dict_free(&opts);
                return false;
            }
        }
        av_dict_free(&opts);

        // 12. 写入头部
        ret = avformat_write_header(fmt_ctx_, nullptr);
        if (ret < 0) {
            print_error("avformat_write_header failed", ret);
            return false;
        }

        av_dump_format(fmt_ctx_, 0, rtsp_url_.c_str(), 1);
        return true;
    }

    // 推流循环（线程函数）
    void send_loop() {
        FrameData frame_data;
        AVPacket* pkt = av_packet_alloc();

        while (is_running_) {
            // 从队列取帧
            if (!queue_.pop(frame_data)) {
                break;
            }

            // 处理结束标记
            if (frame_data.is_eof) {
                break;
            }

            // 空帧跳过
            if (!frame_data.frame) {
                continue;
            }

            // 设置帧时间戳
            frame_data.frame->pts = frame_data.pts;

            // 发送帧到编码器
            int ret = avcodec_send_frame(codec_ctx_, frame_data.frame);
            if (ret < 0) {
                print_error("avcodec_send_frame failed", ret);
                continue;
            }

            // 接收编码后的数据包并推流
            while (ret >= 0) {
                ret = avcodec_receive_packet(codec_ctx_, pkt);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                } else if (ret < 0) {
                    print_error("avcodec_receive_packet failed", ret);
                    break;
                }

                // 调整时间戳
                av_packet_rescale_ts(pkt, codec_ctx_->time_base, stream_->time_base);
                pkt->stream_index = stream_->index;

                // 写入RTSP流
                ret = av_interleaved_write_frame(fmt_ctx_, pkt);
                if (ret < 0) {
                    print_error("av_interleaved_write_frame failed", ret);
                } else {
                    static int frame_count = 0;
                    if (++frame_count % 100 == 0) {
                        printf("Pushed %d frames, queue size: %zu\n", frame_count, queue_.size());
                    }
                }

                av_packet_unref(pkt);
            }
        }

        // 发送编码器尾帧
        avcodec_send_frame(codec_ctx_, nullptr);
        while (true) {
            int ret = avcodec_receive_packet(codec_ctx_, pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            } else if (ret < 0) {
                print_error("avcodec_receive_packet (flush) failed", ret);
                break;
            }

            av_packet_rescale_ts(pkt, codec_ctx_->time_base, stream_->time_base);
            pkt->stream_index = stream_->index;
            av_interleaved_write_frame(fmt_ctx_, pkt);
            av_packet_unref(pkt);
        }

        // 写入尾部
        av_write_trailer(fmt_ctx_);
    }

    // 清理资源
    void cleanup() {
        if (codec_ctx_) avcodec_free_context(&codec_ctx_);
        if (fmt_ctx_) {
            if (!(fmt_ctx_->oformat->flags & AVFMT_NOFILE)) {
                avio_closep(&fmt_ctx_->pb);
            }
            avformat_free_context(fmt_ctx_);
        }
        avformat_network_deinit();
        codec_ctx_ = nullptr;
        fmt_ctx_ = nullptr;
        stream_ = nullptr;
    }

    // 打印错误信息
    void print_error(const char* msg, int err_code) {
        char err_buf[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        av_strerror(err_code, err_buf, sizeof(err_buf));
        fprintf(stderr, "%s: %s (code: %d)\n", msg, err_buf, err_code);
    }

private:
    std::string rtsp_url_;
    SafeQueue<FrameData>& queue_;
    std::thread send_thread_;
    std::atomic<bool> is_running_;

    // FFmpeg上下文
    AVFormatContext* fmt_ctx_ = nullptr;
    AVCodecContext* codec_ctx_ = nullptr;
    AVStream* stream_ = nullptr;
};

// 主函数
int main02(int argc, char* argv[]) {
    //if (argc != 3) {
    //    fprintf(stderr, "Usage: %s <input_mp4_path> <rtsp_url>\n", argv[0]);
    //    fprintf(stderr, "Example: %s test.mp4 rtsp://127.0.0.1:554/stream/test\n", argv[0]);
    //    return -1;
    //}
    //std::string input_path = argv[1];
    //std::string rtsp_url = argv[2];
    std::string input_path = R"(rtsp://admin:admin@123@172.16.45.172:554/unicast/c1/s0/live)";
    std::string rtsp_url = R"(rtsp://127.0.0.1:554/live/test)";


    // 1. 创建线程安全队列（缓冲区）
    SafeQueue<FrameData> frame_queue(30); // 队列最大30帧，避免内存占用过高

    // 2. 创建读取器和推流器
    VideoReader reader(input_path, frame_queue);
    RTSPSender sender(rtsp_url, frame_queue);

    // 3. 启动读取器
    if (!reader.start()) {
        fprintf(stderr, "Failed to start video reader\n");
        return -1;
    }

    // 4. 获取视频参数并启动推流器
    int width, height;
    AVRational time_base, frame_rate;
    reader.get_video_params(width, height, time_base, frame_rate);

    if (!sender.start(width, height, time_base, frame_rate, 4000000)) {
        fprintf(stderr, "Failed to start RTSP sender\n");
        reader.stop();
        return -1;
    }

    // 5. 等待用户输入退出
    printf("Press Enter to stop pushing...\n");
    getchar();

    // 6. 停止推流和读取
    sender.stop();
    reader.stop();
    frame_queue.stop();

    printf("Push finished\n");
    return 0;
}