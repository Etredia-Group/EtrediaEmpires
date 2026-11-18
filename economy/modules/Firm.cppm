export module Economy:Firm;
import :Resources;
import entt;
import EcsContext;
import aliases;
import std;

export namespace ee_economy {

    struct FirmDefinition {
        using Tag = ee::FirmIdentity; Tag tag;

        ee::RecipeIdentity productionRecipe;
        //ee::RecipeIdentity constructionRecipe; TODO
        ee::StorageUsage storageCapacity;
        std::uint32_t constructionSpeed;
    };
    using FirmList = std::vector<FirmDefinition>;

    class FirmDefinitionTable final : public ee_ecs::TagTable<FirmDefinition> {
    public:
        FirmDefinitionTable(entt::registry &registry, const ee::ResourceLayout &sharedStorage);
        void OnLink(const FirmDefinition &definition) override;
        void OnLink() override;

        std::span<const ee::Id> getResourceLayout(ee::Id firmId) const noexcept;
        const ProductionRecipe& getProduction(ee::Id firmId) const noexcept;

    private:
        ResourceTable &resourcesContext;
        ProductionRecipeTable &recipesContext;
        std::vector<ee::ResourceLayout> resourceLayouts;
        ee::ResourceLayout sharedResourceLayout;
        std::vector<ProductionRecipe> productions; // Recipe cache for every definition

        std::optional<ee::ResourceTypesRange> getResourceIndex(
            ee::Id firmId,
            ee::Id resource
        ) const noexcept;
    };

}

template<> struct ee_ecs::table_by_tag<ee::FirmIdentity>{ using type = ee_economy::FirmDefinitionTable; };

// ECS Components

export namespace ee_economy {

    struct FirmDefComponent { std::uint8_t id; };
    struct FirmBatchComponent {
        std::uint32_t count = 0;
        float damage = 0.0f;
        float efficiency = 1.0f;

        ee::StorageSlot storageSlot;
    };

}

// ECS Component operations

export namespace ee_economy {

    void MergeFirmBatches(FirmBatchComponent &target, const FirmBatchComponent &source) noexcept;

    float CalcFirmsIncomeFactor(const FirmBatchComponent &batch) noexcept;
    float CalcFirmsOutcomeFactor(const FirmBatchComponent &batch) noexcept;

}