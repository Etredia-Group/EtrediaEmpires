module;

#include <entt/entt.hpp>

export module EcsContext:RegionIndex;
import aliases;

export namespace ee_ecs {

    struct OnRegionComponent { ee::RegionId id; };

    class RegionIndex {
    public:
        void Add(ee::RegionId region, entt::entity entity);
        void Remove(ee::RegionId region, entt::entity entity);
        std::span<const entt::entity> Get(ee::RegionId region) const noexcept;

    private:
        entt::dense_map<ee::RegionId, std::vector<entt::entity>> entitiesByRegion;
    };

    void PlaceInRegion(entt::registry &registry, entt::entity entity, ee::RegionId region);
    void RemoveFromRegion(entt::registry &registry, entt::entity entity);

}