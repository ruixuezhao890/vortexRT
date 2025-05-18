/**
  ******************************************************************************
  * @file           : log.hpp
  * @author         : ruixuezhao
  * @brief          : None
  * @attention      : None
  * @date           : 25-5-16
  ******************************************************************************
  */


#ifndef LOG_H
#define LOG_H

#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <array>
#include <algorithm>
#include <vortexRT.h>

namespace vx_log {
    /**
    Trace:Trace是最低级别，用于详细追踪程序流程，如方法调用和分支执行。适用于调试复杂逻辑，但生产环境通常禁用，因为日志量太大。\n
    Debug：用于开发阶段的详细调试信息，如函数调用和变量值。生产环境一般不开启，但内测版可能允许。\n
    Info：Info用于记录关键事件，如服务启动和配置加载，帮助确认系统正常运行，适合日常运维监控。\n
    Warning：Warning用于潜在问题，如资源不足或配置错误，不影响系统但需关注，用于预防问题恶化。\n
    Error：Error记录可恢复的错误，如API调用失败，需包含上下文信息以便排查，但系统仍能运行。\n
    Critical：Critical用于不可逆错误，如内存耗尽，导致系统崩溃，需立即处理。\n
    Always：用户自定义级别，强制输出，可能用于关键信息，如用户登录，确保记录。
    */
    enum class Level : uint8_t {
        Trace = 100,
        Debug,
        Info,
        Warning,
        Error,
        Critical,
        Always
    };

    enum class Error {
        None = 0,
        SubscribersExceeded,
        NotSubscribed
    };

    constexpr size_t MAX_SUBSCRIBERS = 6;
    constexpr size_t MAX_MESSAGE_LENGTH = 1024;

    using SubscriberCallback = void(*)(const char *message);
    using AssertHandler = void(*)(const char *file, int line, const char *func, const char *expr);

    class Logger {
    public:
        static Logger &instance() {
            static Logger instance;
            return instance;
        }

        void init() {
            subscribers_.fill({nullptr, Level::Always});
            update_lowest_level();
        }

        Error subscribe(SubscriberCallback fn, Level threshold) {
            auto it = std::find_if(subscribers_.begin(), subscribers_.end(),
                                   [fn](const auto &sub) { return sub.callback == fn; });

            if (it != subscribers_.end()) {
                it->threshold = threshold;
                update_lowest_level();
                return Error::None;
            }

            const auto empty = std::find_if(subscribers_.begin(), subscribers_.end(),
                                            [](const auto &sub) { return sub.callback == nullptr; });

            if (empty == subscribers_.end()) {
                return Error::SubscribersExceeded;
            }

            *empty = {fn, threshold};
            update_lowest_level();
            return Error::None;
        }

        Error unsubscribe(SubscriberCallback fn) {
            auto it = std::find_if(subscribers_.begin(), subscribers_.end(),
                                   [fn](const auto &sub) { return sub.callback == fn; });

            if (it == subscribers_.end()) {
                return Error::NotSubscribed;
            }

            it->callback = nullptr;
            update_lowest_level();
            return Error::None;
        }

        const char *level_name(Level level) const {
            switch (level) {
                case Level::Trace: return "TRACE";
                case Level::Debug: return "DEBUG";
                case Level::Info: return "INFO";
                case Level::Warning: return "WARNING";
                case Level::Error: return "ERROR";
                case Level::Critical: return "CRITICAL";
                case Level::Always: return "ALWAYS";
                default: return "UNKNOWN";
            }
        }

        void message(Level severity, int line, const char *file, const char *func_name, const char *format, ...) {
            if (static_cast<int>(severity) < static_cast<int>(lowest_level_)) {
                return;
            }

            // 添加颜色控制代码
            auto color_code = "";
            auto reset_code = "\033[0m";
            switch (severity) {
                case Level::Trace: color_code = "\033[0;37m";
                    break; // 灰色
                case Level::Debug: color_code = "\033[36m";
                    break; // 青色
                case Level::Info: color_code = "\033[32m";
                    break; // 绿色
                case Level::Warning: color_code = "\033[33m";
                    break; // 黄色
                case Level::Error: color_code = "\033[31m";
                    break; // 红色
                case Level::Critical: color_code = "\033[35m";
                    break; // 紫色
                case Level::Always: color_code = "\033[34m";
                    break; // 蓝色
                default: color_code = "";
            }
            char header[MAX_MESSAGE_LENGTH]{0};
            if (severity == Level::Debug || severity == Level::Critical || severity == Level::Error) {
                // 新增调试信息格式化
                snprintf(header, sizeof(header), "%s[%s][file_name:%s line:%d][func_name:%s] message:", color_code,
                         level_name(severity), file, line, func_name);
            } else {
                snprintf(header, sizeof(header), "%s[%s] message:", color_code, level_name(severity));
            }
            va_list args;
            va_start(args, format);
            vsnprintf(buffer_.data(), buffer_.size(), format, args);
            va_end(args);

            // 合并头部和消息内容
            strncat(header, buffer_.data(), sizeof(header) - strlen(header) - 1);
            strncpy(buffer_.data(), header, buffer_.size());

            // 在消息末尾添加颜色重置代码
            strncat(buffer_.data(), reset_code, buffer_.size() - strlen(buffer_.data()) - 1);


            for (const auto &sub: subscribers_) {
                if (sub.callback && severity >= sub.threshold) {
                    sub.callback(buffer_.data());
                }
            }
        }

