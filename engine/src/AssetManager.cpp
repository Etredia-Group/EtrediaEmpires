/* Operations with capnproto in template methods need to be isolated
 * inside AssetManager module using implementation unit.
 * Otherwise <ee_utils/serialization.hpp> needs to be included in every
 * file that calls Load* methods to avoid compilation error.
 *
 * Register here list of every used asset type with template specializations.
 */

module;

#include <ee_utils/serialization.hpp>

module AssetManager;
import std;
import utils;
import FileSystem;

// Registry of assets

template const AssetOperations* AssetManager::OperationsFor<ee_assets::Binary>();
template const AssetOperations* AssetManager::OperationsFor<ee_assets::MapMesh>();
template const AssetOperations* AssetManager::OperationsFor<ee_assets::Material>();
template const AssetOperations* AssetManager::OperationsFor<ee_assets::Save>();
template const AssetOperations* AssetManager::OperationsFor<ee_assets::ResourceList>();
template const AssetOperations* AssetManager::OperationsFor<ee_assets::FirmList>();
template const AssetOperations* AssetManager::OperationsFor<ee_assets::ProductionRecipeList>();

// Finish* methods implementation

template <ee_assets::Asset T>
const AssetOperations* AssetManager::OperationsFor() {
    static constexpr AssetOperations ops { &FinishLoadAsset<T>, &DestroyAsset<T> };
    return &ops;
}

template <ee_assets::Asset T>
void AssetManager::FinishLoadAsset(AssetManager &self, AssetLoadingEntry &loaded) {
    if constexpr (std::constructible_from<T, std::uint8_t*&, std::size_t>) {
        self.FinishLoadConstruct<T>(loaded);
    }
    else {
        self.FinishLoadCapnprotoDeserialize<T>(loaded);
    }
}

template <ee_assets::Asset T>
void AssetManager::FinishLoadConstruct(AssetLoadingEntry& loaded) {
    Redirect(loaded.handle, Allocate<T>(loaded.file->data, loaded.file->size));
}

template <ee_assets::Asset T>
void AssetManager::FinishLoadCapnprotoDeserialize(AssetLoadingEntry& loaded) {
    try {
        if (rfl::Result<T> result = rfl::capnproto::read<T>(
            reinterpret_cast<const char*>(loaded.file->data),
            loaded.file->size
        )) {
            Redirect(loaded.handle, Allocate<T>(std::move(*result)));
        }
        else {
            Redirect(loaded.handle, Allocate<T>());
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
        Redirect(loaded.handle, Allocate<T>());
#if !defined(NDEBUG)
        LOG(ERROR, "Failed to read asset: [%s]. [%hs]", loaded.file->debugPath.c_str(), e.what());
#else
        LOG(ERROR, "Failed to read asset: ID[%d]. [%hs]", loaded.file->id, e.what());
#endif
    }
}