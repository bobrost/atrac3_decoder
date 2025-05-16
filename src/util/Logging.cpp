#include "Logging.h"
#include <cstdarg>
#include <stdio.h>

#define IMPLEMENT_LOG_FN(FnName, Level) \
  void FnName(const char* category, const char* format, ...) { \
    if (ILogger* logger = ILogger::Get()) { \
      va_list args; \
      va_start(args, format); \
      logger->LogMessage(Level, category, format, args); \
      va_end(args); \
    } \
  }

IMPLEMENT_LOG_FN(LogDebug, LogLevel::Debug)
IMPLEMENT_LOG_FN(LogVerbose, LogLevel::Verbose)
IMPLEMENT_LOG_FN(LogInfo, LogLevel::Info)
IMPLEMENT_LOG_FN(LogWarning, LogLevel::Warning)
IMPLEMENT_LOG_FN(LogError, LogLevel::Error)

ILogger* ILogger::_globalLogger = nullptr;

void ILogger::Set(ILogger* logger) {
  _globalLogger = logger;
}
ILogger* ILogger::Get() {
  return _globalLogger;
}

const char* ILogger::GetLevelName(LogLevel level) {
  switch(level) {
    case LogLevel::Verbose:
      return "Verbose";
    case LogLevel::Info:
      return "Info";
    case LogLevel::Warning:
      return "Warning";
    case LogLevel::Error:
      return "Error";
    default: break;
  }
  return "";
}

void PrintfLogger::SetLevel(LogLevel level) {
  _level = level;
}

void ILogger::LogMessage(LogLevel level, const char* category, const char* format, ...) {
  va_list args;
  va_start(args, format);
  LogMessage(level, category, format, args);
  va_end(args);
}

void PrintfLogger::LogMessage(LogLevel level, const char* category, const char* format, va_list args) {
  if (level < _level) {
    return;
  }
  printf("[%s] [%s] ", GetLevelName(level), category);
  vprintf(format, args);
  printf("\n");
}
