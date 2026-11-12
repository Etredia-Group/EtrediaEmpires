module;

#include <entt/entt.hpp>

export module Economy:Firm;
import :Resources;
import EcsContext;
import aliases;
import std;

export namespace ee_economy {

    struct FirmDefinition {
        ee::FirmTag tag;
        ee::RecipeTag productionRecipe;
        ee::RecipeTag constructionRecipe;
        std::uint8_t constructionSpeed;
    };
    using FirmList = std::vector<FirmDefinition>;

    class FirmDefinitionTable final : public ee_ecs::TagTable<FirmDefinition, ee::FirmDefId> {
    public:
        FirmDefinitionTable(entt::registry &registry, const ee::ResourceLayout &sharedStorage);
        void OnAdd(const FirmDefinition &definition) override;

        std::span<const ee::ResourceId> getResourceLayout(ee::FirmDefId id) const noexcept;
        std::optional<ee::ResourceTypesRange> getResourceIndex(
            ee::FirmDefId id,
            ee::ResourceId resource
        ) const noexcept;

    private:
        ResourceTable &resourcesContext;
        ProductionRecipeTable &recipesContext;
        std::vector<ee::ResourceLayout> resourceLayouts;
        ee::ResourceLayout sharedResourceLayout;
    };

}

// ECS Components

export namespace ee_economy {

    struct FirmDefComponent { ee::FirmDefId id; };
    struct FirmBatchComponent {
        std::uint32_t count;
        float damage;
        float efficiency;

        ee::StorageSlot storageSlot;
    };

}

// ECS Component operations

export namespace ee_economy {

    void MergeFirmBatches(FirmBatchComponent &target, const FirmBatchComponent &source) noexcept;
    ee::ResourceAmount FirmsScaleIncome(const FirmBatchComponent &batch, ee::ResourceAmount value) noexcept;
    ee::ResourceAmount FirmsScaleOutcome(const FirmBatchComponent &batch, ee::ResourceAmount value) noexcept;

}