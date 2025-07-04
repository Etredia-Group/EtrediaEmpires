#pragma once

#include <gtest/gtest.h>
#include <ee_ecs/ecs.hpp>

struct WorldBenchmark : testing::Test
{
    ee_ecs::world world;

    void SetUp()
    {
        for (short i = 0; i < 1000; i++)
        {
            ee_economy::container resources = {.capacity = 100.0};
            resources.add(ee_economy::RESOURCE_TYPE::COAL, 100.0);
            entt::entity state = world.register_state(i, resources);

            ee_economy::enterprise_defaults miner = {
                .container = {.capacity = 100.0},
                .resource_target = ee_economy::RESOURCE_TYPE::GOLD,
                .base_efficiency_flat = 1.0
            };
            
            world.spawn_enterprise(false, ee_economy::ACTION_TYPE::MINING, state, miner);
            world.spawn_enterprise(false, ee_economy::ACTION_TYPE::MINING, state, miner);
            world.spawn_enterprise(false, ee_economy::ACTION_TYPE::MINING, state, miner);
            world.spawn_enterprise(false, ee_economy::ACTION_TYPE::MINING, state, miner);
            world.spawn_enterprise(false, ee_economy::ACTION_TYPE::MINING, state, miner);
        }
    }

    void TearDown()
    {

    }
};