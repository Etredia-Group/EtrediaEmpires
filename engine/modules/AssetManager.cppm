module;

#include "compile_config.hpp"
#include <xxhash.h>
#include <rfl/capnproto.hpp>

export module AssetManager;
export import AssetData;

import utils;
import FileSystem;
import std;

export
{
    struct AssetAddress {
        std::atomic<void*> resource;
    };

    struct AssetHandle {
        static constexpr std::uint32_t INVALID_ASSET_ID = UINT32_MAX;
        std::size_t slot = 0;
        std::uint32_t id = 0;
    };

    class AssetInfo {
    public:
        ee_assets::AssetType type = ee_assets::BINARY;

        std::atomic<bool> isReady;
        AssetAddress *resourceAddress;

        explicit AssetInfo(void *resource) {
            this->resourceAddress = Allocate<AssetAddress>();
            this->resourceAddress->resource.store(resource);
        }

        AssetInfo(const AssetInfo&) = delete;
        AssetInfo& operator=(const AssetInfo&) = delete;

        AssetInfo(AssetInfo&& other) noexcept :
            type(other.type),
            resourceAddress(other.resourceAddress) {
            isReady.store(other.isReady.load(std::memory_order_relaxed), std::memory_order_relaxed);

            other.resourceAddress = nullptr;
            other.isReady.store(false, std::memory_order_relaxed);
        }

        AssetInfo& operator=(AssetInfo&& other) noexcept {
            if (this != &other) {
                type = other.type;
                resourceAddress = other.resourceAddress;
                isReady.store(other.isReady.load(std::memory_order_relaxed), std::memory_order_relaxed);

                other.resourceAddress = nullptr;
                other.isReady.store(false, std::memory_order_relaxed);
            }
            return *this;
        }
    };

    struct AssetLoadingEntry {
        AssetHandle handle;
        FileHandle *file;
        IFileSystem *fileOwner;
    };

    struct AssetStreamCommand : ee_utils::WorkerCommand {
        enum Type : std::uint8_t {
            NONE,

            LOAD,
            FIND,
            GET,

            COMPLETED
        };

        AssetHandle target;
        std::uint32_t targetId = AssetHandle::INVALID_ASSET_ID;
        std::filesystem::path targetPath;
        ee_assets::AssetType targetType = ee_assets::BINARY;
        std::atomic<void*> *out = nullptr;
    };

    class AssetManager final : public ee_utils::ThreadedWorker<AssetStreamCommand, AssetStreamCommand::COMPLETED> {
    public:
        AssetManager() : ThreadedWorker(L"Assets Stream", 250) {
            commandsMapping[AssetStreamCommand::LOAD] = std::bind_front(&AssetManager::LoadCmd, this);
            commandsMapping[AssetStreamCommand::FIND] = std::bind_front(&AssetManager::FindCmd, this);
            commandsMapping[AssetStreamCommand::GET] = std::bind_front(&AssetManager::GetCmd, this);

            diskSystem = Allocate<NativeFileSystem>();
        }

        ~AssetManager() override {
            Deallocate(diskSystem);
        }

        void LoadDir(ee_assets::AssetType type, const std::filesystem::path& path) {
            for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(path)) {
                if (entry.is_directory()) continue;
                LoadAsync(type, entry.path());
            }
        }

        void LoadAsync(ee_assets::AssetType type, const std::filesystem::path& path) {
            AssetStreamCommand command;
            command.type = AssetStreamCommand::LOAD;
            command.targetPath = path;
            command.targetType = type;

            Request(command);
        }

        AssetHandle& Find(const std::filesystem::path& path) {
            std::atomic<void*> result;
            AssetStreamCommand command;
            command.type = AssetStreamCommand::FIND;
            command.targetPath = path;
            command.out = &result;

            Request(command);
            while (command.out->load() == nullptr) {}
            return *static_cast<AssetHandle*>(command.out->load());
        }

        AssetHandle& Find(const std::uint32_t id) {
            std::atomic<void*> result;
            AssetStreamCommand command;
            command.type = AssetStreamCommand::FIND;
            command.targetId = id;
            command.out = &result;

            Request(command);
            while (command.out->load() == nullptr) {}
            return *static_cast<AssetHandle*>(command.out->load());
        }

        // Returned resource reference should not be stored
        template <ee_assets::Asset T>
        T& FindOnce(const std::filesystem::path& path) {
            return Get<T>(Find(path));
        }

        // Returned resource reference should not be stored
        template <ee_assets::Asset T>
        T& FindOnce(const std::uint32_t id) {
            return Get<T>(Find(id));
        }

        void Redirect(const AssetHandle& handle, void *resource) const {
            const AssetInfo& asset = activeAssets[handle.slot];
            if (asset.resourceAddress == nullptr) {
                LOG(WARNING, "Attempt to redirect empty handle");
                return;
            }

            // TODO: LEAK! Free old resource after redirecting handler to other resource
            if (asset.resourceAddress->resource.load() != nullptr) {
                void *oldResource = asset.resourceAddress->resource.exchange(nullptr, std::memory_order_acq_rel);
                //if (oldResource)
                //Deallocate(oldResource);
            }

            asset.resourceAddress->resource.store(resource);
        }

        // Returned resource reference should not be stored
        template <ee_assets::Asset T>
        T& Get(const AssetHandle& handle) {
            std::atomic<void*> result;
            AssetStreamCommand command;
            command.type = AssetStreamCommand::GET;
            command.target = handle;
            command.out = &result;

            Request(command);
            while (command.out->load() == nullptr) {}
            return *static_cast<T*>(command.out->load());
        }

        void Free(const AssetHandle& handle) {
            AssetInfo& asset = activeAssets[handle.slot];
            Redirect(handle, nullptr);
            Deallocate(asset.resourceAddress);
            asset.resourceAddress = nullptr;

            freeAssetSlots.push_back(handle.slot);
        }

    protected:
        void Update() override {
            // TODO: FinishLoadTasks()
            for (AssetLoadingEntry& entry : loadingEntries) {
                if (entry.fileOwner->PollReady(*entry.file)) {
                    AssetInfo& asset = activeAssets[entry.handle.slot];

                    switch (asset.type) {
                    case ee_assets::MAP_MESH:
                        FinishLoadCapnprotoDeserialize(entry, mapMeshesStorage);
                        break;
                    case ee_assets::SAVE:
                        FinishLoadCapnprotoDeserialize(entry, savesStorage);
                        break;

                    case ee_assets::SHADER:
                        FinishLoadConstruct(entry, shadersStorage);
                        break;
                    case ee_assets::MATERIAL:
                        FinishLoadCapnprotoDeserialize(entry, materialsStorage);
                        break;
                    case ee_assets::IMAGE:
                        break;

                    case ee_assets::BINARY:
                    default:
                        FinishLoadConstruct(entry, binaryStorage);
                        break;
                    }

                    entry.fileOwner->Close(*entry.file);
#ifndef NDEBUG
                    LOG(INFO, "Asset loaded: [%hs]", entry.file->debugPath.generic_u8string().c_str());
#endif
                    entry.file = nullptr;
                    asset.isReady.store(true);
                }
            }

            std::erase_if(loadingEntries, [](const AssetLoadingEntry& entry) {
                return entry.file == nullptr;
            });
        }

        void Release() override {
            for (const AssetInfo& asset : activeAssets) {
                //TODO: LEAK! Free resource
                if (asset.resourceAddress->resource.load() != nullptr) {
                    void *resource = asset.resourceAddress->resource.exchange(nullptr, std::memory_order_acq_rel);
                    //if (resource)
                    //Deallocate(resource);
                }
                Deallocate(asset.resourceAddress);
            }
            activeAssets.clear();

            for (AssetHandle *& handle : assetsMapping | std::views::values)
                Deallocate(handle);
            assetsMapping.clear();

            binaryStorage.clear();
            mapMeshesStorage.clear();
            shadersStorage.clear();
            materialsStorage.clear();
        }

    private:
        IFileSystem *diskSystem;

        std::vector<AssetInfo> activeAssets;
        std::unordered_map<std::filesystem::path, std::uint32_t> assetsIdMapping;
        std::unordered_map<std::uint32_t, AssetHandle*> assetsMapping;
        std::vector<std::size_t> freeAssetSlots;
        std::vector<AssetLoadingEntry> loadingEntries;

        std::vector<ee_assets::AssetBinary*> binaryStorage;
        std::vector<ee_assets::AssetSave*> savesStorage;
        std::vector<ee_assets::AssetMapMesh*> mapMeshesStorage;
        std::vector<ee_assets::AssetShader*> shadersStorage;
        std::vector<ee_assets::AssetMaterial*> materialsStorage;

        void LoadCmd(AssetStreamCommand& command) {
            LOG(INFO, "LOAD command: [%hs]", command.targetPath.generic_u8string().c_str());

            /* TODO: process path to select FS */
            /* TODO: select between async load and sync job task */

            //Async load of full file without mmap in one thread
            FileHandle& file = diskSystem->Open(command.targetPath, IFileSystem::FILE_OPEN_READ);

            diskSystem->ReadAsync(file, static_cast<std::uint32_t>(file.size), IFileSystem::FILE_READ_SEQUENTIAL);

            AssetHandle *handle = Allocate<AssetHandle>();
            AssetInfo asset(nullptr);
            asset.type = command.targetType;

            std::size_t assetPos = 0;
            if (freeAssetSlots.size() > 0) {
                assetPos = freeAssetSlots.back();
                Deallocate(activeAssets[assetPos].resourceAddress);
                activeAssets[assetPos] = std::move(asset);
                freeAssetSlots.pop_back();
            }
            else {
                activeAssets.push_back(std::move(asset));
                assetPos = activeAssets.size() - 1;
            }
            handle->slot = assetPos;
            const std::basic_string<char8_t> assetPath = command.targetPath.generic_u8string();
            const std::uint32_t assetId = assetsIdMapping[command.targetPath] =
                XXH32(assetPath.data(), assetPath.size(), ASSETS_HASH_SPACE);
            handle->id = assetId;
            assetsMapping[assetId] = handle;

            const AssetLoadingEntry loadingEntry{
                .handle = *handle,
                .file = &file,
                .fileOwner = diskSystem
            };
            loadingEntries.push_back(loadingEntry);

            command.type = AssetStreamCommand::COMPLETED;
        }

        void FindCmd(AssetStreamCommand& command) {
            std::uint32_t id = command.targetId;
            if (id == AssetHandle::INVALID_ASSET_ID) {
                std::unordered_map<std::filesystem::path, std::uint32_t>::iterator idMapping = assetsIdMapping.find(
                    command.targetPath);
                if (idMapping == assetsIdMapping.end())
                    return;
                id = idMapping->second;
            }

            const std::unordered_map<std::uint32_t, AssetHandle*>::iterator assetMapping = assetsMapping.find(id);

            if (assetMapping == assetsMapping.end())
                return;

            if (command.out != nullptr)
                command.out->store(assetMapping->second);

            command.type = AssetStreamCommand::COMPLETED;
        }

        void GetCmd(AssetStreamCommand& command) const {
            if (activeAssets.size() <= command.target.slot)
                return;
            if (!activeAssets[command.target.slot].isReady)
                return;

            if (command.out != nullptr)
                command.out->store(activeAssets[command.target.slot].resourceAddress->resource);

            command.type = AssetStreamCommand::COMPLETED;
        }

        template <ee_assets::Asset T>
        void FinishLoadConstruct(AssetLoadingEntry& loaded, std::vector<T*>& storage) {
            T *resource = Allocate<T>(loaded.file->data, loaded.file->size);
            storage.push_back(resource);
            Redirect(loaded.handle, storage.back());
        }

        template <ee_assets::Asset T>
        void FinishLoadCapnprotoDeserialize(AssetLoadingEntry& loaded, std::vector<T*>& storage) {
            try {
                if (rfl::Result<T> result = rfl::capnproto::read<T>(
                    reinterpret_cast<const char*>(loaded.file->data),
                    loaded.file->size
                )) {
                    storage.push_back(Allocate<T>(std::move(*result)));
                    Redirect(loaded.handle, storage.back());
                }
                else {
#if !defined(NDEBUG)
                    LOG(
                        ERROR,
                        "Failed to read asset: [%s]. [%hs]",
                        loaded.file->debugPath.c_str(),
                        result.error().what().c_str()
                    );
#else
                    LOG(ERROR, "Failed to read asset: ID[%d]. [%hs]", loaded.file->id, result.error().what().c_str());
#endif
                }
            } catch (std::exception e) {
                storage.push_back(Allocate<T>());
                Redirect(loaded.handle, storage.back());
#if !defined(NDEBUG)
                LOG(ERROR, "Failed to read asset: [%s]. [%hs]", loaded.file->debugPath.c_str(), e.what());
#else
                LOG(ERROR, "Failed to read asset: ID[%d]. [%hs]", loaded.file->id, e.what());
#endif
            }
        }

        template <ee_assets::Asset T>
        void ClearStorage(std::vector<T*>& storage) {
            for (T * & resource : storage) {
                if (resource != nullptr)
                    Deallocate(resource);
            }
            storage.clear();
        }
    };
}
