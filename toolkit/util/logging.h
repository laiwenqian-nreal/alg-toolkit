#pragma once

#include <framework/util/log.h>
#include <framework/util/singleton.h>

namespace xreal {
namespace toolkits {
namespace utils {

class Logger : public framework::util::Singleton<Logger> {
public:
  bool IsLogAllLevel() const { return log_all_level_; }
  bool IsLogDebugLevel() const { return log_debug_level_; }

  void SetLogAllLevel(bool value) {
    log_all_level_ = value;
    if (log_all_level_) {
      GetLogger()->set_level(framework::util::log::LogLevel::trace);
      GetLogger()->flush_on(framework::util::log::LogLevel::trace);
    }
  }

  void SetLogLevel(int value) {
    log_all_level_ = (value >= 2);
    log_debug_level_ = (value >= 1);
    if (log_all_level_) {
      // framework default is info
      GetLogger()->set_level(framework::util::log::LogLevel::trace);
      GetLogger()->flush_on(framework::util::log::LogLevel::trace);
    } else if (log_debug_level_) {
      GetLogger()->set_level(framework::util::log::LogLevel::debug);
      GetLogger()->flush_on(framework::util::log::LogLevel::debug);
    }
  }

  framework::util::log::LoggerPtr GetLogger() {
    return framework::util::log::Logger::defaultLogger(section_);
  }
  void SetSection(const std::string &section) { section_ = section; }

private:
  bool log_all_level_{false};
  bool log_debug_level_{false};
  std::string section_{"NRAlgToolkits"};
};

//#define DLOG_TRACE(format, ...)
#define DLOG_TRACE2(format1, format2, ...)                                     \
  {                                                                            \
    std::string format = format1;                                              \
    format += format2;                                                         \
    Logger::GetInstance()->GetLogger()->trace(format.c_str(), ##__VA_ARGS__);  \
  }
#define DLOG_TRACE(format, ...)                                                \
  if (Logger::GetInstance()->IsLogAllLevel())                                  \
    Logger::GetInstance()->GetLogger()->trace(format, ##__VA_ARGS__);
#define DLOG_DEBUG(format, ...)                                                \
  if (Logger::GetInstance()->IsLogDebugLevel())                                \
    Logger::GetInstance()->GetLogger()->debug(format, ##__VA_ARGS__);

#define DLOG_ASSERT(condition, format, ...)                                    \
  Logger::GetInstance()->GetLogger()->runtime_assert(condition, format,        \
                                                     ##__VA_ARGS__);

//#define DLOG_DEBUG(format, ...)
// Logger::GetInstance()->GetLogger()->debug(format, ##__VA_ARGS__);
#define DLOG_INFO(format, ...)                                                 \
  Logger::GetInstance()->GetLogger()->info(format, ##__VA_ARGS__);
#define DLOG_WARN(format, ...)                                                 \
  Logger::GetInstance()->GetLogger()->warn(format, ##__VA_ARGS__);
#define DLOG_ERROR(format, ...)                                                \
  Logger::GetInstance()->GetLogger()->error(format, ##__VA_ARGS__);
#define DLOG_FATAL(format, ...)                                                \
  Logger::GetInstance()->GetLogger()->fatal(format, ##__VA_ARGS__);

} // namespace utils
} // namespace toolkits
} // namespace xreal
