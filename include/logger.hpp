#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>
#include <string>

class Logger {
  public:
    enum class Level { DEBUG = 0, INFO, WARNING, ERROR, CRITICAL, NLEVELS };

  private:
    Level mSeverity = Level::WARNING;

    template<typename... Args>
    void log(Level severity, Args&&... args) {
      if (severity >= mSeverity) {
        auto& os = (severity >= Level::WARNING) ? std::cerr : std::cout;
        (os << ... << args);
        os << std::endl;
      }
    }

  public:
    Logger() = default;
    ~Logger() = default;

    template<typename... Args>
    void logCritical(Args&&... args) { log(Level::CRITICAL, std::forward<Args>(args)...); }
    template<typename... Args>
    void logError(Args&&... args) { log(Level::ERROR, std::forward<Args>(args)...); }
    template<typename... Args>
    void logWarning(Args&&... args) { log(Level::WARNING, std::forward<Args>(args)...); }
    template<typename... Args>
    void logInfo(Args&&... args) { log(Level::INFO, std::forward<Args>(args)...); }
    template<typename... Args>
    void logDebug(Args&&... args) { log(Level::DEBUG, std::forward<Args>(args)...); }

    void setSeverity(const Level severity) { mSeverity = severity; }
    void increaseSeverity(int incr = 1) {
      int value = static_cast<int>(mSeverity) + incr;
      if (value >= static_cast<int>(Level::NLEVELS)) {
        setSeverity(Level::CRITICAL);
      }
      else {
        setSeverity(Level{value});
      }
    }
    void decreaseSeverity(int incr = 1) {
      int value = static_cast<int>(mSeverity) - incr;
      if (value < 0) {
        setSeverity(Level::DEBUG);
      }
      else {
        setSeverity(Level{value});
      }
    }
    std::string getSeverity() {
      switch(mSeverity) {
        case Level::DEBUG :
          return "DEBUG";
          break;
        case Level::INFO :
          return "INFO";
          break;
        case Level::WARNING :
          return "WARNING";
          break;
        case Level::ERROR :
          return "ERROR";
          break;
        case Level::CRITICAL :
          return "CRITICAL";
          break;
        default:
          logCritical("Logging level is at a strange value");
      }
    }
};

inline Logger LogInstance;

template<typename... Args>
void logCritical(Args&&... args) { LogInstance.logCritical(std::forward<Args>(args)...); }
template<typename... Args>
void logError(Args&&... args) { LogInstance.logError(std::forward<Args>(args)...); }
template<typename... Args>
void logWarning(Args&&... args) { LogInstance.logWarning(std::forward<Args>(args)...); }
template<typename... Args>
void logInfo(Args&&... args) { LogInstance.logInfo(std::forward<Args>(args)...); }
template<typename... Args>
void logDebug(Args&&... args) { LogInstance.logDebug(std::forward<Args>(args)...); }

inline void setSeverity(Logger::Level severity) { LogInstance.setSeverity(severity); }
inline void increaseSeverity(int incr = 1) { LogInstance.increaseSeverity(incr); }
inline void decreaseSeverity(int incr = 1) { LogInstance.decreaseSeverity(incr); }
inline std::string getSeverity() { return LogInstance.getSeverity(); }

#endif
