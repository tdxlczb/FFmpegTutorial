#include "video_decoder.h"
extern "C"
{
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/frame.h>
#include <libswscale/swscale.h>
}

VideoReader::VideoReader(const std::string& videoFile, size_t frameInterval, int maxCacheFrameSize)
    : _formatContext(nullptr)
    , _codecParameters(nullptr)
    , _codec(nullptr)
    , _codecContext(nullptr)
    , _frameInterval(frameInterval)
    , _maxCacheFrameSize(maxCacheFrameSize)
{
    _isOpen = OpenFile(videoFile);
    if (_isOpen && frameInterval > 0)
    {
        // 启动解码线程
        _threadRun    = true;
        _isDecoding   = true;
        _decodeThread = std::thread(&VideoReader::DecoderThread, this);
    }
}

VideoReader::~VideoReader()
{
    printf("INFO: [Video Decoder] [BEGIN] Rlease FFMPEG videoReader ...\n");

    _threadRun = false;
    if (_decodeThread.joinable())
    {
        _decodeThread.join();
    }
    avcodec_free_context(&_codecContext);
    avformat_close_input(&_formatContext);
    avformat_network_deinit();
    printf("INFO: [Video Decoder] [END] Rlease FFMPEG videoReader Done. \n");
}

bool VideoReader::IsOpened()
{
    return _isOpen;
}

bool VideoReader::IsDecoding()
{
    return _isDecoding;
}

void VideoReader::StopDecoding()
{
    _threadRun = false;
}

bool VideoReader::OpenFile(const std::string& videoFile)
{
    printf("INFO: [Video Decoder] Open video: %s\n", videoFile.c_str());
    // 初始化FFmpeg
    avformat_network_init();

    // 打开视频文件
    if (avformat_open_input(&_formatContext, videoFile.c_str(), nullptr, nullptr) != 0)
    {
        printf("ERROR: [Video Decoder] Failed to open video file: %s\n", videoFile.c_str());
        avformat_network_deinit();
        return false;
    }

    // 获取视频流信息
    if (avformat_find_stream_info(_formatContext, nullptr) < 0)
    {
        printf("ERROR: [Video Decoder] Failed to find stream information\n");
        avformat_close_input(&_formatContext);
        avformat_network_deinit();
        return false;
    }

    //打印相关信息
    printf(
        "INFO: [Video Decoder] duration:%lld, number of streams:%d, format name: %s\n", _formatContext->duration, _formatContext->nb_streams,
        _formatContext->iformat->name
    );

    // 找到视频流索引
    _videoStreamIndex = -1;
    for (unsigned int i = 0; i < _formatContext->nb_streams; ++i)
    {
        if (_formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            _videoStreamIndex = i;
            break;
        }
    }
    if (_videoStreamIndex == -1)
    {
        printf("ERROR: [Video Decoder] Failed to find video stream\n");
        avformat_close_input(&_formatContext);
        avformat_network_deinit();
        return false;
    }
    AVStream* videoStream = _formatContext->streams[_videoStreamIndex];
    _timeBase             = videoStream->time_base;
    _fps                  = av_q2d(videoStream->avg_frame_rate);
    _frameCount           = videoStream->nb_frames;

    // 获取视频解码器
    _codecParameters = _formatContext->streams[_videoStreamIndex]->codecpar;
    _codec           = avcodec_find_decoder(_codecParameters->codec_id);
    if (_codec == nullptr)
    {
        printf("ERROR: [Video Decoder] Failed to find video decoder\n");
        avcodec_free_context(&_codecContext);
        avformat_close_input(&_formatContext);
        avformat_network_deinit();
        return false;
    }

    // 创建解码器上下文
    _codecContext = avcodec_alloc_context3(_codec);
    if (avcodec_parameters_to_context(_codecContext, _codecParameters) < 0)
    {
        printf("ERROR: [Video Decoder] Failed to copy codec parameters to codec context\n");
        avcodec_free_context(&_codecContext);
        avformat_close_input(&_formatContext);
        avformat_network_deinit();
        return false;
    }
    if (avcodec_open2(_codecContext, _codec, nullptr) != 0)
    {
        printf("ERROR: [Video Decoder] Failed to open video decoder\n");
        avcodec_free_context(&_codecContext);
        avformat_close_input(&_formatContext);
        avformat_network_deinit();
        return false;
    }
    if (_codecContext->width <= 0 || _codecContext->height <= 0 || _codecContext->pix_fmt == AV_PIX_FMT_NONE)
    {
        printf("ERROR: [Video Decoder] open video decoder error\n");
        avcodec_free_context(&_codecContext);
        avformat_close_input(&_formatContext);
        avformat_network_deinit();
        return false;
    }
    return true;
}

