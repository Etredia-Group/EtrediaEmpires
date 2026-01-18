#include <ee_economy/economy.hpp>
#include <ee_ecs/ecs.hpp>
#include <stdexcept>

void ee_ecs::World::updateEnterprises()
{
	ee_economy::enterpriseRoutine(this->registry);

	ee_economy::mining(this->registry);
	ee_economy::delivering(this->registry);
}

void ee_ecs::World::update()
{
	this->updateEnterprises();
}

entt::entity ee_ecs::World::registerState(const int16_t id, const ee_economy::Container &resources)
{
	if (this->statesTable.contains(id))
		throw std::invalid_argument("State with this Id is already registered");

	entt::entity entity = this->registry.create();

	this->registry.emplace<ee_economy::State>(entity, id);
	this->registry.emplace<ee_economy::Container>(
		entity,
		resources.capacity,
		resources.total,
		resources.content
	);

	this->statesTable[id] = entity;

	return entity;
}

entt::entity ee_ecs::World::spawnEnterprise(
	const bool isBuilding,
	const ee_economy::ACTION_TYPE action,
	const entt::entity state,
	const ee_economy::EnterpriseDefaults &defaults
)
{
	entt::entity entity = this->registry.create();

	this->registry.emplace<ee_economy::Enterprise>(
		entity,
		isBuilding,
		state,
		action,

		defaults.baseEfficiency * defaults.factorEfficiency,

		defaults.baseEfficiency,
		defaults.factorEfficiency
	);

	this->registry.emplace<ee_economy::Container>(
		entity,
		defaults.container.capacity,
		defaults.container.total,
		defaults.container.content
	);

	switch (action)
	{
	case ee_economy::ACTION_TYPE::MINING:
	{
		this->registry.emplace<ee_economy::Miner>(entity, defaults.resourceTarget);
		break;
	}

	case ee_economy::ACTION_TYPE::DELIVERY:
	{
		this->registry.emplace<ee_economy::Deliverer>(entity, defaults.resourceTarget);
		break;
	}

	default:
		break;
	}

	return entity;
}

entt::registry& ee_ecs::World::getRegistry() {return this->registry;}