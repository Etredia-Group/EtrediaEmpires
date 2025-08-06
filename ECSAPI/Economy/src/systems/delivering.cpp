#include <ee_economy/economy.hpp>

void ee_economy::delivering(entt::registry &registry)
{
    auto deliverers = registry.view<enterprise, deliverer, container>();

    for (auto [entity, enterprise, deliverer, storage]: deliverers.each())
    {
        if (enterprise.is_bankrupt) continue;

        for (deliver_contract &contract: deliverer.contracts)
        {
            double paid_amount = contract.amount;
            
            if (contract.cost > 0)
            {
                ee_economy::enterprise &sponsor = registry.get<ee_economy::enterprise>(contract.sponsor);
                
                double cost_per_one = contract.cost / contract.amount;
                double payment = contract.cost;

                if (payment > sponsor.total_money)
                {
                    payment = (int)(sponsor.total_money / cost_per_one) * cost_per_one;
                    paid_amount = payment / cost_per_one;
                }
                sponsor.set_money(sponsor.total_money - payment);
                enterprise.set_money(enterprise.total_money + payment);
                contract.cost -= payment;
            }

            container &from_storage = registry.get<container>(contract.from);
            container &to_storage = registry.get<container>(contract.to);

            double deliver_value = enterprise.efficiency;
            if (contract.amount < enterprise.efficiency)
                deliver_value = contract.amount;
            if (to_storage.total + deliver_value > to_storage.capacity)
                deliver_value = to_storage.capacity - to_storage.total;
            if (deliver_value > from_storage.content[deliverer.resource])
                deliver_value = from_storage.content[deliverer.resource];
            if (deliver_value > paid_amount)
                deliver_value = paid_amount;
            if (deliver_value < 0.0)
                deliver_value = 0.0;

            to_storage.add(deliverer.resource, deliver_value);
            from_storage.add(deliverer.resource, -deliver_value);
            contract.amount -= deliver_value;
        }

        std::erase_if(deliverer.contracts, [](const deliver_contract &contract) {
            return contract.amount <= 0.0;
        });
    }
}