#pragma once
#include <filesystem>
#include <vector>
#include <cstdint>

#ifdef _WIN32
    #include <windows.h>
#endif

#ifdef __linux__
#endif

#define FILE_OPEN_NONE 0
#define FILE_OPEN_READ (1 << 0)
#define FILE_OPEN_WRITE (1 << 1)
#define FILE_OPEN_MMAP (1 << 2)

#define FILE_READ_SEQUENTIAL 0
#define FILE_READ_RANDOM_ACCESS (1 << 0)

struct FileHandle
{
    size_t id = 0;

    size_t size = 0;
    size_t bufferSize = 0;
    uint8_t *data = nullptr;
    uint8_t *buffer = nullptr;

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

class IFileSystem
{
public:
    virtual FileHandle& Open(const std::filesystem::path &path, const uint32_t flags) = 0;
    virtual void ReadAsync(FileHandle &file, const uint32_t bufferSize, const uint32_t flags) = 0;
    virtual bool PollReady(FileHandle &file) = 0;
    virtual void Close(const FileHandle &file) = 0;

protected:
    std::vector<FileHandle> activeFiles;
    std::vector<FileHandle> failedFiles;
    std::vector<size_t> freeFileSlots;
};

class NativeFileSystem : public IFileSystem
{
public:
    FileHandle& Open(const std::filesystem::path &path, const uint32_t flags) override;
    void ReadAsync(FileHandle &file, const uint32_t bufferSize, uint32_t flags) override;
    bool PollReady(FileHandle &file) override;
    void Close(const FileHandle &file) override;

private:

};