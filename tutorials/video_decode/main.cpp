#include <iostream>
#include <string>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <functional>
#include <logger/logger.h>
#include <foundation/file/file_utils.h>
#include "examples.h"

int multi_thread_test()
{
    std::string output   = "E:\\res\\mca\\output";
    std::string filePath = "E:\\res\\mca\\dump.265";

    int threadNum = 1;
    std::cin >> threadNum;
    std::vector<std::thread> threads;
    for (size_t i = 0; i < threadNum; i++)
    {
        auto th = std::thread([=]() {
            VideoToImages(i, filePath, output);
            });
        threads.push_back(std::move(th));
    }
    getchar();
    for (size_t i = 0; i < threadNum; i++)
    {
        threads[i].join();
    }
    return 0;
}

int video_to_image()
{
    std::string output   = "E:\\res\\mca\\output";
    std::string filePath = "E:\\res\\mca\\dump.265";

    std::string dir  = R"(E:\res\mca\1703762903540_2)";
    std::string dir1 = R"(E:\res\mca\1701047259978_141)";

    std::vector<std::string> fileList;
    foundation::FileUtils::GetFileList2(dir1, fileList, "*.ts");

    int index = 0;
    for (size_t i = 0; i < fileList.size(); i++)
    {
        index++;
        auto startTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        bool ret = VideoToImages(0, fileList[i], output);
        if (!ret)
        {
            std::cout << "err index=" << index << std::endl;
        }
        auto endTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        std::cout << "duration=" << endTime - startTime << std::endl;
    }
    getchar();
    return 0;
}


#include <libavcodec/avcodec.h>
#include <libavutil/hwcontext.h>
#include <iostream>

