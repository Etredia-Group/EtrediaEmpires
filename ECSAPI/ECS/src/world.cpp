#include <ee_ecs/ecs.hpp>

void ee_ecs::world::update()
{
	mining(this->registry);
}

void ee_ecs::world::spawn_enterprise(const bool is_building, const ACTION_TYPE action, const entt::entity state, const enterprise_defaults defaults)
{
	entt::entity entity = this->registry.create();

	auto &enterprise_c = this->registry.emplace<enterprise>(entity);
	enterprise_c.is_building = is_building;
	enterprise_c.state = state;

	auto &container_c = this->registry.emplace<container>(entity);
	container_c = defaults.container;

	switch (action)
	{
	case ACTION_TYPE::MINING:
	{
		auto& miner_c = this->registry.emplace<miner>(entity);
		miner_c.resource = defaults.resource_target;
		miner_c.value = defaults.base_efficiency_flat;

		break;
	}

	default:
		break;
	}
}

entt::registry& ee_ecs::world::get_registry() {return this->registry;}