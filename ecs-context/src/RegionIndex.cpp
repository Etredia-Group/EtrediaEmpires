module;

#include <entt/entt.hpp>

module EcsContext;

void ee_ecs::RegionIndex::Add(const ee::RegionId region, const entt::entity entity) {
    entitiesByRegion[region].push_back(entity);
}

void ee_ecs::RegionIndex::Remove(const ee::RegionId region, const entt::entity entity) {
    const auto entry = entitiesByRegion.find(region);
    if (entry == entitiesByRegion.end()) return;

    std::vector<entt::entity> &list = entry->second;
    if (
        const auto pos = std::ranges::find(list, entity);
        pos != list.end()
    ) {
        *pos = list.back();
        list.pop_back();
    }
}

std::span<const entt::entity> ee_ecs::RegionIndex::Get(const ee::RegionId region) const noexcept {
    const auto entry = entitiesByRegion.find(region);
    if (entry == entitiesByRegion.end()) return {};
    return std::span{entry->second};
}

void ee_ecs::PlaceInRegion(entt::registry &registry, const entt::entity entity, const ee::RegionId region) {
    RegionIndex &index = registry.ctx().get<RegionIndex>();

    if (OnRegionComponent *current = registry.try_get<OnRegionComponent>(entity)) {
        if (current->id == region) return;

        index.Remove(current->id, entity);
        current->id = region;
    }
    else {
        registry.emplace<OnRegionComponent>(entity, region);
    }

    index.Add(region, entity);
}

void ee_ecs::RemoveFromRegion(entt::registry &registry, const entt::entity entity) {
    registry.ctx().get<RegionIndex>().Remove(registry.get<OnRegionComponent>(entity).id, entity);
    registry.remove<OnRegionComponent>(entity);
}
