#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>

// 日志级别枚举
enum LogLevel {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
};

// 当前日志级别，可通过LOG_SET_LEVEL宏修改
static LogLevel current_log_level = LOG_INFO;

// 日志文件指针
static FILE* log_file = NULL;

// 互斥锁，保证多线程安全
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

// 是否同时输出到控制台
static int log_to_console = 1;

// 获取日志级别的字符串表示
const char* log_level_to_string(LogLevel level) {
    switch(level) {
        case LOG_DEBUG: return "[DEBUG]";
        case LOG_INFO: return "[INFO]";
        case LOG_WARNING: return "[WARNING]";
        case LOG_ERROR: return "[ERROR]";
        default: return "[UNKNOWN]";
    }
}

// 获取格式化的时间戳
void get_timestamp(char* buffer, size_t size) {
    time_t now;
    time(&now);
    struct tm* timeinfo = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", timeinfo);
}

// 初始化日志文件
int log_init_file(const char* filename, int console_output) {
    pthread_mutex_lock(&log_mutex);
    
    // 关闭已打开的日志文件
    if (log_file != NULL) {
        fclose(log_file);
        log_file = NULL;
    }
    
    // 打开日志文件（追加模式）
    log_file = fopen(filename, "a");
    if (log_file == NULL) {
        fprintf(stderr, "Failed to open log file '%s': %s\n", filename, strerror(errno));
        pthread_mutex_unlock(&log_mutex);
        return -1;
    }
    
    // 设置是否同时输出到控制台
    log_to_console = console_output;
    
    pthread_mutex_unlock(&log_mutex);
    return 0;
}

// 关闭日志文件
void log_close_file() {
    pthread_mutex_lock(&log_mutex);
    
    if (log_file != NULL) {
        fclose(log_file);
        log_file = NULL;
    }
    
    pthread_mutex_unlock(&log_mutex);
}

// 日志输出函数
void log_message(LogLevel level, const char* format, ...) {
    // 检查日志级别
    if (level < current_log_level) {
        return;
    }
    
    // 获取时间戳
    char timestamp[32];
    get_timestamp(timestamp, sizeof(timestamp));
    
    // 创建完整的日志消息
    char log_buffer[1024];
    int pos = 0;
    
    // 构建日志头
    pos += snprintf(log_buffer + pos, sizeof(log_buffer) - pos, "%s %s ", 
                    timestamp, log_level_to_string(level));
    
    // 构建日志内容
    va_list args;
    va_start(args, format);
    pos += vsnprintf(log_buffer + pos, sizeof(log_buffer) - pos, format, args);
    va_end(args);
    
    // 添加换行
    snprintf(log_buffer + pos, sizeof(log_buffer) - pos, "\n");
    
    // 加锁保证线程安全
    pthread_mutex_lock(&log_mutex);
    
    // 输出到控制台
    if (log_to_console) {
        printf("%s", log_buffer);
        fflush(stdout);
    }
    
    // 输出到文件
    if (log_file != NULL) {
        fprintf(log_file, "%s", log_buffer);
        fflush(log_file);  // 立即刷新到文件
    }
    
    pthread_mutex_unlock(&log_mutex);
}

// 宏定义，便于使用
#define LOG_DEBUG(format, ...) log_message(LOG_DEBUG, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...) log_message(LOG_INFO, format, ##__VA_ARGS__)
#define LOG_WARNING(format, ...) log_message(LOG_WARNING, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) log_message(LOG_ERROR, format, ##__VA_ARGS__)

// 设置日志级别
#define LOG_SET_LEVEL(level) current_log_level = level

// 初始化日志文件的宏
#define LOG_INIT_FILE(filename, console_output) log_init_file(filename, console_output)

// 关闭日志文件的宏
#define LOG_CLOSE_FILE() log_close_file()

#endif // LOGGER_H