#include <ee_economy/economy.hpp>

void ee_economy::mining(entt::registry &registry)
{
	auto miners = registry.view<enterprise, miner, container>();

	for (auto [entity, enterprise, miner, local_storage]: miners.each())
	{
		container& state = registry.get<container>(enterprise.state);

		double mined_value = enterprise.efficiency;
		if (local_storage.total + mined_value > local_storage.capacity)
			mined_value = local_storage.capacity - local_storage.total;
		if (mined_value > state.content[miner.resource])
			mined_value = state.content[miner.resource];

		local_storage.add(miner.resource, mined_value);
		state.add(miner.resource, -mined_value);
	}
}