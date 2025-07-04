#include <ee_economy/economy.hpp>
#include <ee_ecs/ecs.hpp>
#include <stdexcept>

void ee_ecs::world::update_enterprises()
{
	ee_economy::mining(this->registry);
}

void ee_ecs::world::update()
{
	this->update_enterprises();
}

entt::entity ee_ecs::world::register_state(const short id, const ee_economy::container &resources)
{
	if (this->states_table.contains(id))
		throw std::invalid_argument("State with this Id is already registered");

	entt::entity entity = this->registry.create();

	this->registry.emplace<ee_economy::state>(entity, id);
	this->registry.emplace<ee_economy::container>(
		entity,
		resources.capacity,
		resources.total,
		resources.content
	);

	this->states_table[id] = entity;

	return entity;
}

entt::entity ee_ecs::world::spawn_enterprise(
	const bool is_building,
	const ee_economy::ACTION_TYPE action,
	const entt::entity state,
	const ee_economy::enterprise_defaults &defaults
)
{
	entt::entity entity = this->registry.create();

	this->registry.emplace<ee_economy::enterprise>(entity, is_building, state);

	this->registry.emplace<ee_economy::container>(
		entity,
		defaults.container.capacity,
		defaults.container.total,
		defaults.container.content
	);

	switch (action)
	{
	case ee_economy::ACTION_TYPE::MINING:
	{
		this->registry.emplace<ee_economy::miner>(entity, defaults.resource_target, defaults.base_efficiency_flat);
		break;
	}

	default:
		break;
	}

	return entity;
}

entt::registry& ee_ecs::world::get_registry() {return this->registry;}