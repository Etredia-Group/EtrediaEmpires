#include "ee_ecs/world.hpp"
#include <ee_ecs/ecs.hpp>


static ee_ecs::world world_instance;

void ee_ecs::init(ee_ecs::InitConfiguration config)
{
	for (auto& [id, container]: config.states)
    {
        world_instance.register_state(id, container);
    }
}

void ee_ecs::start()
{
	
}

void ee_ecs::stop()
{
	
}

