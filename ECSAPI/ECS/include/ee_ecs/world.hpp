#pragma once
#include <entt/entt.hpp>
#include <ee_economy/economy.hpp>

namespace ee_ecs
{
	class world
	{

	private:
		entt::registry registry;

	public:
		/// <summary>
		/// Run every registered system
		/// </summary>
		void update();

		/// <summary>
		/// Register new enterprise entity in specified state
		/// </summary>
		/// <param name="action">represents type of activity that enterprise do every tick</param>
		/// <param name="state">is ID of state entity where enterprise will be placed</param>
		/// <param name="defaults">is initial properties of enterprise</param>
		void spawn_enterprise(const bool is_building, const ACTION_TYPE action, const entt::entity state, const enterprise_defaults defaults);


		entt::registry& get_registry();
	};
}