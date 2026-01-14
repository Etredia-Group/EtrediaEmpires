#include "compile_config.hpp"
#include "AssetManager.hpp"
#include "allocator.hpp"
#include "Logger.h"
#include <stdexcept>
#include <xxhash.h>
#include <cstring>


AssetInfo::AssetInfo(void *resource)
{
    this->resourceAddress = Allocate<AssetAddress>();
    this->resourceAddress->resource.store(resource);
}

AssetInfo::AssetInfo(AssetInfo&& other) noexcept :
    type(other.type),
    resourceAddress(other.resourceAddress)
{
    isReady.store(other.isReady.load(std::memory_order_relaxed), std::memory_order_relaxed);
    
    other.resourceAddress = nullptr;
    other.isReady.store(false, std::memory_order_relaxed);
}

AssetInfo& AssetInfo::operator=(AssetInfo&& other) noexcept
{
    if (this != &other)
    {
        type = other.type;
        resourceAddress = other.resourceAddress;
        isReady.store(other.isReady.load(std::memory_order_relaxed), std::memory_order_relaxed);

        other.resourceAddress = nullptr;
        other.isReady.store(false, std::memory_order_relaxed);
    }
    return *this;
}

void AssetManager::Redirect(AssetHandle &handle, void* resource)
{
    AssetInfo &asset = activeAssets[handle.slot];
    if (asset.resourceAddress == nullptr)
    {
        LOG(WARNING, "Attempt to redirect empty handle");
        return;
    }

    if (asset.resourceAddress->resource.load() != nullptr)
    {
        void* oldResource = asset.resourceAddress->resource.exchange(nullptr, std::memory_order_acq_rel);
        if (oldResource)
            Deallocate(asset.resourceAddress->resource.load());
    }

    asset.resourceAddress->resource.store(resource);
}

void AssetManager::Free(AssetHandle &handle)
{
    AssetInfo &asset = activeAssets[handle.slot];
    Redirect(handle, nullptr);
    Deallocate(asset.resourceAddress);
    asset.resourceAddress = nullptr;

    freeAssetSlots.push_back(handle.slot);
}

AssetManager::AssetManager()
{
    diskSystem = Allocate<NativeFileSystem>();
}

AssetManager::~AssetManager()
{
    Deallocate(diskSystem);
}

void AssetManager::Request(const AssetStreamCommand &command)
{
	commands.push(command);
}

void AssetManager::Request(const AssetStreamCommand::Type &type)
{
	commands.push(AssetStreamCommand {.type = type});
}

void AssetManager::LoadAsync(AssetInfo::Type type, const std::filesystem::path &path)
{
    AssetStreamCommand command {
        .type = AssetStreamCommand::LOAD,
        .targetPath = path,
        .targetType = type
    };
    Request(command);
}

AssetHandle& AssetManager::Find(const std::filesystem::path &path)
{
    std::atomic<void*> result;
    AssetStreamCommand command {
        .type = AssetStreamCommand::FIND,
        .targetPath = path,
        .out = &result
    };
    Request(command);
    while (command.out->load() == nullptr);
    return *reinterpret_cast<AssetHandle*>(command.out->load());
}
AssetHandle& AssetManager::Find(const uint32_t &id)
{
    std::atomic<void*> result;
    AssetStreamCommand command {
        .type = AssetStreamCommand::FIND,
        .targetId = id,
        .out = &result
    };
    Request(command);
    while (command.out->load() == nullptr);
    return *reinterpret_cast<AssetHandle*>(command.out->load());
}

void AssetManager::Start()
{
    if (isRunning) throw std::logic_error("Asset Streaming can be started only once");

    isRunning.store(true);
    streamingThread = std::jthread([this](std::stop_token stop) {
        this->stoken = &stop;
        Threading();
    });
}

void AssetManager::Shutdown()
{
    streamingThread.request_stop();
}

void AssetManager::Threading()
{
    Logger::trace->Register_Thread(L"Asset Stream", 0);
    LOG(INFO, "Asset Streaming thread started");

    Loop();

    Release();
}

