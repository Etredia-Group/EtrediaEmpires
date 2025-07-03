#pragma once
#include "../defines.hpp"
#include <entt/entt.hpp>

namespace ee_ecs
{
	struct miner
	{
		RESOURCE_TYPE resource = RESOURCE_TYPE::NONE;
		double value = 0.;
	};
}