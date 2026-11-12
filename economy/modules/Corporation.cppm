module;

#include <entt/entt.hpp>

export module Economy:Corporation;
import aliases;
import :Firm;

export namespace ee_economy {

    using FirmIndex = std::uint64_t;
    struct FirmIndexHash {
        std::size_t operator()(FirmIndex x) const noexcept;
    };
    constexpr FirmIndex GetFirmIndex(ee::RegionId region, ee::FirmDefId definition) noexcept;
    using Firms = entt::dense_map<FirmIndex, entt::entity, FirmIndexHash>;

}

// ECS Components

export namespace ee_economy {

    struct Corporation {
        Firms firms;
    };

}

// ECS Component operations

export namespace ee_economy {

    entt::entity CreateCorporation(entt::registry &registry);
    entt::entity AddFirms(
        entt::registry &registry,
        entt::entity corporation,
        ee::RegionId region,
        ee::FirmDefId firmsDef,
        FirmBatchComponent &firmsData
    );
    void DestroyFirms(entt::registry &registry, entt::entity firmsBatch);

}