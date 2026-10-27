module;

#include <ee_utils/platform.hpp>

export module FileSystem;
import std;

export namespace ee_engine
{
    struct FileHandle {
        std::size_t id = 0;

        std::size_t size = 0;
        std::size_t bufferSize = 0;
        std::uint8_t *data = nullptr;
        std::uint8_t *buffer = nullptr;

        bool isCompleted = false;
        bool isClosing = false;

#ifndef NDEBUG
        std::filesystem::path debugPath;
#endif

#ifdef _WIN32
        HANDLE win32Handle = INVALID_HANDLE_VALUE;
#endif

#ifdef __linux__
#endif
    };

#ifdef _WIN32
    struct IOContext {
        OVERLAPPED overlapped;
        FileHandle *file;
        std::size_t currentOffset;
    };
#endif


    class IFileSystem {
    public:
        static constexpr std::uint32_t FILE_OPEN_NONE = 0;
        static constexpr std::uint32_t FILE_OPEN_READ = (1 << 0);
        static constexpr std::uint32_t FILE_OPEN_WRITE = (1 << 1);
        static constexpr std::uint32_t FILE_OPEN_MMAP = (1 << 2);
        static constexpr std::uint32_t FILE_READ_SEQUENTIAL = 0;
        static constexpr std::uint32_t FILE_READ_RANDOM_ACCESS = (1 << 0);

        virtual FileHandle& Open(const std::filesystem::path& path, std::uint32_t flags) = 0;

        virtual void ReadAsync(FileHandle& file, std::uint32_t bufferSize, std::uint32_t flags) = 0;

        virtual void PollLoads() = 0;

        virtual void Close(FileHandle& file) = 0;

        virtual ~IFileSystem();

    protected:
        std::vector<FileHandle*> activeFiles;
        std::vector<FileHandle*> failedFiles;
        std::vector<std::size_t> freeFileSlots;
    };

    class NativeFileSystem final : public IFileSystem {
    public:
        NativeFileSystem();
        ~NativeFileSystem() override;

        FileHandle& Open(const std::filesystem::path &path, std::uint32_t flags) override;

        void ReadAsync(FileHandle &file, std::uint32_t bufferSize, std::uint32_t flags) override;

        void PollLoads() override;

        void Close(FileHandle &file) override;

    private:

#ifdef _WIN32
        HANDLE iocpHandle = INVALID_HANDLE_VALUE;

        bool HandleChunk(IOContext *context, DWORD bytesTransferred, bool success);
        void IssueRead(IOContext *context);
#endif

        void FreeFile(FileHandle &file);
    };
}