        // 新增断言核心逻辑
        void assert_fail(const char *file, int line, const char *func, const char *expr, const char *format = nullptr,
                         ...) {
            // 构造断言信息
            char message[MAX_MESSAGE_LENGTH]{0};
            if (format) {
                va_list args;
                va_start(args, format);
                vsnprintf(message, sizeof(message), format, args);
                va_end(args);
            }

            // 记录日志
            if (static_cast<int>(Level::Critical) >= static_cast<int>(lowest_level_)) {
                this->message(Level::Critical, line, file, func, message);

                while (true);
            }
        }

    private:
        struct Subscriber {
            SubscriberCallback callback;
            Level threshold;
        };

        std::array<Subscriber, MAX_SUBSCRIBERS> subscribers_{};
        std::array<char, MAX_MESSAGE_LENGTH> buffer_{};
        Level lowest_level_ = Level::Always;

        void update_lowest_level() {
            auto min_level = Level::Always;
            for (const auto &sub: subscribers_) {
                if (sub.callback && sub.threshold < min_level) {
                    min_level = sub.threshold;
                }
            }
            lowest_level_ = min_level;
        }
    };

    // 宏定义保留类似C接口的使用方式
#ifdef vortexRT_DEBUG_ENABLE
#define filename(x) strrchr(x,'\\')?strrchr(x,'\\')+1:x
#define VX_LOG_INIT()           vx_log::Logger::instance().init()
#define VX_LOG_SUBSCRIBE(f, t)  vx_log::Logger::instance().subscribe(f, t)
#define VX_LOG_UNSUBSCRIBE(f)   vx_log::Logger::instance().unsubscribe(f)
#define VX_LOG_LEVEL_NAME(l)   vx_log::Logger::instance().level_name(l)

#define VX_LOG(level, line, file, func, ...)         vx_log::Logger::instance().message(level, line, file, func, __VA_ARGS__)
#define VX_LOG_TRACE(...)      VX_LOG(vx_log::Level::Trace,__LINE__,filename(__FILE__),__func__,__VA_ARGS__)
#define VX_LOG_DEBUG(...)      VX_LOG(vx_log::Level::Debug,__LINE__,filename(__FILE__),__func__,__VA_ARGS__)
#define VX_LOG_INFO(...)       VX_LOG(vx_log::Level::Info, __LINE__,filename(__FILE__),__func__,__VA_ARGS__)
#define VX_LOG_WARNING(...)    VX_LOG(vx_log::Level::Warning, __LINE__,filename(__FILE__),__func__,__VA_ARGS__)
#define VX_LOG_ERROR(...)      VX_LOG(vx_log::Level::Error, __LINE__,filename(__FILE__),__func__,__VA_ARGS__)
#define VX_LOG_CRITICAL(...)   VX_LOG(vx_log::Level::Critical, __LINE__,filename(__FILE__),__func__,__VA_ARGS__)
#define VX_LOG_ALWAYS(...)     VX_LOG(vx_log::Level::Always, __LINE__,filename(__FILE__),__func__,__VA_ARGS__)
#define VX_ASSERT(expr, ...) \
    do { \
        if (!(expr)) { \
        vx_log::Logger::instance().assert_fail(filename(__FILE__), __LINE__,__ASSERT_FUNC, ##__VA_ARGS__,__VA_ARGS__); \
        } \
    } while(0)
#else
    #define VX_LOG_INIT()          do {} while(0)
    #define VX_LOG_SUBSCRIBE(f, t) do {} while(0)
    #define VX_LOG_UNSUBSCRIBE(f)  do {} while(0)
    #define VX_LOG_LEVEL_NAME(l)  ""

    #define VX_LOG(s, ...)         do {} while(0)
    #define VX_LOG_TRACE(...)      do {} while(0)
    #define VX_LOG_DEBUG(...)      do {} while(0)
    #define VX_LOG_INFO(...)       do {} while(0)
    #define VX_LOG_WARNING(...)    do {} while(0)
    #define VX_LOG_ERROR(...)      do {} while(0)
    #define VX_LOG_CRITICAL(...)   do {} while(0)
    #define VX_LOG_ALWAYS(...)     do {} while(0)
    #define VX_ASSERT(expr, ...)   do {} while(0)
#endif
} // namespace vx_log

#endif