void VideoReader::DecoderThread()
{
    if (!_isOpen || _frameInterval <= 0)
    {
        _isDecoding = false;
        return;
    }
    AVPixelFormat pixFormat = AV_PIX_FMT_BGR24;
    //使用sws_getContext会出现内存泄漏，使用sws_getCachedContext代替
    SwsContext*   sws_ctx   = sws_getCachedContext(
        NULL, _codecContext->width, _codecContext->height, _codecContext->pix_fmt, _codecContext->width, _codecContext->height, AV_PIX_FMT_BGR24,
        SWS_BILINEAR, NULL, NULL, NULL
    );

    if (!sws_ctx)
    {
        _isDecoding = false;
        return;
    }

    // 创建RGB帧
    AVFrame* frameRGB = av_frame_alloc();
    int      numBytes = av_image_get_buffer_size(pixFormat, _codecContext->width, _codecContext->height, AV_INPUT_BUFFER_PADDING_SIZE);
    uint8_t* buffer   = (uint8_t*) av_malloc(numBytes * sizeof(uint8_t));
    av_image_fill_arrays(
        frameRGB->data, frameRGB->linesize, buffer, pixFormat, _codecContext->width, _codecContext->height, AV_INPUT_BUFFER_PADDING_SIZE
    );

    int       frameIndex = -1;
    AVFrame*  frame      = av_frame_alloc();
    AVPacket* packet     = av_packet_alloc();
    while (_threadRun && av_read_frame(_formatContext, packet) >= 0)
    {
        if (packet->stream_index == _videoStreamIndex)
        {
            if (avcodec_send_packet(_codecContext, packet) < 0)
            {
                // 处理发送数据包到解码器失败的情况
                av_packet_unref(packet);
                break;
            }

            while (avcodec_receive_frame(_codecContext, frame) >= 0)
            {
                frameIndex++;
                if (frameIndex % _frameInterval != 0)
                {
                    av_frame_unref(frame);
                    continue;
                }

                int ret = sws_scale(sws_ctx, frame->data, frame->linesize, 0, _codecContext->height, frameRGB->data, frameRGB->linesize);
                if (ret <= 0)
                {
                    av_frame_unref(frame);
                    continue;
                }

                while (_maxCacheFrameSize >= 0 && GetCacheFrameSize() >= _maxCacheFrameSize)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }

                cv::Mat mat = cv::Mat(_codecContext->height, _codecContext->width, CV_8UC3, frameRGB->data[0], frameRGB->linesize[0]);

                double     timestamp = frame->pts * av_q2d(_timeBase) * 1000;
                VideoFrame videoFrame;
                videoFrame.msTs = (int) timestamp;
                mat.copyTo(videoFrame.mat);
                std::lock_guard<std::mutex> lock(_frameQueueMutex);
                _frameQueue.push(videoFrame);

                av_frame_unref(frame);
            }
            av_frame_unref(frame);
        }
        av_packet_unref(packet);
    }

    av_packet_unref(packet);
    av_packet_free(&packet);
    av_frame_unref(frame);
    av_frame_free(&frame);
    av_freep(&buffer); //释放av_malloc申请的buffer，需要单独释放

    sws_freeContext(sws_ctx);
    _isDecoding = false;
    return;
}

bool VideoReader::GetFrame(VideoFrame& frame)
{
    if (!_isOpen)
        return false;
    std::lock_guard<std::mutex> lock(_frameQueueMutex);
    if (_frameQueue.empty())
        return false;
    frame = _frameQueue.front();
    _frameQueue.pop();
    return true;
}

int VideoReader::GetCacheFrameSize()
{
    std::lock_guard<std::mutex> lock(_frameQueueMutex);
    return _frameQueue.size();
}

int VideoReader::GetFPS()
{
    return (int) _fps;
}

int VideoReader::GetTotalFrameCount()
{
    return _frameCount;
}
