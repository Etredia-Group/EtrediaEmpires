/**
 * Global logging system based on P7 library and made compatible with priviously used LOG macro.
 * It should be redesigned to support both global and instance regimes without littering of global namespace.
 */

module;

#include <P7_Trace.h>
#undef ERROR
#undef TM

export module utils:logger;
import std;

namespace logger {
    /**
     * Represents a wide-character string literal constructed from a narrow-character string literal.
     * @tparam length number of characters in the string literal (including the null terminator).
     */
    template <std::size_t length>
    struct wstring {
        wchar_t content[length];
        // ReSharper disable once CppNonExplicitConversionOperator
        constexpr operator const wchar_t*() const { return content; }

        explicit constexpr wstring(const char (&str)[length]) : content{} {
            for (std::size_t i = 0; i < length; i++)
                content[i] = static_cast<wchar_t>(str[i]);
        }
    };

    /**
     * Expression that converts a narrow-character string literal to a wide-character string literal at compile time.
     * @tparam length number of symbols in string.
     * @param str narrow-character string literal to convert.
     * @return wide-character string literal.
     */
    template <std::size_t length>
    constexpr wstring<length> TM(const char (&str)[length]) {
        return wstring<length>(str);
    }

    /**
     * Represents the severity level of a log message.
     */
    enum LogLevel : std::uint8_t {
        TRACE,
        DEBUG,
        INFO,
        WARNING,
        ERROR,
        CRITICAL,
    };

    /**
     * Global singleton used to configure logging with logger::LOG and logger::LOG_WCHAR.
     */
    class Logger {
    public:
        /**
         * Singleton cannot be instantiated.
         */
        Logger() = delete;

        /**
         * Initializes global state of P7 logger.
         * @note Use logger::Logger::Release() when you are done or need different configuration.
         * @param options commands line for logger. See possible options in P7Log documentation.
         * @param location used for multiple initialization warning details.
         */
        static void Init(const wchar_t *options,
                         const std::source_location& location = std::source_location::current()) {
            if (isReady) {
                trace->Trace(
                    0,
                    static_cast<eP7Trace_Level>(WARNING),
                    nullptr,
                    location.line(),
                    location.file_name(),
                    location.function_name(),
                    L"Logger is alredy initialized"
                );
                return;
            }

            P7_Set_Crash_Handler();
            client = P7_Create_Client(options);
            P7_Set_Crash_Handler();
            trace = P7_Create_Trace(client, L"TC 1");

            isReady = true;
        }

        /**
         * Releases P7 COM-objects if Logger is initialized.
         */
        static void Release() {
            if (!isReady) return;
            trace->Release();
            client->Release();
        }

        /**
         * Specifies name for current thread allowing to have thread name in messages in addition to thread id.
         * @note Use logger::Logger::UnregisterThread when you are done.
         * @warning Not recommended to use for thoudends of threads.
         * All threads info will be stored in RAM and may consume CPU to organize them in RB tree.
         * @param name thread name
         */
        static void RegisterThread(const wchar_t *name) {
            trace->Register_Thread(name, 0);
        }

        static void UnregisterThread() {
            trace->Unregister_Thread(0);
        }

    private:
        inline static IP7_Client *client = nullptr;
        inline static IP7_Trace *trace = nullptr;

        inline static bool isReady = false;

        // Allows to use logger::LOG_WCHAR instead of logger::Logger::LOG_WCHAR for compatibility.
        template <typename... Args> friend struct LOG_WCHAR;
        // Allows to use logger::LOG instead of logger::Logger::LOG for compatibility.
        template <std::size_t length, typename... Args> friend struct LOG;
    };

    /**
     * Function-like utility type for wide-character formatted logging.
     *
     * This type is intended to be used as a temporary object.
     * Logging is performed in the constructor, and the object itself
     * is not meant to be stored.
     *
     * Template argument deduction guides allow usage without explicitly
     * specifying template parameters.
     *
     * @tparam Args types of the formatting arguments.
     */
    template <typename... Args>
    struct LOG_WCHAR {
        /**
         * Logs a formatted wide-character message.
         * @param level severity level of the log message.
         * @param format wide-character format string literal.
         * @param args formatting arguments corresponding to the format specifiers in the format string.
         * @param location source location info. Can be specified to report a different call site.
         */
        LOG_WCHAR(
            LogLevel level,
            const wchar_t *format,
            Args&&... args,
            const std::source_location& location = std::source_location::current()
        ) {
            Logger::trace->Trace(
                0,
                static_cast<eP7Trace_Level>(level),
                nullptr,
                location.line(),
                location.file_name(),
                "", //TODO: location.function_name(),
                format,
                std::forward<Args>(args)...
            );

            if (level == ERROR || level == CRITICAL) {
                Logger::client->Flush();
            }
        }
    };

    /**
     * Function-like utility type for ASCII-only formatted logging.
     * @note Non-ASCII characters may be converted incorrectly due to simple
     * byte-to-wchar_t casting. Use logger::LOG_WCHAR for non-ASCII or Unicode literals.
     *
     * This type is intended to be used as a temporary object.
     * Logging is performed in the constructor, and the object itself
     * is not meant to be stored.
     *
     * Template argument deduction guides allow usage without explicitly
     * specifying template parameters.
     *
     * @tparam length number of characters in the format string literal (including the null terminator).
     * @tparam Args types of the formatting arguments.
     */
    template <std::size_t length, typename... Args>
    struct LOG {
        /**
         * Logs a formatted narrow-character message.
         * @param level severity level of the log message.
         * @param format narrow-character format string literal.
         * @param args formatting arguments corresponding to the format specifiers in the format string.
         * @param location source location info. Can be specified to report a different call site.
         */
        LOG(
            LogLevel level,
            const char (&format)[length],
            Args&&... args,
            const std::source_location& location = std::source_location::current()
        ) {
            Logger::trace->Trace(
                0,
                static_cast<eP7Trace_Level>(level),
                nullptr,
                location.line(),
                location.file_name(),
                "", //TODO: location.function_name(),
                TM(format),
                std::forward<Args>(args)...
            );

            if (level == ERROR || level == CRITICAL) {
                Logger::client->Flush();
            }
        }
    };

    /**
     * Deduction guide for LOG_WCHAR.
     *
     * Enables deduction of the formatting argument types,
     * allowing usage without explicit template arguments.
     */
    template <typename... Args>
    LOG_WCHAR(LogLevel, const wchar_t *, Args&&...) -> LOG_WCHAR<Args...>;
    /**
     * Deduction guide for LOG.
     *
     * Enables deduction of the string literal length and formatting
     * argument types, allowing usage without explicit template arguments.
     */
    template <std::size_t length, typename... Args>
    LOG(LogLevel, const char (&)[length], Args&&...) -> LOG<length, Args...>;
}

/**
 * Export to global namespace for compatibility
 */
export using logger::Logger;
export using logger::LOG_WCHAR;
export using logger::LOG;
export using logger::LogLevel::TRACE;
export using logger::LogLevel::DEBUG;
export using logger::LogLevel::INFO;
export using logger::LogLevel::WARNING;
export using logger::LogLevel::ERROR;
export using logger::LogLevel::CRITICAL;
