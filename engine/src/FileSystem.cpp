#include "FileSystem.hpp"
#include "Logger.h"
#include <ee_utils/allocator.hpp>

IFileSystem::~IFileSystem()
{
    for (FileHandle* handle : activeFiles)
        Deallocate(handle);
    for (FileHandle* handle : failedFiles)
        Deallocate(handle);
}

#ifdef _WIN32

FileHandle& NativeFileSystem::Open(const std::filesystem::path &path, const uint32_t flags)
{
    FileHandle* result = Allocate<FileHandle>();

    #ifndef NDEBUG
    result->debugPath = path;
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
        return *result;
    }

    result->win32Handle = file;
    result->size = GetFileSize(file, NULL);
    if (result->size == INVALID_FILE_SIZE)
        LOG(ERROR, "Failed to get size of file %s. Error: %d", path.c_str(), GetLastError());

    if (freeFileSlots.size() > 0)
    {
        size_t pos = freeFileSlots.back();
        result->id = pos;
        activeFiles[pos] = result;
        freeFileSlots.pop_back();
    }
    else
    {
        result->id = activeFiles.size();
        activeFiles.push_back(result);
    }

    return *result;
}

void NativeFileSystem::ReadAsync(FileHandle &file, const uint32_t bufferSize, const uint32_t flags)
{
    file.overlapped = {
        .hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr)
    };
    file.buffer = AllocateArray<uint8_t>(bufferSize);
    file.bufferSize = bufferSize;

    BOOL isSync = ReadFile(
        file.win32Handle,
        file.buffer,
        bufferSize,
        NULL,
        &file.overlapped
    );

    if (isSync || GetLastError() != ERROR_IO_PENDING)
        LOG(ERROR, "File reading is synchronious while asynchronious is awaited");
}

bool NativeFileSystem::PollReady(FileHandle &file)
{
    DWORD status = WaitForSingleObject(file.overlapped.hEvent, 0);

    switch (status)
    {
        case WAIT_OBJECT_0:
        {
            if (file.size == file.bufferSize)
            {
                file.data = file.buffer;
                file.buffer = nullptr;
            }
            else
            {
                /* TODO: reserve size for data, shift and move */
            }

            break;
        }

        case WAIT_ABANDONED:
        case WAIT_TIMEOUT:
            return false;

        default:
        {
            LOG(CRITICAL, "Failed to get file status. Error: %d", GetLastError());
            return false;
        }
    }
    
    return true;
}

void NativeFileSystem::Close(FileHandle &file)
{
    if (file.overlapped.hEvent)
        CloseHandle(file.overlapped.hEvent);

    if (file.buffer != nullptr)
    {
        DeallocateArray(file.buffer);
        file.buffer = nullptr;
    }
    if (file.data != nullptr)
    {
        DeallocateArray(file.data);
        file.data = nullptr;
    }

    if (file.win32Handle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(file.win32Handle);
        file.win32Handle = INVALID_HANDLE_VALUE;
    }
    freeFileSlots.push_back(file.id);
}

#endif

#ifdef __linux__



#endif