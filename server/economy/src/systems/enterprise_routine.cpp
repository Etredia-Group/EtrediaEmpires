#include <ee_economy/economy.hpp>

void ee_economy::enterpriseRoutine(entt::registry &registry)
{
    auto enterprises = registry.view<Enterprise>();

    for (auto [entity, enterprise]: enterprises.each())
    {
        double salaryDebt = enterprise.salary * enterprise.staffSize;
        if (enterprise.totalMoney < salaryDebt)
        {
            enterprise.isBankrupt = true;
            enterprise.setMoney(0.0);
        }
        else
        {
            enterprise.setMoney(enterprise.totalMoney - salaryDebt);
        }
    }
}