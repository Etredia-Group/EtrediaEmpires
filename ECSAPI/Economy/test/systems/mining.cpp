#include <gtest/gtest.h>
#include <ee_economy/economy.hpp>
#include "../test_conf.hpp"

TEST_F(SystemsTest, MiningNormal)
{
    ee_ecs::container &state_container = registry.get<ee_ecs::container>(state);
    ee_ecs::container &miner_container = registry.get<ee_ecs::container>(miner);
    ee_ecs::miner &miner_miner = registry.get<ee_ecs::miner>(miner);
    
    state_container.capacity = 15.0;
    state_container.add(ee_ecs::RESOURCE_TYPE::COAL, 10.0);

    miner_container.capacity = 5.0;
    miner_miner.resource = ee_ecs::RESOURCE_TYPE::COAL;
    miner_miner.value = 0.1;

    ee_ecs::mining(registry);

    ASSERT_DOUBLE_EQ(state_container.total, 9.9);
    ASSERT_DOUBLE_EQ(state_container.content[ee_ecs::RESOURCE_TYPE::COAL], 9.9);
    ASSERT_DOUBLE_EQ(miner_container.total, 0.1);
    ASSERT_DOUBLE_EQ(miner_container.content[ee_ecs::RESOURCE_TYPE::COAL], 0.1);
}

TEST_F(SystemsTest, MiningEmptyState)
{
    ee_ecs::container &state_container = registry.get<ee_ecs::container>(state);
    ee_ecs::container &miner_container = registry.get<ee_ecs::container>(miner);
    ee_ecs::miner &miner_miner = registry.get<ee_ecs::miner>(miner);
    
    state_container.capacity = 15.0;

    miner_container.capacity = 5.0;
    miner_miner.resource = ee_ecs::RESOURCE_TYPE::COAL;
    miner_miner.value = 0.1;

    ee_ecs::mining(registry);

    ASSERT_DOUBLE_EQ(state_container.total, 0.0);
    ASSERT_DOUBLE_EQ(state_container.content[ee_ecs::RESOURCE_TYPE::COAL], 0.0);
    ASSERT_DOUBLE_EQ(miner_container.total, 0.0);
    ASSERT_DOUBLE_EQ(miner_container.content[ee_ecs::RESOURCE_TYPE::COAL], 0.0);
}

TEST_F(SystemsTest, MiningPoorState)
{
    ee_ecs::container &state_container = registry.get<ee_ecs::container>(state);
    ee_ecs::container &miner_container = registry.get<ee_ecs::container>(miner);
    ee_ecs::miner &miner_miner = registry.get<ee_ecs::miner>(miner);
    
    state_container.capacity = 15.0;
    state_container.add(ee_ecs::RESOURCE_TYPE::COAL, 0.05);

    miner_container.capacity = 5.0;
    miner_miner.resource = ee_ecs::RESOURCE_TYPE::COAL;
    miner_miner.value = 0.1;

    ee_ecs::mining(registry);

    ASSERT_DOUBLE_EQ(state_container.total, 0.0);
    ASSERT_DOUBLE_EQ(state_container.content[ee_ecs::RESOURCE_TYPE::COAL], 0.0);
    ASSERT_DOUBLE_EQ(miner_container.total, 0.05);
    ASSERT_DOUBLE_EQ(miner_container.content[ee_ecs::RESOURCE_TYPE::COAL], 0.05);
}

TEST_F(SystemsTest, MiningReachCap)
{
    ee_ecs::container &state_container = registry.get<ee_ecs::container>(state);
    ee_ecs::container &miner_container = registry.get<ee_ecs::container>(miner);
    ee_ecs::miner &miner_miner = registry.get<ee_ecs::miner>(miner);
    
    state_container.capacity = 15.0;
    state_container.add(ee_ecs::RESOURCE_TYPE::COAL, 10.0);

    miner_container.capacity = 0.05;
    miner_miner.resource = ee_ecs::RESOURCE_TYPE::COAL;
    miner_miner.value = 0.1;

    ee_ecs::mining(registry);

    ASSERT_DOUBLE_EQ(state_container.total, 9.95);
    ASSERT_DOUBLE_EQ(state_container.content[ee_ecs::RESOURCE_TYPE::COAL], 9.95);
    ASSERT_DOUBLE_EQ(miner_container.total, 0.05);
    ASSERT_DOUBLE_EQ(miner_container.content[ee_ecs::RESOURCE_TYPE::COAL], 0.05);
}