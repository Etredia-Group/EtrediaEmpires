#include <ee_economy/economy.hpp>

void ee_economy::enterprise_routine(entt::registry &registry)
{
    auto enterprises = registry.view<enterprise>();

    for (auto [entity, enterprise]: enterprises.each())
    {
        double salary_debt = enterprise.salary * enterprise.staff_size;
        if (enterprise.total_money < salary_debt)
        {
            enterprise.is_bankrupt = true;
            enterprise.set_money(0.0);
        }
        else
        {
            enterprise.set_money(enterprise.total_money - salary_debt);
        }
    }
}