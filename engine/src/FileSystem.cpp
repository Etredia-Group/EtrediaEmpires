module;

#include <ee_utils/platform.hpp>

module FileSystem;
import utils;

IFileSystem::~IFileSystem() {
    for (FileHandle *handle : activeFiles)
        Deallocate(handle);
    for (FileHandle *handle : failedFiles)
        Deallocate(handle);
}

#ifdef _WIN32

NativeFileSystem::NativeFileSystem() {
    iocpHandle = CreateIoCompletionPort(
        INVALID_HANDLE_VALUE,
        nullptr,
        0,
        0
    );
    if (iocpHandle == nullptr) {
        LOG(CRITICAL, "Failed to create IOCP. Error: %d", GetLastError());
    }
}

NativeFileSystem::~NativeFileSystem() {
    // I have no idea why ReSharper think that this condition is always false
    // ReSharper disable once CppDFAConstantConditions
    if (iocpHandle != INVALID_HANDLE_VALUE && iocpHandle != nullptr) {
        // ReSharper disable once CppDFAUnreachableCode
        CloseHandle(iocpHandle);
    }
}

FileHandle& NativeFileSystem::Open(const std::filesystem::path& path, const std::uint32_t flags) {
    FileHandle *result = Allocate<FileHandle>();

#ifndef NDEBUG
    result->debugPath = path;
#endif

    DWORD createFileMode = 0;

    if (flags & FILE_OPEN_READ)
        createFileMode |= GENERIC_READ;
    if (flags & FILE_OPEN_WRITE)
        createFileMode |= GENERIC_WRITE;

    const HANDLE file = CreateFileW(
        path.c_str(),
        createFileMode,
        NULL,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        nullptr
    );

    if (file == INVALID_HANDLE_VALUE) {
        LOG(ERROR, "Failed to open file %s. Error: %d", path.c_str(), GetLastError());
        failedFiles.push_back(result);
        return *result;
    }

    result->win32Handle = file;
    result->size = GetFileSize(file, nullptr);
    if (result->size == INVALID_FILE_SIZE)
        LOG(ERROR, "Failed to get size of file %s. Error: %d", path.c_str(), GetLastError());

    if (
        CreateIoCompletionPort(
            file,
            iocpHandle,
            NULL,
            NULL
        ) == nullptr
    ) {
        LOG(ERROR, "Failed to associate file with IOCP. Error: %d", GetLastError());
    }

    if (freeFileSlots.size() > 0) {
        const std::size_t pos = freeFileSlots.back();
        result->id = pos;
        activeFiles[pos] = result;
        freeFileSlots.pop_back();
    }
    else {
        result->id = activeFiles.size();
        activeFiles.push_back(result);
    }

    return *result;
}

void NativeFileSystem::ReadAsync(FileHandle& file, const std::uint32_t bufferSize, const std::uint32_t flags) {
    file.isCompleted = false;
    file.isClosing = false;
    file.bufferSize = bufferSize;
    file.buffer = AllocateArray<std::uint8_t>(file.bufferSize);

    IOContext *context = Allocate<IOContext>();
    context->file = &file;
    context->currentOffset = 0;
    context->overlapped = {};

    if (!ReadFile(file.win32Handle, file.buffer, bufferSize, nullptr, &context->overlapped)) {
        if (DWORD error = GetLastError(); error != ERROR_IO_PENDING) {
#ifndef NDEBUG
            LOG(ERROR, "Failed to start reading file [%s]. Error: %d", file.debugPath.c_str(), error);
#else
            LOG(ERROR, "Failed to start reading file [%d]. Error: %d", file.id, error);
#endif
            HandleChunk(context, 0, false);
        }
    }
}

void NativeFileSystem::PollLoads() {
    DWORD bytesTransferred = 0;
    OVERLAPPED *overlapped = nullptr;
    ULONG_PTR ck = 999;

    while (true) {
        const bool success = GetQueuedCompletionStatus(
            iocpHandle,
            &bytesTransferred,
            &ck,
            &overlapped,
            0
        );

        if (!overlapped) { // No more ready files
            if (DWORD error = GetLastError(); !success && error != WAIT_TIMEOUT) {
                LOG(ERROR, "Can not get files loading status. Error: %d", error);
            }
            break;
        }

        if (
            IOContext *context = reinterpret_cast<IOContext*>(overlapped);
            HandleChunk(context, bytesTransferred, success)
        ) {
            IssueRead(context);
        }
    }
}

void NativeFileSystem::Close(FileHandle& file) {
    if (file.win32Handle != INVALID_HANDLE_VALUE) {
        CancelIoEx(file.win32Handle, nullptr);

        if (file.isCompleted) {
            FreeFile(file);
        }
        else {
            file.isClosing = true;
        }
    }
}

bool NativeFileSystem::HandleChunk(IOContext *context, const DWORD bytesTransferred, const bool success) {
    FileHandle &file = *context->file;

    if (!success) {
        if (const DWORD error = GetLastError(); error != ERROR_OPERATION_ABORTED) {
#ifndef NDEBUG
            LOG(ERROR, "IOCP read error [%s]. Error: %d", file.debugPath.c_str(), error);
#else
            LOG(ERROR, "IOCP read error [%d]. Error: %d", file.id, error);
#endif
        }

        if (file.isClosing) FreeFile(file);

        Deallocate(context);
        file.isCompleted = true;
        return false;
    }

    if (file.data == nullptr) { // Initialize for first chunk
        if (file.size <= file.bufferSize) { // File of one chunk, just target resource
            file.data = file.buffer;
            file.buffer = nullptr;
        }
        else {
            file.data = AllocateArray<std::uint8_t>(file.size); // Allocate memory for all chunks
        }
    }

    if (bytesTransferred == 0) { // Case of unexpected EOF
        Deallocate(context);
        if (file.isClosing) FreeFile(file);
        file.isCompleted = true;
        return false;
    }

    if (file.buffer != nullptr) {
        std::ranges::copy(file.buffer, file.buffer + bytesTransferred, file.data + context->currentOffset);
    }
    context->currentOffset += bytesTransferred;

    if (context->currentOffset >= file.size) { // Finish reading of completed file
        Deallocate(context);
        if (file.isClosing) FreeFile(file);
        file.isCompleted = true;
        return false;
    }

    return true;
}

void NativeFileSystem::IssueRead(IOContext *context) {
    // 64-bit offset is seperated for WinAPI
    context->overlapped.Offset = static_cast<DWORD>(context->currentOffset);
    context->overlapped.OffsetHigh = static_cast<DWORD>(context->currentOffset >> 32);

    if (
        !ReadFile(
            context->file->win32Handle,
            context->file->buffer,
            context->file->bufferSize,
            nullptr,
            &context->overlapped
        )
    ) {
        if (GetLastError() != ERROR_IO_PENDING) {
            HandleChunk(context, 0, false);
        }
    }
}

#endif

#ifdef __linux__


#endif

void NativeFileSystem::FreeFile(FileHandle &file) {
    if (file.buffer != nullptr) DeallocateArray(file.buffer);
    if (file.data != nullptr) DeallocateArray(file.data);
    file.buffer = nullptr;
    file.data = nullptr;

#ifdef _WIN32
    if (file.win32Handle != INVALID_HANDLE_VALUE) CloseHandle(file.win32Handle);
    file.win32Handle = INVALID_HANDLE_VALUE;
#endif

    freeFileSlots.push_back(file.id);
}