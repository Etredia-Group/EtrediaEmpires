#include <gtest/gtest.h>
#include <ee_economy/economy.hpp>
#include "../test_conf.hpp"

TEST_F(SystemsTest, MiningNormal)
{
    ee_economy::Container &stateContainer = registry.get<ee_economy::Container>(state);
    ee_economy::Container &minerContainer = registry.get<ee_economy::Container>(miner);
    ee_economy::Enterprise &minerEnterprise = registry.get<ee_economy::Enterprise>(miner);
    ee_economy::Miner &minerMiner = registry.get<ee_economy::Miner>(miner);
    
    stateContainer.capacity = 15.0;
    stateContainer.add(ee_economy::RESOURCE_TYPE::COAL, 10.0);

    minerContainer.capacity = 5.0;
    minerMiner.resource = ee_economy::RESOURCE_TYPE::COAL;
    minerEnterprise.setBaseEfficiency(0.1);

    ee_economy::mining(registry);

    ASSERT_DOUBLE_EQ(stateContainer.total, 9.9);
    ASSERT_DOUBLE_EQ(stateContainer.content[ee_economy::RESOURCE_TYPE::COAL], 9.9);
    ASSERT_DOUBLE_EQ(minerContainer.total, 0.1);
    ASSERT_DOUBLE_EQ(minerContainer.content[ee_economy::RESOURCE_TYPE::COAL], 0.1);
}

TEST_F(SystemsTest, MiningEmptyState)
{
    ee_economy::Container &stateContainer = registry.get<ee_economy::Container>(state);
    ee_economy::Container &minerContainer = registry.get<ee_economy::Container>(miner);
    ee_economy::Enterprise &minerEnterprise = registry.get<ee_economy::Enterprise>(miner);
    ee_economy::Miner &minerMiner = registry.get<ee_economy::Miner>(miner);
    
    stateContainer.capacity = 15.0;

    minerContainer.capacity = 5.0;
    minerMiner.resource = ee_economy::RESOURCE_TYPE::COAL;
    minerEnterprise.setBaseEfficiency(0.1);

    ee_economy::mining(registry);

    ASSERT_DOUBLE_EQ(stateContainer.total, 0.0);
    ASSERT_DOUBLE_EQ(stateContainer.content[ee_economy::RESOURCE_TYPE::COAL], 0.0);
    ASSERT_DOUBLE_EQ(minerContainer.total, 0.0);
    ASSERT_DOUBLE_EQ(minerContainer.content[ee_economy::RESOURCE_TYPE::COAL], 0.0);
}

TEST_F(SystemsTest, MiningPoorState)
{
    ee_economy::Container &stateContainer = registry.get<ee_economy::Container>(state);
    ee_economy::Container &minerContainer = registry.get<ee_economy::Container>(miner);
    ee_economy::Enterprise &minerEnterprise = registry.get<ee_economy::Enterprise>(miner);
    ee_economy::Miner &minerMiner = registry.get<ee_economy::Miner>(miner);
    
    stateContainer.capacity = 15.0;
    stateContainer.add(ee_economy::RESOURCE_TYPE::COAL, 0.05);

    minerContainer.capacity = 5.0;
    minerMiner.resource = ee_economy::RESOURCE_TYPE::COAL;
    minerEnterprise.setBaseEfficiency(0.1);

    ee_economy::mining(registry);

    ASSERT_DOUBLE_EQ(stateContainer.total, 0.0);
    ASSERT_DOUBLE_EQ(stateContainer.content[ee_economy::RESOURCE_TYPE::COAL], 0.0);
    ASSERT_DOUBLE_EQ(minerContainer.total, 0.05);
    ASSERT_DOUBLE_EQ(minerContainer.content[ee_economy::RESOURCE_TYPE::COAL], 0.05);
}

TEST_F(SystemsTest, MiningReachCap)
{
    ee_economy::Container &stateContainer = registry.get<ee_economy::Container>(state);
    ee_economy::Container &minerContainer = registry.get<ee_economy::Container>(miner);
    ee_economy::Enterprise &minerEnterprise = registry.get<ee_economy::Enterprise>(miner);
    ee_economy::Miner &minerMiner = registry.get<ee_economy::Miner>(miner);
    
    stateContainer.capacity = 15.0;
    stateContainer.add(ee_economy::RESOURCE_TYPE::COAL, 10.0);

    minerContainer.capacity = 0.05;
    minerMiner.resource = ee_economy::RESOURCE_TYPE::COAL;
    minerEnterprise.setBaseEfficiency(0.1);

    ee_economy::mining(registry);

    ASSERT_DOUBLE_EQ(stateContainer.total, 9.95);
    ASSERT_DOUBLE_EQ(stateContainer.content[ee_economy::RESOURCE_TYPE::COAL], 9.95);
    ASSERT_DOUBLE_EQ(minerContainer.total, 0.05);
    ASSERT_DOUBLE_EQ(minerContainer.content[ee_economy::RESOURCE_TYPE::COAL], 0.05);
}

TEST_F(SystemsTest, MiningBankrupt)
{
    ee_economy::Container &stateContainer = registry.get<ee_economy::Container>(state);
    ee_economy::Container &minerContainer = registry.get<ee_economy::Container>(miner);
    ee_economy::Enterprise &minerEnterprise = registry.get<ee_economy::Enterprise>(miner);
    ee_economy::Miner &minerMiner = registry.get<ee_economy::Miner>(miner);
    
    stateContainer.capacity = 15.0;
    stateContainer.add(ee_economy::RESOURCE_TYPE::COAL, 10.0);

    minerContainer.capacity = 5.0;
    minerMiner.resource = ee_economy::RESOURCE_TYPE::COAL;
    minerEnterprise.setBaseEfficiency(0.1);
    minerEnterprise.isBankrupt = true;

    ee_economy::mining(registry);

    ASSERT_DOUBLE_EQ(stateContainer.total, 10.0);
    ASSERT_DOUBLE_EQ(stateContainer.content[ee_economy::RESOURCE_TYPE::COAL], 10.0);
    ASSERT_DOUBLE_EQ(minerContainer.total, 0.0);
    ASSERT_DOUBLE_EQ(minerContainer.content[ee_economy::RESOURCE_TYPE::COAL], 0.0);
}