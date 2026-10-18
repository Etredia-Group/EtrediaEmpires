module;

#include <entt/entt.hpp>

export module Economy:Firm;
import aliases;
import std;

export namespace ee_economy {

    struct FirmDefinition {
        ee::FirmTag tag;
        ee::RecipeTag productionRecipe;
        ee::RecipeTag constructionRecipe;
        std::uint8_t constructionSpeed;
    };
    using FirmDefId = std::uint8_t;
    using FirmList = std::vector<FirmDefinition>;

    class FirmDefinitionTable {
    public:
        explicit FirmDefinitionTable(FirmList &definitions);
        std::optional<FirmDefId> Find(ee::FirmTag tag) const noexcept;
        const FirmDefinition &operator[](FirmDefId id) const noexcept;

    private:
        FirmList &definitions;
        entt::dense_map<ee::FirmTag, FirmDefId> defByTag;
    };

}

// ECS Components

export namespace ee_economy {

    struct FirmDefComponent { FirmDefId definition; };
    struct FirmBatchComponent {
        std::uint16_t count;
        float damage;
        float efficiency;
    };

}

// ECS Component operations

export namespace ee_economy {

    void MergeFirmBatches(FirmBatchComponent &target, const FirmBatchComponent &source) noexcept;

}