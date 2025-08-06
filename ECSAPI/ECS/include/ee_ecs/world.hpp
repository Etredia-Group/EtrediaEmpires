#pragma once
#include "ee_economy/components/container.hpp"
#include <entt/entity/fwd.hpp>
#include <entt/entt.hpp>
#include <ee_economy/economy.hpp>
#include <unordered_map>

namespace ee_ecs
{
	class world
	{

	private:
		entt::registry registry;
		std::unordered_map<int16_t, entt::entity> states_table;

		void update_enterprises();

	public:
		// Run every registered system
		void update();

		entt::entity register_state(const int16_t id, const ee_economy::container &resources);
		
		entt::entity spawn_enterprise(
			const bool is_building,
			const ee_economy::ACTION_TYPE action,
			const entt::entity state,
			const ee_economy::enterprise_defaults &defaults
		);

		entt::registry& get_registry();
	};
}