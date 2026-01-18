#pragma once
#include <ee_economy/economy.hpp>
#include <vector>

namespace ee_economy
{
    struct DeliverContract
    {
        entt::entity from;
        entt::entity to;
        entt::entity sponsor;

        double amount = 0.0;
        double cost = 0.0;
        double distance = 1.0;
    };

    struct Deliverer
    {
        RESOURCE_TYPE resource = RESOURCE_TYPE::NONE;

        std::vector<DeliverContract> contracts;
    };
}