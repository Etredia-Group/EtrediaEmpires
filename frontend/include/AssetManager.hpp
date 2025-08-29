#pragma once
#include "FileSystem.hpp"
#include "AssetData.hpp"

#include <vector>
#include <unordered_map>
#include <filesystem>
#include <thread>
#include <atomic>
#include <cstdint>
#include <Boost/lockfree/spsc_queue.hpp>

#define INVALID_ASSET_ID UINT32_MAX

struct AssetAddress
{
    std::atomic<void*> resource;
};

struct AssetHandle
{
    size_t slot = 0;
    uint32_t id = 0;
};

class AssetInfo
{
public:
    enum Type
    {
        BINARY,

        SHADER,
        MATERIAL,
        IMAGE
    } type = BINARY; 

    std::atomic<bool> isReady;
    AssetAddress* resourceAddress;

    AssetInfo(void *resource);

    AssetInfo(const AssetInfo&) = delete;
    AssetInfo& operator=(const AssetInfo&) = delete;

    AssetInfo(AssetInfo&& other) noexcept;
    AssetInfo& operator=(AssetInfo&& other) noexcept;
};

struct AssetLoadingEntry
{
    AssetHandle handle;
    FileHandle* file;
    IFileSystem* fileOwner;
};

struct AssetStreamCommand
{
    enum Type
    {
        NONE,

        LOAD,
        FIND,
        GET,

        COMPLETED
    } type = NONE;

    AssetHandle target;
    uint32_t targetId = INVALID_ASSET_ID;
    std::filesystem::path targetPath;
    AssetInfo::Type targetType = AssetInfo::BINARY;
    std::atomic<void*>* out;
};

class AssetManager
{
public:
    std::atomic<bool> isRunning;

    AssetManager();
    ~AssetManager();

    void Start();
    void Shutdown();

    void Request(const AssetStreamCommand &command);
    void Request(const AssetStreamCommand::Type &type);

    void LoadShaders();
    
    void LoadAsync(AssetInfo::Type type, const std::filesystem::path &path);
    AssetHandle& Find(const std::filesystem::path &path);
    AssetHandle& Find(const uint32_t &id);
    void Redirect(AssetHandle &handle, void *resource);
    template <Asset T> T& Get(const AssetHandle &handle);
    void Free(AssetHandle &handle);

private:
    std::stop_token *stoken;
    std::jthread streamingThread;
    boost::lockfree::spsc_queue<AssetStreamCommand, boost::lockfree::capacity<512>> commands;

    IFileSystem *diskSystem;

    std::vector<AssetInfo> activeAssets;
    std::unordered_map<std::filesystem::path, uint32_t> assetsIdMapping;
    std::unordered_map<uint32_t, AssetHandle*> assetsMapping;
    std::vector<size_t> freeAssetSlots;
    std::vector<AssetLoadingEntry> loadingEntries;

    std::vector<AssetBinary*> binaryStorage;
    std::vector<AssetShader*> shadersStorage;
    std::vector<AssetMaterial*> materialsStorage;

    void Threading();
    void Loop();
    void ProcessCommand(AssetStreamCommand &command);
    void Release();
};

template <Asset T> T& AssetManager::Get(const AssetHandle &handle)
{
    std::atomic<void*> result;
    AssetStreamCommand command {
        .type = AssetStreamCommand::GET,
        .target = handle,
        .out = &result
    };
    Request(command);
    while (command.out->load() == nullptr);
    return *reinterpret_cast<T*>(command.out->load());
}