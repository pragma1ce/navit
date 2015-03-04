#ifndef LOG_H
#define LOG_H

#include <spdlog/spdlog.h>

#define nTrace() spdlog::get("nxe_logger")->trace() << __FILENAME__ << "@" << __LINE__ << " "
#define nDebug() spdlog::get("nxe_logger")->debug() << __FILENAME__ << "@" << __LINE__ << " "
#define nInfo() spdlog::get("nxe_logger")->info() << __FILENAME__ << "@" << __LINE__ << " "
#define nError() spdlog::get("nxe_logger")->error()
#define nFatal() spdlog::get("nxe_logger")->critical()

#endif // LOG_H

