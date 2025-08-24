#include "AssetManager.hpp"
#include "allocator.hpp"
#include "Logger.h"
#include <stdexcept>

AssetInfo::AssetInfo(void *resource)
{
    this->resourceAddress = Allocate<AssetAddress>();
    this->resourceAddress->resource.store(resource);
}

AssetInfo::AssetInfo(AssetInfo&& other) noexcept :
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
        resourceAddress = other.resourceAddress;
        isReady.store(other.isReady.load(std::memory_order_relaxed), std::memory_order_relaxed);

        other.resourceAddress = nullptr;
        other.isReady.store(false, std::memory_order_relaxed);
    }
    return *this;
}

void AssetManager::Redirect(AssetHandle &handle, void* resource)
{
    AssetInfo &asset = activeAssets[handle.id];
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
    AssetInfo &asset = activeAssets[handle.id];
    Redirect(handle, nullptr);
    Deallocate(asset.resourceAddress);
    asset.resourceAddress = nullptr;

    freeAssetSlots.push_back(handle.id);
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
                AssetInfo& asset = activeAssets[entry.handle.id];

                //TODO: functions for each type
                switch (asset.type)
                {
                    case AssetInfo::MATERIAL:
                    {
                        break;
                    }

                    case AssetInfo::IMAGE:
                    {
                        break;
                    }

                    case AssetInfo::SHADER:
                    case AssetInfo::BINARY:
                    default:
                    {
                        AssetBinary* data = new AssetBinary(entry.file->data, entry.file->size);
                        binaryStorage.push_back(data);
                        Redirect(entry.handle, binaryStorage.back());
                        break;
                    }
                }

                entry.fileOwner->Close(*entry.file);
#ifndef NDEBUG
                auto path = entry.file->debugPath.c_str();
                LOG(INFO, "Asset loaded: [%s]", path);
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
            LOG(INFO, "LOAD command: [%s]", command.targetPath.c_str());

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
            handle->id = assetPos;
            assetsMapping[command.targetPath] = handle;

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
            std::unordered_map<std::filesystem::path, AssetHandle*>::iterator mapping = assetsMapping.find(command.targetPath);
            
            if (mapping == assetsMapping.end())
                break;

            command.out->store(mapping->second);

            command.type = AssetStreamCommand::COMPLETED;
            break;
        }

        case AssetStreamCommand::GET:
        {
            if (activeAssets.size() <= command.target.id)
                break;
            if (!activeAssets[command.target.id].isReady)
                break;

            command.out->store(activeAssets[command.target.id].resourceAddress->resource);

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