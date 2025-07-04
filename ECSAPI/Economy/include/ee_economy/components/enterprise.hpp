#pragma once
#include <entt/entt.hpp>

namespace ee_economy
{
	struct enterprise
	{
		bool is_building = false;

		entt::entity state;
	};
}