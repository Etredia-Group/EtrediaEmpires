/**
 * Global logging system based on P7 library and made compatible with priviously used LOG macro.
 * It should be redesigned to support both global and instance regimes without littering of global namespace.
 */

module;

#include <cstdio>
#include <P7_Trace.h>
#undef ERROR
#undef TM

export module utils:logger;
import std;

namespace logger::detail {


    constexpr bool is_ident(const char c) {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') ||
               (c >= 'A' && c <= 'Z') || c == '_' || c == '~';
    }

    constexpr std::string_view trim_right(std::string_view s) {
        while (!s.empty() && s.back() == ' ') s.remove_suffix(1);
        return s;
    }

    constexpr std::string_view extract_function_name(std::string_view full) {
        // 1. Find '(' of parameters list, skipping "operator()" / "operator ()".
        std::size_t paren = full.size();
        for (std::size_t i = 0; i < full.size(); ++i) {
            if (full[i] != '(') continue;
            if (trim_right(full.substr(0, i)).ends_with("operator")) {
                std::size_t j = full.find(')', i);
                if (j == std::string_view::npos) break;
                i = j;
                continue;
            }
            paren = i;
            break;
        }

        // 2. Everything before '(': "void __cdecl ns::A::foo<int>"
        auto head = trim_right(full.substr(0, paren));

        // 3. Cut template params <...> in the end.
        if (!head.empty() && head.back() == '>') {
            int depth = 0;
            for (std::size_t i = head.size(); i-- > 0;) {
                if (head[i] == '>') ++depth;
                else if (head[i] == '<' && --depth == 0) {
                    head = trim_right(head.substr(0, i));
                    break;
                }
            }
        }

        // 4. Go back inside identifier.
        std::size_t b = head.size();
        while (b > 0 && is_ident(head[b - 1])) --b;
        return head.substr(b);
    }

    struct fixed_name {
        char data[255]{};
        explicit constexpr fixed_name(const std::string_view s) {
            const std::size_t n = s.size() < sizeof(data) - 1 ? s.size() : sizeof(data) - 1;
            for (std::size_t i = 0; i < n; ++i) data[i] = s[i];
        }
    };

}

namespace logger {
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
                if (!trace) return;
                trace->Trace(
                    0,
                    static_cast<eP7Trace_Level>(WARNING),
                    nullptr,
                    location.line(),
                    location.file_name(),
                    detail::fixed_name(
                        detail::extract_function_name(location.function_name()).data()
                    ).data,
                    L"Logger is alredy initialized"
                );
                return;
            }

            std::setvbuf(stdout, nullptr, _IONBF, 0);
            std::setvbuf(stderr, nullptr, _IONBF, 0);

            //TODO: P7_Set_Crash_Handler(); hides crash reason.
            //Needs custom handler to ensure flush on crash!
            client = P7_Create_Client(options);
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
            isReady = false;
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

        // Allows to use logger::LOG instead of logger::Logger::LOG for compatibility.
        template <typename... Args> friend struct LOG;
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
    struct LOG {
        /**
         * Logs a formatted wide-character message.
         * @param level severity level of the log message.
         * @param format wide-character format string literal.
         * @param args formatting arguments corresponding to the format specifiers in the format string.
         * @param location source location info. Can be specified to report a different call site.
         */
        LOG(
            LogLevel level,
            const wchar_t *format,
            Args&&... args,
            const std::source_location& location = std::source_location::current()
        ) {
#ifdef LOGGER_DISABLED
            return;
#endif

            Logger::trace->Trace(
                0,
                static_cast<eP7Trace_Level>(level),
                nullptr,
                location.line(),
                location.file_name(),
                detail::fixed_name(
                    detail::extract_function_name(location.function_name())
                ).data,
                format,
                std::forward<Args>(args)...
            );

            if (level == ERROR || level == CRITICAL) {
                Logger::client->Flush();
            }
        }
    };

    /**
     * Deduction guide for LOG.
     *
     * Enables deduction of the formatting argument types,
     * allowing usage without explicit template arguments.
     */
    template <typename... Args>
    LOG(LogLevel, const wchar_t *, Args&&...) -> LOG<Args...>;
}

/**
 * Export to global namespace for compatibility
 */
export using logger::Logger;
export using logger::LOG;
export using logger::LogLevel::TRACE;
export using logger::LogLevel::DEBUG;
export using logger::LogLevel::INFO;
export using logger::LogLevel::WARNING;
export using logger::LogLevel::ERROR;
export using logger::LogLevel::CRITICAL;