void AssetManager::Loop()
{
    AssetStreamCommand commandBuffer;
    std::vector<AssetStreamCommand> incompleteCommands;
    while (!stoken->stop_requested())
    {
		while (commands.pop(commandBuffer))
		{
			ProcessCommand(commandBuffer);
            if (commandBuffer.type != AssetStreamCommand::COMPLETED)
                incompleteCommands.push_back(commandBuffer);
		}
        for (AssetStreamCommand &command : incompleteCommands)
        {
            commands.push(command);
        }
        incompleteCommands.clear();

		for (AssetLoadingEntry &entry : loadingEntries)
        {
            bool isReady = entry.fileOwner->PollReady(*entry.file);
            if (isReady)
            {
                AssetInfo& asset = activeAssets[entry.handle.slot];

                switch (asset.type)
                {
                    case AssetInfo::SHADER:
                    {
                        AssetShader* shader = Allocate<AssetShader>(entry.file->data, entry.file->size);
                        shadersStorage.push_back(shader);
                        Redirect(entry.handle, shadersStorage.back());
                        break;
                    }

                    case AssetInfo::MATERIAL:
                    {
                        AssetMaterial* material = Allocate<AssetMaterial>();
                        if (entry.file->size < sizeof(AssetMaterial))
                            LOG(CRITICAL, "File size is less than needed for material");
                        std::memcpy(material, entry.file->data, sizeof(AssetMaterial));
                        materialsStorage.push_back(material);
                        Redirect(entry.handle, materialsStorage.back());
                        break;
                    }

                    case AssetInfo::IMAGE:
                    {
                        break;
                    }

                    case AssetInfo::BINARY:
                    default:
                    {
                        AssetBinary* data = Allocate<AssetBinary>(entry.file->data, entry.file->size);
                        binaryStorage.push_back(data);
                        Redirect(entry.handle, binaryStorage.back());
                        break;
                    }
                }

                entry.fileOwner->Close(*entry.file);
#ifndef NDEBUG
                LOG(INFO, "Asset loaded: [%hs]", entry.file->debugPath.generic_u8string().c_str());
#endif
                entry.file = nullptr;
                asset.isReady.store(true);
            }
        }

        std::erase_if(loadingEntries, [](const AssetLoadingEntry &entry) {
            return entry.file == nullptr;
        });
    }
}
void AssetManager::ProcessCommand(AssetStreamCommand &command)
{
    switch(command.type)
	{
		case AssetStreamCommand::LOAD:
        {
            LOG(INFO, "LOAD command: [%hs]", command.targetPath.generic_u8string().c_str());

            /* TODO: process path to select FS */
            /* TODO: select between async load and sync job task */

            //Async load of full file without mmap in one thread
            FileHandle &file = diskSystem->Open(command.targetPath, FILE_OPEN_READ);

            diskSystem->ReadAsync(file, static_cast<uint32_t>(file.size), FILE_READ_SEQUENTIAL);
            
            AssetHandle* handle = Allocate<AssetHandle>();
            AssetInfo asset(nullptr);
            asset.type = command.targetType;
            
            size_t assetPos = 0;
            if (freeAssetSlots.size() > 0)
            {
                assetPos = freeAssetSlots.back();
                activeAssets[assetPos] = std::move(asset);
                freeAssetSlots.pop_back();
            }
            else
            {
                activeAssets.push_back(std::move(asset));
                assetPos = activeAssets.size() - 1;
            }
            handle->slot = assetPos;
            auto assetPath = command.targetPath.generic_u8string();
            uint32_t assetId = assetsIdMapping[command.targetPath] =
                XXH32(assetPath.data(), assetPath.size(), ASSETS_HASH_SPACE);
            handle->id = assetId;
            assetsMapping[assetId] = handle;

            AssetLoadingEntry loadingEntry {
                .handle = *handle,
                .file = &file,
                .fileOwner = diskSystem
            };
            loadingEntries.push_back(loadingEntry);
            
            command.type = AssetStreamCommand::COMPLETED;
            break;
        }

        case AssetStreamCommand::FIND:
        {
            uint32_t id = command.targetId;
            if (id == INVALID_ASSET_ID)
            {
                std::unordered_map<std::filesystem::path, uint32_t>::iterator idMapping = assetsIdMapping.find(command.targetPath);
                if (idMapping == assetsIdMapping.end())
                    break;
                id = idMapping->second;
            }

            std::unordered_map<uint32_t, AssetHandle*>::iterator assetMapping = assetsMapping.find(id);
            
            if (assetMapping == assetsMapping.end())
                break;

            command.out->store(assetMapping->second);

            command.type = AssetStreamCommand::COMPLETED;
            break;
        }

        case AssetStreamCommand::GET:
        {
            if (activeAssets.size() <= command.target.slot)
                break;
            if (!activeAssets[command.target.slot].isReady)
                break;

            command.out->store(activeAssets[command.target.slot].resourceAddress->resource);

            command.type = AssetStreamCommand::COMPLETED;
            break;
        }

		default:
		{
			LOG(WARNING, "Unhandled command. Type id: %d", command.type);
			break;
		}
	}
}

void AssetManager::Release()
{
    for (AssetInfo &asset : activeAssets)
    {
        if (asset.resourceAddress->resource.load() != nullptr)
        {
            void* oldResource = asset.resourceAddress->resource.exchange(nullptr, std::memory_order_acq_rel);
            if (oldResource)
                Deallocate(asset.resourceAddress->resource.load());
        }
        Deallocate(asset.resourceAddress);
    }
    activeAssets.clear();

    for (auto &mapping : assetsMapping)
        Deallocate(mapping.second);
    assetsMapping.clear();

    //TODO: ClrearStorage(&storage);
    for (AssetBinary* &resource : binaryStorage)
    {
        if (resource != nullptr)
            Deallocate(resource);
    }
    binaryStorage.clear();
    for (AssetShader* &resource : shadersStorage)
    {
        if (resource != nullptr)
            Deallocate(resource);
    }
    shadersStorage.clear();

    LOG(INFO, "Asset Streaming is released");
    isRunning.store(false);
}

void AssetManager::LoadShaders()
{
    std::filesystem::path shadersRoot = "assets/shaders";
    for (const std::filesystem::directory_entry &entry : std::filesystem::directory_iterator(shadersRoot))
    {
        if (entry.is_directory()) continue;
        LoadAsync(AssetInfo::SHADER, entry.path());
    }
}