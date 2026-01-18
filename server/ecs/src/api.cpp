#include "ee_ecs/world.hpp"
#include <ee_ecs/ecs.hpp>


static ee_ecs::World worldInstance;

void ee_ecs::init(ee_ecs::InitConfiguration config)
{
	for (auto& [id, container]: config.states)
    {
        worldInstance.registerState(id, container);
    }
}

void ee_ecs::start()
{
	while (true) {}
}

template<typename T>
    const std::vector<ee_ecs::EnterpriseInfo> ee_ecs::getAllEnterprises()
{
    std::vector<EnterpriseInfo> result;
    //TODO: REPLACE VIEWS WITH GROUPS!
    auto enterprises = worldInstance.getRegistry().group<>(entt::get<
        ee_economy::Enterprise,
        ee_economy::Container,
        T
    >);
    result.reserve(enterprises.size_hint()); 

    for (auto [entity, enterprise, container, action]: enterprises.each())
    {
        EnterpriseInfo eninfo;
        eninfo.info = &enterprise;
        
        eninfo.storage = &container;
        eninfo.specialInfo = &action;

        result.push_back(eninfo);
    }

    return result;
}
