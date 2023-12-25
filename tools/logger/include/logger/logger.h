#pragma once

#include <iostream>
#include <sstream>

#define LOG_DEBUG std::cout << "[" << __FILE__ << "][" << __FUNCTION__ << ":" << __LINE__ << "][-]"
#define LOG_INFO  LOG_DEBUG
#define LOG_WARN  LOG_DEBUG
#define LOG_ERROR LOG_DEBUG
#define LOG_FATAL LOG_DEBUG