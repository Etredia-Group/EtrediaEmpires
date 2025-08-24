#pragma once

#include <gtest/gtest.h>
#include <ee_ecs/ecs.hpp>

struct WorldBenchmark : testing::Test
{
    ee_ecs::World world;

    void SetUp()
    {
        for (int16_t i = 0; i < 1000; i++)
        {
            ee_economy::Container resources {.capacity = 100.0};
            resources.add(ee_economy::RESOURCE_TYPE::COAL, 100.0);
            entt::entity state = world.registerState(i, resources);

            ee_economy::EnterpriseDefaults miner {
                .container {.capacity = 100.0},
                .resourceTarget = ee_economy::RESOURCE_TYPE::GOLD,
                .baseEfficiency = 1.0
            };
            
            world.spawnEnterprise(false, ee_economy::ACTION_TYPE::MINING, state, miner);
            world.spawnEnterprise(false, ee_economy::ACTION_TYPE::MINING, state, miner);
            world.spawnEnterprise(false, ee_economy::ACTION_TYPE::MINING, state, miner);
            world.spawnEnterprise(false, ee_economy::ACTION_TYPE::MINING, state, miner);
            world.spawnEnterprise(false, ee_economy::ACTION_TYPE::MINING, state, miner);
        }
    }

    void TearDown()
    {

    }
};