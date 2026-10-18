module;

#include <entt/entt.hpp>

export module Economy:BaseComponents;
import aliases;

export namespace ee_economy {

    struct OwnerCorpoComponent { entt::entity entity; };
    struct OnRegionComponent { ee::RegionId id; };

}