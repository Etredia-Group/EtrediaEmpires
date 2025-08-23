#include "FileSystem.hpp"
#include "Logger.h"
#include "allocator.hpp"

#ifdef _WIN32

FileHandle& NativeFileSystem::Open(const std::filesystem::path &path, const uint32_t flags)
{
    FileHandle result;

    #ifndef NDEBUG
    result.debugPath = path;
    #endif

    DWORD createFileMode = 0;

    if (flags & FILE_OPEN_READ)
        createFileMode |= GENERIC_READ;
    if (flags & FILE_OPEN_WRITE)
        createFileMode |= GENERIC_WRITE;

    HANDLE file = CreateFileW(
        path.c_str(),
        createFileMode,
        NULL,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL
    );

    if (file == INVALID_HANDLE_VALUE)
    {
        LOG(ERROR, "Failed to open file %s. Error: %d", path.c_str(), GetLastError());
        failedFiles.push_back(result);
        return failedFiles[result.id];
    }

    result.win32Handle = file;
    result.size = GetFileSize(file, NULL);
    if (result.size == INVALID_FILE_SIZE)
        LOG(ERROR, "Failed to get size of file %s. Error: %d", path.c_str(), GetLastError());

    if (freeFileSlots.size() > 0)
    {
        activeFiles[freeFileSlots.back()] = result;
        result.id = freeFileSlots.back();
        freeFileSlots.pop_back();
    }
    else
    {
        activeFiles.push_back(result);
        result.id = activeFiles.size() - 1;
    }

    return activeFiles[result.id];
}

void NativeFileSystem::ReadAsync(FileHandle &file, const uint32_t bufferSize, const uint32_t flags)
{
    file.overlapped = {};
    file.buffer = AllocateArray<uint8_t>(bufferSize);

    ReadFile(
        file.win32Handle,
        file.buffer,
        bufferSize,
        NULL,
        &file.overlapped
    );
}

bool NativeFileSystem::PollReady(FileHandle &file)
{
    DWORD transferredSize = 0L;
    BOOL status = GetOverlappedResult(
        file.win32Handle,
        &file.overlapped,
        &transferredSize,
        FALSE
    );
    if (!status && GetLastError() != ERROR_IO_INCOMPLETE)
    {
        LOG(ERROR, "Error in file status. Code: %d", GetLastError());
        return false;
    }

    if (file.size == transferredSize)
    {
        file.data = file.buffer;
    }
    else
    {
        /* TODO: reserve size for data, shift and move */
    }
    file.buffer = nullptr;

    return status;
}

void NativeFileSystem::Close(const FileHandle &file)
{
    DeallocateArray(file.buffer);
    DeallocateArray(file.data);

    CloseHandle(file.win32Handle);
    activeFiles[file.id].win32Handle = INVALID_HANDLE_VALUE;
    freeFileSlots.push_back(file.id);
}

#endif

#ifdef __linux__



#endif