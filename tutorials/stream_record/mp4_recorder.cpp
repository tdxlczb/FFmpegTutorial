#include "mp4_recorder.h"



MP4Recorder::MP4Recorder()
{
}

MP4Recorder::~MP4Recorder()
{
}

bool MP4Recorder::Init(std::vector<AVCodecParameters*> codecpars, const std::string& filepath)
{
    std::lock_guard<std::mutex> lock(m_mtx);
    // 错误日志
    char errMsg[1024] = { 0 };

    // 输出上下文，格式："mpegts"(ts)，"matroska"(mkv)，"mov"，"avi"，"mp4"
    int ret = avformat_alloc_output_context2(&m_outputCtx, nullptr, "mov", filepath.c_str());
    if (ret < 0) {
        av_strerror(ret, errMsg, sizeof(errMsg));
        fprintf(stderr, "%s avformat_alloc_output_context2 failed! errMsg:%s\n", __func__, errMsg);
        return false;
    }

    // 创建输出流,写入文件调用avio_open2
    ret = avio_open2(&m_outputCtx->pb, filepath.c_str(), AVIO_FLAG_READ_WRITE, nullptr, nullptr);
    if (ret < 0) {
        av_strerror(ret, errMsg, sizeof(errMsg));
        fprintf(stderr, "%s avio_open2 failed! errMsg:%s\n", __func__, errMsg);
        DeInit();
        return false;
    }

    // 遍历输入流，拷贝输入流的编码参数到输出流
    for (int i = 0; i < codecpars.size(); i++) {
        // 创建输出流
        AVStream* out_stream = avformat_new_stream(m_outputCtx, nullptr);
        // 拷贝输入流的编码参数到输出流
        ret = avcodec_parameters_copy(out_stream->codecpar, codecpars[i]);
        if (ret < 0) {
            av_strerror(ret, errMsg, sizeof(errMsg));
            fprintf(stderr, "%s avcodec_parameters_copy failed! errMsg:%s\n", __func__, errMsg);
            DeInit();
            return false;
        }
    }

    // 初始化媒体文件的头部信息
    ret = avformat_write_header(m_outputCtx, nullptr);
    if (ret < 0) {
        av_strerror(ret, errMsg, sizeof(errMsg));
        fprintf(stderr, "%s avformat_write_header failed! errMsg:%s\n", __func__, errMsg);
        DeInit();
        return false;
    }

    fprintf(stdout, "%s open %s success!", __func__, filepath.c_str());
    m_isInit = true;
    return true;
}

bool MP4Recorder::DeInit()
{
    if (m_isInit && m_outputCtx)
    {
        av_write_trailer(m_outputCtx);
    }

    if (m_outputCtx && m_outputCtx->pb) {
        avio_closep(&m_outputCtx->pb);
    }
    if (m_outputCtx) {
        avformat_free_context(m_outputCtx);
    }
    return true;
}

bool MP4Recorder::SaveOneFrame(AVMediaType mediaType, AVRational timebase, AVPacket* pkt)
{
    //在等待录制的情况下，直到遇到一个视频关键帧才取消等待开始正式录制以预防花屏
    if (m_isWaitRecord && mediaType == AVMEDIA_TYPE_VIDEO && (pkt->flags & AV_PKT_FLAG_KEY)) {
        m_isWaitRecord = false;
    }
    if (m_isWaitRecord) {
        return false;
    }

    //不能并发操作AVFormatContext，需要加锁
    std::lock_guard<std::mutex> lock(m_mtx);
    //必须调用av_packet_rescale_ts转换时间戳
    av_packet_rescale_ts(pkt, timebase, m_outputCtx->streams[(int)mediaType]->time_base);
    if (pkt->pts == AV_NOPTS_VALUE) {
        pkt->pts = 0;
    }
    if (mediaType == AVMEDIA_TYPE_VIDEO) {
        if (m_iFirstVideoPts == AV_NOPTS_VALUE) {
            m_iFirstVideoPts = pkt->pts;
        }
        pkt->pts = pkt->pts - m_iFirstVideoPts;
        pkt->dts = pkt->dts - m_iFirstVideoPts;
        fprintf(stdout, "%s write pts %ld\n", __func__, pkt->pts);
        //qDebug() << "packet pts:" << pkt->pts << ", dts:" << pkt->dts;
    } else if (mediaType == AVMEDIA_TYPE_AUDIO) {
        if (m_iFirstAudioPts == AV_NOPTS_VALUE) {
            m_iFirstAudioPts = pkt->pts;
        }
        pkt->pts = pkt->pts - m_iFirstAudioPts;
        pkt->dts = pkt->dts - m_iFirstAudioPts;
    }
    int iRet = av_interleaved_write_frame(m_outputCtx, pkt);
    if (iRet < 0) {
        fprintf(stderr, "%s writePacket failed!\n", __func__);
        return false;
    }
    return true;
}
