module;

#include <P7_Trace.h>
#undef ERROR
#undef TM

export module utils:logger;
import std;

namespace logger
{
    template <std::size_t length>
    struct wstring
    {
        wchar_t content[length];
        constexpr operator const wchar_t*() const {return content;}

        explicit constexpr wstring(const char (&str)[length]) : content{}
        {
            for (std::size_t i = 0; i < length; i++)
                content[i] = static_cast<wchar_t>(str[i]);
        }
    };

    template <std::size_t length>
    constexpr wstring<length> TM(const char (&str)[length])
    {
        return wstring<length>(str);
    }

    enum LogLevel : std::uint8_t
    {
        TRACE,
        DEBUG,
        INFO,
        WARNING,
        ERROR,
        CRITICAL,
    };

    class Logger;

    template <typename... Args>
    struct LOG_WCHAR
    {
        LOG_WCHAR(
            LogLevel level,
            const wchar_t* format,
            Args&&... args,
            const std::source_location &location = std::source_location::current()
        )
        {
            Logger::trace->Trace(
                0,
                static_cast<eP7Trace_Level>(level),
                nullptr,
                location.line(),
                location.file_name(),
                location.function_name(),
                format,
                std::forward<Args>(args)...
            );
        }
    };

    // Notice: logging char string supports only ASCII, use LOG_WCHAR with wchar string for UTF-8
    template <std::size_t length, typename... Args>
    struct LOG
    {
        LOG(
            LogLevel level,
            const char (&format)[length],
            Args&&... args,
            const std::source_location &location = std::source_location::current()
        )
        {
            Logger::trace->Trace(
                0,
                static_cast<eP7Trace_Level>(level),
                nullptr,
                location.line(),
                location.file_name(),
                location.function_name(),
                TM(format),
                std::forward<Args>(args)...
            );
        }
    };

    template <typename... Args> LOG_WCHAR(LogLevel, const wchar_t*, Args&&...) -> LOG_WCHAR<Args...>;
    template <std::size_t length, typename... Args> LOG(LogLevel, const char (&)[length], Args&&...) -> LOG<length, Args...>;

    class Logger
    {
    public:
        Logger() = delete;

        static void Init(const wchar_t *options)
        {
            if (isReady)
            {
                LOG_WCHAR(WARNING, L"Logger already initialized.");
                return;
            }

            client = P7_Create_Client(options);
            P7_Set_Crash_Handler();
            trace = P7_Create_Trace(client, L"TC 1");

            isReady = true;
        }

        static void Release()
        {
            trace->Release();
            client->Release();
        }

        static void RegisterThread(const wchar_t *name)
        {
            trace->Register_Thread(name, 0);
        }
        static void UnregisterThread()
        {
            trace->Unregister_Thread(0);
        }

    private:
        inline static IP7_Client *client = nullptr;
        inline static IP7_Trace *trace = nullptr;

        inline static bool isReady = false;

        template <typename... Args> friend struct LOG_WCHAR;
        template <std::size_t length, typename... Args> friend struct LOG;
    };

}

export using logger::Logger;
export using logger::LOG_WCHAR;
export using logger::LOG;
export using logger::LogLevel::TRACE;
export using logger::LogLevel::DEBUG;
export using logger::LogLevel::INFO;
export using logger::LogLevel::WARNING;
export using logger::LogLevel::ERROR;
export using logger::LogLevel::CRITICAL;