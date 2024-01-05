#pragma once
#include <mutex>
#include <thread>
#include <queue>
#include <condition_variable>
#include <opencv2/opencv.hpp>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/frame.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

struct VideoFrame
{
    uint64_t msTs = 0; //毫秒时间戳
    bool     end  = false;
    cv::Mat  mat;
};

class VideoReader
{
public:
    VideoReader() = delete;
    VideoReader(const std::string& videoFile, size_t frameInterval, int maxCacheFrameSize = -1);
    ~VideoReader();
    bool IsOpened();
    bool IsDecoding();
    void StopDecoding();
    bool GetFrame(VideoFrame& frame);
    int  GetCacheFrameSize();
    int  GetFPS();
    int  GetTotalFrameCount();

private:
    bool OpenFile(const std::string& videoFile);
    //解码
    void DecoderThread();

private:
    AVFormatContext*   _formatContext;
    AVCodecParameters* _codecParameters;
    const AVCodec*     _codec;
    AVCodecContext*    _codecContext;
    int                _videoStreamIndex = -1;
    AVRational         _timeBase;
    double             _fps        = 0.0;
    int                _frameCount = 0;
    bool               _isOpen     = false;
    bool               _isDecoding = false;

    std::thread            _decodeThread;
    std::atomic_bool       _threadRun = false;
    std::queue<VideoFrame> _frameQueue;
    std::mutex             _frameQueueMutex;

    const size_t _frameInterval = 0; //取帧间隔, 每几帧取一帧, 为0时不取帧, 为1时每帧都取
    int _maxCacheFrameSize = -1; //-1表示无上限，缓存队列的数据，8k视频一帧占用48M，缓存太多帧会导致内存占用太大
};
