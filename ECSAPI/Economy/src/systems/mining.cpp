#include <ee_economy/economy.hpp>

void ee_economy::mining(entt::registry &registry)
{
	auto miners = registry.view<Enterprise, Miner, Container>();

	for (auto [entity, enterprise, miner, localStorage]: miners.each())
	{
		if (enterprise.isBankrupt) continue;

		Container& state = registry.get<Container>(enterprise.state);

		double minedValue = enterprise.efficiency;
		if (localStorage.total + minedValue > localStorage.capacity)
			minedValue = localStorage.capacity - localStorage.total;
		if (minedValue > state.content[miner.resource])
			minedValue = state.content[miner.resource];

		localStorage.add(miner.resource, minedValue);
		state.add(miner.resource, -minedValue);
	}
}