#pragma once
#include <ee_economy/economy.hpp>
#include <vector>

namespace ee_economy
{
    struct deliver_contract
    {
        entt::entity from;
        entt::entity to;
        double value = 0.0;
    };

    struct deliverer
    {
        RESOURCE_TYPE resource = RESOURCE_TYPE::NONE;

        std::vector<deliver_contract> contracts;
    };
}