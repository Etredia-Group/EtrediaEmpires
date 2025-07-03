#pragma once
#include <entt/entt.hpp>

namespace ee_ecs
{
	struct enterprise
	{
		bool is_building = false;

		entt::entity state;
	};
}