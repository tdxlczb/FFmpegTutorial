#ifndef __HH_LOGGER_OLD_HH__
#define __HH_LOGGER_OLD_HH__

#include "../glog/logger.h"

//--------------------------------兼容旧格式------------------------------------------//
#define GLDEBUG LOG_DEBUG
#define GSDEBUG LOG_DEBUG
#define GLINFO  LOG_INFO
#define GSINFO  LOG_INFO
#define GLWARN  LOG_WARN
#define GSWARN  LOG_WARN
#define GLERROR LOG_ERROR
#define GSERROR LOG_ERROR
#define GLFATAL LOG_FATAL
#define GSFATAL LOG_FATAL
//--------------------------------兼容旧格式------------------------------------------//

#endif