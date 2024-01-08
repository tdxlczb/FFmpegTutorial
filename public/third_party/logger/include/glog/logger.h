#ifndef UTILS_LOGGER_H
#define UTILS_LOGGER_H

#ifndef GLOG_NO_ABBREVIATED_SEVERITIES
#define GLOG_NO_ABBREVIATED_SEVERITIES
#endif

#include "glog/logging.h"
#include <string>

#if defined(_WIN32) && !defined(__CYGWIN__)
#  ifdef LIBGLOG_EXPORTS
#    define LOG_EXPORT extern "C" __declspec(dllexport)
#  else
#    define LOG_EXPORT extern "C" __declspec(dllimport)
#  endif
#else
#define LOG_EXPORT extern "C"
#endif

/************************************************************************/
/* 1、glog                                                              */
/************************************************************************/
/**
* 初始化日志
* 该方法只需在进程启动的时候调用一次，dll内无需调用
* 请不要多个进程共用同一份日志文件名，多个进程写同一个日志文件会导致错乱；如果同一程序需要启动多次，建议日志名初始化时带上进程pid
*
* @param logName 日志文件名，默认为空时，为进程文件名
* @param logPath 日志存放路径，默认为空时，日志存放路径为'进程所在路径\logs'
*                目录不存在时会自动创建目录，不会检查目录是否正确
* @param maxLogSize 日志路径下所有日志加起来的最大大小，默认1G，为0表示不限制，单位：M
* @param perLogSize 单个日志最大大小，默认10M，单位：M
* @return 0成功，其他失败
*/
GOOGLE_GLOG_DLL_DECL int  InitLogger(
    const std::string &logName = "",
    const std::string &logPath = "",
    const unsigned int &maxLogSize = 1024,
    const unsigned int &perLogSize = 10);

/**
* 卸载日志
* 进程退出时调用
*/
GOOGLE_GLOG_DLL_DECL void ReleaseLogger();

/**
* 写日志宏
* 写日志用，支持流操作，如：LOG_INFO << "string" << 1;
*/
#define LOG_DEBUG google::LogMessage(__FILE__, __LINE__, __FUNCTION__, google::GLOG_DEBUG).stream()
#define LOG_INFO  google::LogMessage(__FILE__, __LINE__, __FUNCTION__, google::GLOG_INFO).stream()
#define LOG_WARN  google::LogMessage(__FILE__, __LINE__, __FUNCTION__, google::GLOG_WARNING).stream()
#define LOG_ERROR google::LogMessage(__FILE__, __LINE__, __FUNCTION__, google::GLOG_ERROR).stream()
#define LOG_FATAL google::LogMessage(__FILE__, __LINE__, __FUNCTION__, google::GLOG_FATAL).stream()

/**
* 写日志宏
* 写日志用，支持流操作，带traceId参数输入; 如：LOG_INFO("traceId-123") << "string" << 1;
*/
#define LOG_DEBUG_T(traceId) google::LogMessage(__FILE__, __LINE__, __FUNCTION__, google::GLOG_DEBUG, traceId).stream()
#define LOG_INFO_T(traceId)  google::LogMessage(__FILE__, __LINE__, __FUNCTION__, google::GLOG_INFO, traceId).stream()
#define LOG_WARN_T(traceId)  google::LogMessage(__FILE__, __LINE__, __FUNCTION__, google::GLOG_WARNING, traceId).stream()
#define LOG_ERROR_T(traceId) google::LogMessage(__FILE__, __LINE__, __FUNCTION__, google::GLOG_ERROR, traceId).stream()
#define LOG_FATAL_T(traceId) google::LogMessage(__FILE__, __LINE__, __FUNCTION__, google::GLOG_FATAL, traceId).stream()

/************************************************************************/
/* 2、mlog                                                              */
/************************************************************************/
/**
* mlog标准回调函数
* 用于接收mlog的日志回调，设置mlog的回调接收时用
*/
LOG_EXPORT void MlogHappened(const char *file, int line, const char *func, int severity, const char *content);

/**
* mlog标准回调函数，带traceId
* 用于接收mlog的日志回调，设置mlog的回调接收时用
*/
LOG_EXPORT void MlogHappenedTrace(const char *file, int line, const char *func, int severity, const char *content, const char *traceId);

/**
* 以下宏主要适用于glog写入mlog回调出来的日志，记录对应的mlog相关file line func信息
* 使用以下宏时，如果自定义了mlog的回调函数，注意回调函数的参数命名要同MlogHappened标准回调函数保持一致
* 写日志用，支持流操作，如：LOG_INFO_M << "string" << 1;
*/
#define LOG_DEBUG_M google::LogMessage(file, line, func, google::GLOG_DEBUG).stream()
#define LOG_INFO_M	google::LogMessage(file, line, func, google::GLOG_INFO).stream()
#define LOG_WARN_M	google::LogMessage(file, line, func, google::GLOG_WARNING).stream()
#define LOG_ERROR_M google::LogMessage(file, line, func, google::GLOG_ERROR).stream()
#define LOG_FATAL_M google::LogMessage(file, line, func, google::GLOG_FATAL).stream()

#define LOG_DEBUG_MT(traceId) google::LogMessage(file, line, func, google::GLOG_DEBUG, traceId).stream()
#define LOG_INFO_MT(traceId)  google::LogMessage(file, line, func, google::GLOG_INFO, traceId).stream()
#define LOG_WARN_MT(traceId)  google::LogMessage(file, line, func, google::GLOG_WARNING, traceId).stream()
#define LOG_ERROR_MT(traceId) google::LogMessage(file, line, func, google::GLOG_ERROR, traceId).stream()
#define LOG_FATAL_MT(traceId) google::LogMessage(file, line, func, google::GLOG_FATAL, traceId).stream()

/************************************************************************/
/* 3、C语言格式的打印日志                                                 */
/************************************************************************/
/**
* 将format格式转换为string
* 内部使用，不需要调用
*/
GOOGLE_GLOG_DLL_DECL std::string StringPrintf(const char* format, ...);

/**
* 写日志宏
* 写日志用，支持格式化操作，如：LOG_DEBUG_F("debug time is:%I64d, index:%d", time(nullptr), 1);
*/
#define LOG_DEBUG_F(LOG_FORMAT, ...) LOG_DEBUG << StringPrintf(LOG_FORMAT, __VA_ARGS__);
#define LOG_INFO_F(LOG_FORMAT, ...)  LOG_INFO  << StringPrintf(LOG_FORMAT, __VA_ARGS__);
#define LOG_WARN_F(LOG_FORMAT, ...)  LOG_WARN  << StringPrintf(LOG_FORMAT, __VA_ARGS__);
#define LOG_ERROR_F(LOG_FORMAT, ...) LOG_ERROR << StringPrintf(LOG_FORMAT, __VA_ARGS__);
#define LOG_FATAL_F(LOG_FORMAT, ...) LOG_FATAL << StringPrintf(LOG_FORMAT, __VA_ARGS__);

#endif // UTILS_LOGGER_H
