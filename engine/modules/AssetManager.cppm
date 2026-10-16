module;

#include "compile_config.hpp"
#include <xxhash.h>
#include <ee_utils/serialization.hpp>

export module AssetManager;
export import AssetData;

import utils;
import FileSystem;
import std;

export
{
    // Forward declarations

    struct AssetOperations;
    class AssetManager;

    //

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
        std::atomic<bool> isReady;
        std::atomic<AssetAddress*> resourceAddress;
        const AssetOperations *operations = nullptr;

        explicit AssetInfo(void *resource) {
            this->resourceAddress = Allocate<AssetAddress>();
            this->resourceAddress.load(std::memory_order_relaxed)->resource.store(resource);
        }

        AssetInfo(const AssetInfo&) = delete;
        AssetInfo& operator=(const AssetInfo&) = delete;

        AssetInfo(AssetInfo&& other) noexcept :
            resourceAddress(other.resourceAddress.load(std::memory_order_relaxed)),
            operations(other.operations) {
            isReady.store(other.isReady.load(std::memory_order_relaxed), std::memory_order_relaxed);

            other.resourceAddress = nullptr;
            other.isReady.store(false, std::memory_order_relaxed);
        }

        AssetInfo& operator=(AssetInfo&& other) noexcept {
            if (this != &other) {
                resourceAddress = other.resourceAddress.load(std::memory_order_relaxed);
                operations = other.operations;
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

    struct AssetOperations {
        void (*Finish)(AssetManager&, AssetLoadingEntry&);
        void (*Destroy)(void*);
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
        const AssetOperations *operations = nullptr;
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

        template <ee_assets::Asset T>
        void LoadDir(const std::filesystem::path& path) {
            for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(path)) {
                if (entry.is_directory()) continue;
                LoadAsync<T>(entry.path());
            }
        }

        template <ee_assets::Asset T>
        void LoadAsync(const std::filesystem::path& path) {
            AssetStreamCommand command;
            command.type = AssetStreamCommand::LOAD;
            command.targetPath = path;
            command.operations = OperationsFor<T>();

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

        // ReSharper disable once CppMemberFunctionMayBeConst
        void Redirect(const AssetHandle& handle, void *resource) {
            const AssetInfo& asset = activeAssets[handle.slot];
            AssetAddress *address = asset.resourceAddress.load(std::memory_order_relaxed);
            if (address == nullptr) {
                LOG(WARNING, "Attempt to redirect empty handle");
                return;
            }

            if (
                void *oldResource = address->resource.exchange(
                    resource,
                    std::memory_order_acq_rel
                )
            )
                asset.operations->Destroy(oldResource);
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
            Deallocate(asset.resourceAddress.load(std::memory_order_relaxed));
            asset.resourceAddress = nullptr;

            freeAssetSlots.push_back(handle.slot);
        }

    protected:
        void Update() override {
            FinishLoadTasks();
        }

        void Release() override {
            for (const AssetInfo& asset : activeAssets) {
                if (
                    AssetAddress* address = asset.resourceAddress.load(std::memory_order_relaxed);
                    address != nullptr
                ) {
                    if (
                        void *resource = address->resource.exchange(
                            nullptr,
                            std::memory_order_acq_rel
                        )
                    )
                        asset.operations->Destroy(resource);
                    Deallocate(address);
                }
            }
            activeAssets.clear();

            for (AssetHandle *& handle : assetsMapping | std::views::values)
                Deallocate(handle);
            assetsMapping.clear();
        }

    private:
        IFileSystem *diskSystem;

        std::vector<AssetInfo> activeAssets;
        std::unordered_map<std::filesystem::path, std::uint32_t> assetsIdMapping;
        std::unordered_map<std::uint32_t, AssetHandle*> assetsMapping;
        std::vector<std::size_t> freeAssetSlots;
        std::vector<AssetLoadingEntry> loadingEntries;

        template <ee_assets::Asset T> static const AssetOperations* OperationsFor();

        void LoadCmd(AssetStreamCommand& command) {
            LOG(INFO, "LOAD command: [%hs]", command.targetPath.generic_u8string().c_str());

            /* TODO: process path to select FS */

            //Async load of full file without mmap in one thread
            FileHandle& file = diskSystem->Open(command.targetPath, IFileSystem::FILE_OPEN_READ);

            diskSystem->ReadAsync(file, static_cast<std::uint32_t>(file.size), IFileSystem::FILE_READ_SEQUENTIAL);

            AssetHandle *handle = Allocate<AssetHandle>();
            AssetInfo asset(nullptr);
            asset.operations = command.operations;

            std::size_t assetPos = 0;
            if (freeAssetSlots.size() > 0) {
                assetPos = freeAssetSlots.back();
                Deallocate(activeAssets[assetPos].resourceAddress.load(std::memory_order_relaxed));
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
                command.out->store(
                    activeAssets[command.target.slot].resourceAddress.load(std::memory_order_relaxed)->resource
                );

            command.type = AssetStreamCommand::COMPLETED;
        }

        void FinishLoadTasks() {
            diskSystem->PollLoads();

            for (AssetLoadingEntry& entry : loadingEntries) {
                if (entry.file->isCompleted) {
                    AssetInfo& asset = activeAssets[entry.handle.slot];

                    asset.operations->Finish(*this, entry);

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

        template <ee_assets::Asset T> static void FinishLoadAsset(AssetManager &self, AssetLoadingEntry &loaded);
        template <ee_assets::Asset T> void FinishLoadConstruct(AssetLoadingEntry& loaded);
        template <ee_assets::Asset T> void FinishLoadCapnprotoDeserialize(AssetLoadingEntry& loaded);

        template <ee_assets::Asset T>
        static void DestroyAsset(void *asset) {
            Deallocate(static_cast<T*>(asset));
        }
    };
}
