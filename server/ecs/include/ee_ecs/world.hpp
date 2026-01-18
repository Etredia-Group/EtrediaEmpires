#pragma once
#include "ee_economy/components/container.hpp"
#include <entt/entity/fwd.hpp>
#include <entt/entt.hpp>
#include <ee_economy/economy.hpp>
#include <unordered_map>

namespace ee_ecs
{
	class World
	{
	private:
		entt::registry registry;
		std::unordered_map<int16_t, entt::entity> statesTable;

		void updateEnterprises();

	public:
		// Run every registered system
		void update();

		entt::entity registerState(const int16_t id, const ee_economy::Container &resources);
		
		entt::entity spawnEnterprise(
			const bool isBuilding,
			const ee_economy::ACTION_TYPE action,
			const entt::entity state,
			const ee_economy::EnterpriseDefaults &defaults
		);

		entt::registry& getRegistry();
	};
}