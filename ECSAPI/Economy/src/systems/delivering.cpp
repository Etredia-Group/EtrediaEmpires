#include <ee_economy/economy.hpp>

void ee_economy::delivering(entt::registry &registry)
{
    auto deliverers = registry.view<enterprise, deliverer>();

    for (auto [entity, enterprise, deliverer]: deliverers.each())
    {
        for (deliver_contract &contract: deliverer.contracts)
        {
            container &from_storage = registry.get<container>(contract.from);
            container &to_storage = registry.get<container>(contract.to);

            double deliver_value = enterprise.efficiency;
            if (contract.value < enterprise.efficiency)
                deliver_value = contract.value;
            if (to_storage.total + deliver_value > to_storage.capacity)
                deliver_value = to_storage.capacity - to_storage.total;
            if (deliver_value > from_storage.content[deliverer.resource])
                deliver_value = from_storage.content[deliverer.resource];
            if (deliver_value < 0.0)
                deliver_value = 0.0;

            to_storage.add(deliverer.resource, deliver_value);
            from_storage.add(deliverer.resource, -deliver_value);
            contract.value -= deliver_value;
        }

        std::erase_if(deliverer.contracts, [](const deliver_contract &contract) {
            return contract.value <= 0.0;
        });
    }
}