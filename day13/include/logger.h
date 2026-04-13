#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <sstream>
#include <mutex>
#include <memory>
#include <chrono>
#include <ctime>
#include <vector>
#include <iostream>
#include <thread>
#include <iomanip>

class Logger;

enum class LogLevel { DEBUG = 0, INFO = 1, WARN = 2, ERROR = 3 };

inline const char* levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO ";
        case LogLevel::WARN:  return "WARN ";
        case LogLevel::ERROR: return "ERROR";
        default:              return "UNKNOWN";
    }
}

struct LogEvent {
    LogLevel      level;
    std::string   filename;
    int           line;
    std::string   message;
    std::time_t   time;
    uint32_t      thread_id;

    LogEvent(LogLevel lvl, const char* file, int line_num)
        : level(lvl), filename(file), line(line_num),
          time(std::time(nullptr)),
          thread_id(static_cast<uint32_t>(
              std::hash<std::thread::id>{}(std::this_thread::get_id())))
    {}
};

class LogSink {
public:
    virtual ~LogSink() = default;
    virtual void write(const LogEvent& event) = 0;
    virtual void flush() = 0;
};

class ConsoleSink : public LogSink {
public:
    void write(const LogEvent& event) override {
        auto& stream = (event.level >= LogLevel::ERROR) ? std::cerr : std::cout;
        std::tm local_tm;
        localtime_r(&event.time, &local_tm);
        stream << "[" << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S") << "] "
               << "[" << levelToString(event.level) << "] "
               << "[" << event.filename << ":" << event.line << "] "
               << event.message << std::endl;
    }
    void flush() override {
        std::cout.flush();
        std::cerr.flush();
    }
};

class FileSink : public LogSink {
public:
    explicit FileSink(const std::string& filename) : filename_(filename) {
        file_.open(filename, std::ios::out | std::ios::app);
        if (!file_.is_open()) {
            std::cerr << "[Logger] cannot open log file: " << filename << std::endl;
        }
    }
    ~FileSink() override { if (file_.is_open()) file_.close(); }

    void write(const LogEvent& event) override {
        if (!file_.is_open()) return;
        std::tm local_tm;
        localtime_r(&event.time, &local_tm);
        file_ << "[" << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S") << "] "
              << "[" << levelToString(event.level) << "] "
              << "[" << event.filename << ":" << event.line << "] "
              << event.message << "\n";
        file_.flush();
    }
    void flush() override { if (file_.is_open()) file_.flush(); }

private:
    std::string  filename_;
    std::ofstream file_;
};

class LogStream {
public:
    LogStream(LogLevel level, const char* file, int line)
        : event_(level, file, line) {}

    ~LogStream();
    template<typename T>
    LogStream& operator<<(const T& val) {
        ss_ << val;
        return *this;
    }

private:
    LogEvent         event_;
    std::stringstream ss_;
};

class Logger {
public:
    static Logger& instance() {
        static Logger logger;
        return logger;
    }
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void setLogLevel(LogLevel level) { level_ = level; }
    void addSink(std::shared_ptr<LogSink> sink) { sinks_.push_back(sink); }

    void log(const LogEvent& event) {
    if (event.level < level_) return;

    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& sink : sinks_) {
        sink->write(event);
    }
}

private:
    Logger() : level_(LogLevel::DEBUG) {
        sinks_.push_back(std::make_shared<ConsoleSink>());
    }
    LogLevel                              level_;
    std::vector<std::shared_ptr<LogSink>>  sinks_;
    std::mutex                            mutex_;
};

// ===================== 宏定义 =====================
#define LOG(level) LogStream(LogLevel::level, __FILE__, __LINE__)

#define LOG_DEBUG LOG(DEBUG)
#define LOG_INFO  LOG(INFO)
#define LOG_WARN  LOG(WARN)
#define LOG_ERROR LOG(ERROR)

// ===================== 初始化辅助函数 =====================
inline void initLogger(LogLevel level = LogLevel::DEBUG,
                       const std::string& logfile = "") {
    auto& logger = Logger::instance();
    logger.setLogLevel(level);

    if (!logfile.empty()) {
        logger.addSink(std::make_shared<FileSink>(logfile));
    }
}

#endif // LOGGER_H
