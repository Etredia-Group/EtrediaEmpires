#include "ee_ecs/world.hpp"
#include <ee_ecs/ecs.hpp>


static ee_ecs::world world_instance;

void ee_ecs::init(ee_ecs::init_configuration config)
{
	for (auto& [id, container]: config.states)
    {
        world_instance.register_state(id, container);
    }
}

void ee_ecs::start()
{
	while (true) {}
}

template<typename T>
    const std::vector<ee_ecs::enterprise_info> ee_ecs::get_all_enterprises()
{
    std::vector<enterprise_info> result;
    //TODO: REPLACE VIEWS WITH GROUPS!
    auto enterprises = world_instance.get_registry().group<>(entt::get<
        ee_economy::enterprise,
        ee_economy::container,
        T
    >);
    result.reserve(enterprises.size_hint()); 

    for (auto [entity, enterprise, container, action]: enterprises.each())
    {
        enterprise_info eninfo;
        eninfo.info = &enterprise;
        
        eninfo.storage = &container;
        eninfo.special_info = &action;

        result.push_back(eninfo);
    }

    return result;
}
