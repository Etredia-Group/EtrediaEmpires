#include <ee_economy/economy.hpp>

void ee_economy::delivering(entt::registry &registry)
{
    auto deliverers = registry.view<Enterprise, Deliverer, Container>();

    for (auto [entity, enterprise, deliverer, storage]: deliverers.each())
    {
        if (enterprise.isBankrupt) continue;

        for (DeliverContract &contract: deliverer.contracts)
        {
            double paidAmount = contract.amount;
            
            if (contract.cost > 0)
            {
                Enterprise &sponsor = registry.get<Enterprise>(contract.sponsor);
                
                double costPerOne = contract.cost / contract.amount;
                double payment = contract.cost;

                if (payment > sponsor.totalMoney)
                {
                    payment = (int)(sponsor.totalMoney / costPerOne) * costPerOne;
                    paidAmount = payment / costPerOne;
                }
                sponsor.setMoney(sponsor.totalMoney - payment);
                enterprise.setMoney(enterprise.totalMoney + payment);
                contract.cost -= payment;
            }

            Container &fromStorage = registry.get<Container>(contract.from);
            Container &toStorage = registry.get<Container>(contract.to);

            double deliverValue = enterprise.efficiency;
            if (contract.amount < enterprise.efficiency)
                deliverValue = contract.amount;
            if (toStorage.total + deliverValue > toStorage.capacity)
                deliverValue = toStorage.capacity - toStorage.total;
            if (deliverValue > fromStorage.content[deliverer.resource])
                deliverValue = fromStorage.content[deliverer.resource];
            if (deliverValue > paidAmount)
                deliverValue = paidAmount;
            if (deliverValue < 0.0)
                deliverValue = 0.0;

            toStorage.add(deliverer.resource, deliverValue);
            fromStorage.add(deliverer.resource, -deliverValue);
            contract.amount -= deliverValue;
        }

        std::erase_if(deliverer.contracts, [](const DeliverContract &contract) {
            return contract.amount <= 0.0;
        });
    }
}