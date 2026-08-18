#pragma once
#include <string>
#include <vector>
#include <mutex>

extern "C"
{
#include <libavcodec/avcodec.h>
#include "libavformat/avformat.h"
}

class MP4Recorder
{
public:
    MP4Recorder();
    ~MP4Recorder();

    bool Init(std::vector<AVCodecParameters*> codecpars, const std::string& filepath);
    bool DeInit();
    bool SaveOneFrame(AVMediaType mediaType, AVRational timebase, AVPacket* pkt);
private:
    AVFormatContext* m_outputCtx = nullptr;
    std::mutex      m_mtx;
    bool            m_isInit = false;
    int64_t         m_iFirstVideoPts = AV_NOPTS_VALUE;
    int64_t         m_iFirstAudioPts = AV_NOPTS_VALUE;
    bool            m_isWaitRecord = false; //是否等待录制，用于等待第一个关键帧
};
