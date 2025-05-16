#pragma once

#include <vector>

enum class LogLevel {
  Debug = 0,
  Verbose,
  Info,
  Warning,
  Error,
  None,
};

void LogDebug(const char* category, const char* format, ...);
void LogVerbose(const char* category, const char* format, ...);
void LogInfo(const char* category, const char* format, ...);
void LogWarning(const char* category, const char* format, ...);
void LogError(const char* category, const char* format, ...);

class ILogger {
public:
  static void Set(ILogger* logger);
  static ILogger* Get();

  virtual ~ILogger() = default;
  virtual void setLevel(LogLevel level) = 0;
  virtual void logMessage(LogLevel level, const char* category, const char* format, ...);
  virtual void logMessage(LogLevel level, const char* category, const char* format, va_list args) = 0;
protected:
  static const char* getLevelName(LogLevel level);
  static ILogger* _globalLogger;
};

class PrintfLogger : public ILogger {
public:
  virtual ~PrintfLogger() = default;
  void setLevel(LogLevel level) override;
  void logMessage(LogLevel level, const char* category, const char* format, va_list args) override;
protected:
private:
  LogLevel _level = LogLevel::Verbose;
};
