export module EcsContext:RegionIndex;
import std;
import entt;
import aliases;

export namespace ee_ecs {

    struct OnRegionComponent { ee::RegionId id; };

    class RegionIndex {
    public:
        std::span<const entt::entity> Get(ee::RegionId region) const noexcept;

    private:
        void Add(ee::RegionId region, entt::entity entity);
        void Remove(ee::RegionId region, entt::entity entity);
        entt::dense_map<ee::RegionId, std::vector<entt::entity>> entitiesByRegion;

        friend void PlaceInRegion(entt::registry &registry, entt::entity entity, ee::RegionId region);
        friend void RemoveFromRegion(entt::registry &registry, entt::entity entity);
    };

    void PlaceInRegion(entt::registry &registry, entt::entity entity, ee::RegionId region);
    void RemoveFromRegion(entt::registry &registry, entt::entity entity);

}