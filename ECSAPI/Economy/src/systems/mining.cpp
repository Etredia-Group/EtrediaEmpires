#include <ee_economy/economy.hpp>

void ee_ecs::mining(entt::registry &registry)
{
	auto miners = registry.view<enterprise, miner, container>();

	for (auto [entity, enterprise, miner, local_storage]: miners.each())
	{
		container& state = registry.get<container>(enterprise.state);

		if (!state.content.contains(miner.resource) || state.content[miner.resource] < TOLERANCE)
			continue;

		double mined_value = miner.value;
		if (local_storage.total + mined_value > local_storage.capacity)
			mined_value = local_storage.capacity - local_storage.total;
		if (mined_value > state.content[miner.resource])
			mined_value = state.content[miner.resource];

		local_storage.add(miner.resource, mined_value);
		state.add(miner.resource, -mined_value);
	}
}