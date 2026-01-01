/*
 * 问：什么是日志？
 *
 * 答：日志（Log）是计算机系统中按时间顺序记录的、不可篡改的事件流，本质是系统的“黑匣子”或“审计流水账”。它通过捕捉关键操作、状态变化和错误信息，为系统监控、故障排查和安全分析提供核心依据。
 *
 */
#pragma once
#include <iostream>
#include <string>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

namespace qing {
    /*
     * 这是由Deepseek创作的，用于记录日志的类模块
     * 简化版本，只输出到控制台
     *
     * 优化意见：改为存储日志文件
     */
    class Logger {
        public:
            //获取日志实例（单例模式）
            static Logger& getInstance() {
                static Logger instance;
                return instance;
            }
            
            //初始化日志系统
            void init(LogLevel minLogLevel = LogLevel::INFO,
                    bool consoleOutput = true) {
                std::lock_guard<std::mutex> lock(mutex_);

                if (initialized_) {
                    return;
                }

                minLogLevel_ = minLogLevel;
                consoleOutput_ = consoleOutput;
                initialized_ = true;
            }

            // 设置最低日志级别
            void setMinLogLevel(LogLevel level) {
                std::lock_guard<std::mutex> lock(mutex_);
                minLogLevel_ = level;
            }

            // 日志输出方法
            void log(LogLevel level, const std::string& message) {
                if (level < minLogLevel_ || !consoleOutput_) {
                    return;
                }

                auto now = std::chrono::system_clock::now();
                auto in_time_t = std::chrono::system_clock::to_time_t(now);
                auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch()) % 1000;

                std::tm tm;
                localtime_r(&in_time_t, &tm);

                std::stringstream ss;
                ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
                   << "." << std::setfill('0') << std::setw(3) << ms.count()
                   << " [" << logLevelToString(level) << "] "
                   << message;

                std::lock_guard<std::mutex> lock(mutex_);
                std::cout << ss.str() << std::endl;
            }

            Logger(const Logger&) = delete;
            Logger& operator=(const Logger&) = delete;

        private:
            Logger() = default;
            ~Logger() = default;

            std::string logLevelToString(LogLevel level) {
                switch (level) {
                    case LogLevel::DEBUG:    return "DEBUG";
                    case LogLevel::INFO:     return "INFO";
                    case LogLevel::WARNING:  return "WARNING";
                    case LogLevel::ERROR:    return "ERROR";
                    case LogLevel::CRITICAL: return "CRITICAL";
                    default:                return "UNKNOWN";
                }
            }

            std::mutex mutex_;
            LogLevel minLogLevel_ = LogLevel::INFO;
            bool consoleOutput_ = true;
            bool initialized_ = false;
    };
}

#define LOG_DEBUG(message) qing::Logger::getInstance().log(LogLevel::DEBUG, message)
#define LOG_INFO(message) qing::Logger::getInstance().log(LogLevel::INFO, message)
#define LOG_WARNING(message) qing::Logger::getInstance().log(LogLevel::WARNING, message)
#define LOG_ERROR(message) qing::Logger::getInstance().log(LogLevel::ERROR, message)
#define LOG_CRITICAL(message) qing::Logger::getInstance().log(LogLevel::CRITICAL, message)
