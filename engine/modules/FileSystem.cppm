module;

#include <ee_utils/platform.hpp>

export module FileSystem;
import std;

export
{
    struct FileHandle {
        std::size_t id = 0;

        std::size_t size = 0;
        std::size_t bufferSize = 0;
        std::uint8_t *data = nullptr;
        std::uint8_t *buffer = nullptr;

#ifndef NDEBUG
        std::filesystem::path debugPath;
#endif

#ifdef _WIN32
        HANDLE win32Handle = INVALID_HANDLE_VALUE;
        OVERLAPPED overlapped;
#endif

#ifdef __linux__
#endif
    };

    class IFileSystem {
    public:
        inline static constexpr std::uint32_t FILE_OPEN_NONE = 0;
        inline static constexpr std::uint32_t FILE_OPEN_READ = (1 << 0);
        inline static constexpr std::uint32_t FILE_OPEN_WRITE = (1 << 1);
        inline static constexpr std::uint32_t FILE_OPEN_MMAP = (1 << 2);
        inline static constexpr std::uint32_t FILE_READ_SEQUENTIAL = 0;
        inline static constexpr std::uint32_t FILE_READ_RANDOM_ACCESS = (1 << 0);

        virtual FileHandle& Open(const std::filesystem::path& path, const std::uint32_t flags) = 0;

        virtual void ReadAsync(FileHandle& file, const std::uint32_t bufferSize, const std::uint32_t flags) = 0;

        virtual bool PollReady(FileHandle& file) = 0;

        virtual void Close(FileHandle& file) = 0;

        virtual ~IFileSystem();

    protected:
        std::vector<FileHandle*> activeFiles;
        std::vector<FileHandle*> failedFiles;
        std::vector<std::size_t> freeFileSlots;
    };

    class NativeFileSystem final : public IFileSystem {
    public:
        FileHandle& Open(const std::filesystem::path& path, const std::uint32_t flags) override;

        void ReadAsync(FileHandle& file, const std::uint32_t bufferSize, std::uint32_t flags) override;

        bool PollReady(FileHandle& file) override;

        void Close(FileHandle& file) override;

    private:
    };
}