void list_hwaccels()
{
    AVHWDeviceType type = AV_HWDEVICE_TYPE_NONE;
    std::cout << "Supported hwaccels: ";
    while ((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE)
        std::cout << av_hwdevice_get_type_name(type) << " ";
    std::cout << std::endl;
}

void list_hw_decoders(AVCodecID id)
{
    std::cout << "Supported hwdecoders: ";
    const AVCodec* c = nullptr;
    void* i = nullptr;
    while ((c = av_codec_iterate(&i))) {
        if (c->type == AVMEDIA_TYPE_VIDEO &&
            c->id == id &&
            (c->capabilities & AV_CODEC_CAP_HARDWARE)) {
            std::cout << c->name << std::endl;
        }
    }
    std::cout << std::endl;
}

void FindEncoders()
{
    std::cout << "Available encoders:\n";
    std::cout << "========================================\n";

    const AVCodec* codec = nullptr;
    void* iter = nullptr;

    // 遍历所有编解码器
    while ((codec = av_codec_iterate(&iter))) {
        //if (!av_codec_is_encoder(codec)) {
        //    continue;  // 只关注编码器
        //}

        if (!av_codec_is_decoder(codec)) {
            continue;  // 只关注解码器
        }

        //// 格式化打印
        //// 输出宽度10个字符，左对齐，不足补空格，输出3
        //std::cout << std::setw(10) << std::setfill(' ') << std::left << 2 << std::endl;
        //// 输出宽度14，右对齐，不足补0，输出10
        //std::cout << std::setw(14) << std::setfill('0') << std::right << 10 << std::endl;

        std::cout << "Name: " << codec->name << "\n";
        if (codec->wrapper_name)
            std::cout << "Wrapper Name: " << codec->wrapper_name << "\n";
        else
            std::cout << "Wrapper Name:  \n";

        if (codec->long_name)
            std::cout << "Long Name: " << codec->long_name << "\n";
        else
            std::cout << "Long Name:  \n";

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

int main()
{
    //FindEncoders();
    list_hwaccels();                
    list_hw_decoders(AV_CODEC_ID_H264);
    list_hw_decoders(AV_CODEC_ID_H265);
    InitLogger();
    LOG_INFO << "==================================";
    std::string output    = R"(E:\code\media\temp)";
    std::string filePath  = R"(E:\code\media\traffic.mp4)";
    std::string filePath2 = R"(rtsp://172.16.19.44:554/rtp/34020000001180000195_34020000001310000006_5?token=WSGLtsoIcY7bf25L)";
    std::string filePath3 = R"(rtsp://172.16.19.44:554/rtp/34020000001180000195_34020000001310000002_5?token=G9dSZrnumeb1TDSf)";
    std::string filePath4 = R"(rtsp://127.0.0.1/live/test)";

    VideoToImages(1, filePath4, output);
    //multi_thread_test();
    //video_to_image();

    //int threadNum = 16;
    //std::vector<std::thread> threads;
    //for (size_t i = 0; i < threadNum; i++)
    //{
    //    auto th = std::thread(VideoToImages, i, filePath2, output);
    //    threads.push_back(std::move(th));
    //}
    //getchar();
    //for (size_t i = 0; i < threadNum; i++)
    //{
    //    threads[i].join();
    //}

    getchar();
    return 0;
}


// Force C linkage for FFmpeg C APIs (required for C++)
extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/pixfmt.h>    // for av_get_pix_fmt_name, AVPixelFormat
#include <libavutil/pixdesc.h>   // for av_pix_fmt_desc_get, AVPixFmtDescriptor (CRITICAL!)
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/hwcontext.h> // for AV_HWDEVICE_TYPE_DXVA2
}

#include <iostream>
#include <cstring>

// Print all supported decoders (hardware/software)
void print_all_decoders() {
    std::cout << "==================== FFmpeg Supported Decoders ====================" << std::endl;
    void* iter = nullptr;
    const AVCodec* codec = nullptr;

    while ((codec = av_codec_iterate(&iter))) {
        if (av_codec_is_decoder(codec)) {
            std::string hw_accel = "Software";
            // Check if decoder supports hardware acceleration
            if (codec->capabilities & AV_CODEC_CAP_HARDWARE) {
                hw_accel = "Hardware";
                // Identify DXVA2-specific decoders
                if (strstr(codec->name, "dxva2") != nullptr) {
                    hw_accel = "Hardware (DXVA2)";
                }
            }

            std::cout << "Decoder Name: " << codec->name
                << "\tType: " << (codec->type == AVMEDIA_TYPE_VIDEO ? "Video" : "Audio/Other")
                << "\tAcceleration: " << hw_accel
                << "\tDescription: " << (codec->long_name ? codec->long_name : "N/A") << std::endl;
        }
    }
    std::cout << "===================================================================" << std::endl;
}

// Check if AV_PIX_FMT_DXVA2_VLD is supported
void check_dxva2_pixel_format() {
    std::cout << "\n==================== DXVA2 Pixel Format Check ====================" << std::endl;

#ifdef AV_PIX_FMT_DXVA2_VLD
    // Get pixel format name
    const char* fmt_name = av_get_pix_fmt_name(static_cast<AVPixelFormat>(AV_PIX_FMT_DXVA2_VLD));
    std::cout << "Checking Pixel Format: " << (fmt_name ? fmt_name : "Unknown")
        << " (Enum Value: " << AV_PIX_FMT_DXVA2_VLD << ")" << std::endl;

    // Get pixel format descriptor (from pixdesc.h)
    const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get(static_cast<AVPixelFormat>(AV_PIX_FMT_DXVA2_VLD));
    if (desc) {
        std::cout << "DXVA2_VLD Properties:" << std::endl;
        std::cout << "  - Bit Depth: " << static_cast<int>(desc->comp[0].depth) << std::endl;
        std::cout << "  - Hardware Format: " << (desc->flags & AV_PIX_FMT_FLAG_HWACCEL ? "Yes" : "No") << std::endl;
        std::cout << "  - Chroma Type: " << (desc->log2_chroma_w == 1 ? "YUV420" : "Other") << std::endl;
        std::cout << "Result: AV_PIX_FMT_DXVA2_VLD is supported!" << std::endl;
    } else {
        std::cout << "Result: AV_PIX_FMT_DXVA2_VLD is defined but descriptor not found!" << std::endl;
    }
#else
    std::cout << "Result: AV_PIX_FMT_DXVA2_VLD is NOT defined (FFmpeg built without DXVA2 support)!" << std::endl;
#endif
    std::cout << "====================================================================" << std::endl;
}

// Check if DXVA2 hardware device is usable
void check_dxva2_hardware_device() {
    std::cout << "\n==================== DXVA2 Hardware Device Check ====================" << std::endl;

#ifdef AV_HWDEVICE_TYPE_DXVA2
    AVBufferRef* hw_device_ctx = nullptr;
    int ret = av_hwdevice_ctx_create(&hw_device_ctx, AV_HWDEVICE_TYPE_DXVA2, nullptr, nullptr, 0);
    if (ret >= 0) {
        std::cout << "Result: DXVA2 hardware device created successfully (DXVA2 is supported)!" << std::endl;
        av_buffer_unref(&hw_device_ctx);
    } else {
        std::cout << "Result: Failed to create DXVA2 hardware device! Error Code: " << ret
            << " Error Msg: " << av_err2str(ret) << std::endl;
    }
#else
    std::cout << "Result: AV_HWDEVICE_TYPE_DXVA2 is NOT defined (FFmpeg built without DXVA2 support)!" << std::endl;
#endif
    std::cout << "======================================================================" << std::endl;
}

int main00() {
    // Initialize FFmpeg (modern versions don't require av_register_all())
    av_log_set_level(AV_LOG_INFO);

    // Print FFmpeg version info
    std::cout << "FFmpeg Version Info:" << std::endl;
    std::cout << "  - libavutil: " << LIBAVUTIL_VERSION_MAJOR << "." << LIBAVUTIL_VERSION_MINOR << "." << LIBAVUTIL_VERSION_MICRO << std::endl;
    std::cout << "  - libavcodec: " << LIBAVCODEC_VERSION_MAJOR << "." << LIBAVCODEC_VERSION_MINOR << "." << LIBAVCODEC_VERSION_MICRO << std::endl;
    std::cout << "  - libavformat: " << LIBAVFORMAT_VERSION_MAJOR << "." << LIBAVFORMAT_VERSION_MINOR << "." << LIBAVFORMAT_VERSION_MICRO << std::endl;

    // 1. Print all decoders
    print_all_decoders();

    // 2. Check DXVA2 pixel format support
    check_dxva2_pixel_format();

    // 3. Check DXVA2 hardware device availability
    check_dxva2_hardware_device();

    // Prevent console from closing immediately (Windows only)
    std::cout << "\nPress Enter to exit..." << std::endl;
    std::cin.get();

    return 0;
}

#include <iostream>
#include <sstream>
#include <mutex>
#include <utility>   // std::move

class Logger {
public:
    static void setOutput(std::ostream& os) { s_out = &os; }

    // 工厂：返回可移动临时对象
    static Logger log() { return Logger(); }

    // 模板 << 支持任意可流式类型
    template<typename T>
    Logger& operator<<(const T& t) {
        m_oss << t;
        return *this;
    }

    // 支持 manipulator（如 std::endl）
    Logger& operator<<(std::ostream& (*manip)(std::ostream&)) {
        m_oss << manip;
        return *this;
    }

    // 关键：析构时输出 + 换行
    ~Logger() {
        std::lock_guard<std::mutex> lk(s_mutex);
        *s_out << m_oss.str() << '\n';
        s_out->flush();
    }

    // === 移动构造/赋值（C++11）===
    Logger(Logger&& other) noexcept : m_oss(std::move(other.m_oss)) {}
    Logger& operator=(Logger&&) noexcept = delete;

    // === 禁止拷贝 ===
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

private:
    Logger() = default;   // 只允许工厂函数创建
    std::ostringstream m_oss;
    static std::mutex  s_mutex;
    static std::ostream* s_out;
};

// 静态成员定义
std::mutex Logger::s_mutex;
std::ostream* Logger::s_out = &std::cout;


// 全局宏，方便使用
#define LOG Logger::log()

int main2() {
    LOG << "start, pid=" << 12345;
    int x = 98;
    LOG << "value=" << std::setw(4) << x << std::endl; // endl 也能用
    LOG << "中文测试 " << 3.14 << '!';
    return 0;
}