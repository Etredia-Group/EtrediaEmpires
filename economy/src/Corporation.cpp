module;

#include <entt/entt.hpp>

module Economy;
import :BaseComponents;
import EcsContext;

std::size_t ee_economy::FirmIndexHash::operator()(FirmIndex x) const noexcept {
    x += 0x9e3779b97f4a7c15ull;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ull;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebull;
    return x ^ x >> 31;
}

constexpr ee_economy::FirmIndex ee_economy::GetFirmIndex(
    const ee::RegionId region,
    const ee::FirmDefId definition
) noexcept {
    return (static_cast<FirmIndex>(region) << 32) | definition;
}

entt::entity ee_economy::CreateCorporation(entt::registry &registry) {
    const entt::entity corporation = registry.create();
    registry.emplace<Corporation>(corporation);
    return corporation;
}

entt::entity ee_economy::AddFirms(
    entt::registry &registry,
    const entt::entity corporation,
    const ee::RegionId region,
    const ee::FirmDefId firmsDef,
    FirmBatchComponent &firmsData
) {
    Firms &firms = registry.get<Corporation>(corporation).firms;
    const auto [entry, isInserted] = firms.try_emplace(
        GetFirmIndex(region, firmsDef)
    );

    if (isInserted) {
        firmsData.storageSlot = registry.ctx().get<StoragePools>()[firmsDef].Allocate();

        const entt::entity entity = registry.create();
        registry.emplace<FirmBatchComponent>(entity, firmsData);
        registry.emplace<OwnerCorpoComponent>(entity, corporation);

        ee_ecs::PlaceInRegion(registry, entity, region);

        entry->second = entity;

        return entity;
    }

    const entt::entity batch = entry->second;
    MergeFirmBatches(registry.get<FirmBatchComponent>(batch), firmsData);
    return batch;
}

void ee_economy::DestroyFirms(entt::registry &registry, const entt::entity firmsBatch) {
    const auto [definition, owner, region] =
        registry.get<FirmDefComponent, OwnerCorpoComponent, ee_ecs::OnRegionComponent>(firmsBatch);
    registry.get<Corporation>(owner.entity).firms.erase(
        GetFirmIndex(region.id, definition.id)
    );
    ee_ecs::RemoveFromRegion(registry, firmsBatch);
    registry.destroy(firmsBatch);
}
