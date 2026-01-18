#include <gtest/gtest.h>
#include <ee_economy/economy.hpp>
#include "../test_conf.hpp"

TEST_F(SystemsTest, DeliverySingleNormal)
{
    // [Miner -> Storage] by Deliverer

    ee_economy::Enterprise &delivererEnterprise = registry.get<ee_economy::Enterprise>(deliverer);
    ee_economy::Deliverer &delivererDeliverer = registry.get<ee_economy::Deliverer>(deliverer);

    delivererDeliverer.resource = ee_economy::RESOURCE_TYPE::COAL;
    delivererEnterprise.setBaseEfficiency(1.0);
    delivererDeliverer.contracts.push_back({.from = miner, .to = storage, .amount = 10.0});

    ee_economy::Container &fromContainer = registry.get<ee_economy::Container>(miner);
    ee_economy::Container &toContainer = registry.get<ee_economy::Container>(storage);
    
    fromContainer.capacity = 15.0;
    fromContainer.add(ee_economy::RESOURCE_TYPE::COAL, 15.0);

    toContainer.capacity = 15.0;

    ee_economy::delivering(registry);

    ASSERT_DOUBLE_EQ(fromContainer.total, 14.0);
    ASSERT_DOUBLE_EQ(fromContainer.content[ee_economy::RESOURCE_TYPE::COAL], 14.0);
    ASSERT_DOUBLE_EQ(toContainer.total, 1.0);
    ASSERT_DOUBLE_EQ(toContainer.content[ee_economy::RESOURCE_TYPE::COAL], 1.0);
    ASSERT_EQ(delivererDeliverer.contracts.size(), 1);
}

TEST_F(SystemsTest, DeliverySingleEmpty)
{
    // [Miner -> Storage] by Deliverer

    ee_economy::Enterprise &delivererEnterprise = registry.get<ee_economy::Enterprise>(deliverer);
    ee_economy::Deliverer &delivererDeliverer = registry.get<ee_economy::Deliverer>(deliverer);

    delivererDeliverer.resource = ee_economy::RESOURCE_TYPE::COAL;
    delivererEnterprise.setBaseEfficiency(1.0);
    delivererDeliverer.contracts.push_back({.from = miner, .to = storage, .amount = 10.0});

    ee_economy::Container &fromContainer = registry.get<ee_economy::Container>(miner);
    ee_economy::Container &toContainer = registry.get<ee_economy::Container>(storage);
    
    fromContainer.capacity = 15.0;
    toContainer.capacity = 15.0;

    ee_economy::delivering(registry);

    ASSERT_DOUBLE_EQ(fromContainer.total, 0.0);
    ASSERT_DOUBLE_EQ(fromContainer.content[ee_economy::RESOURCE_TYPE::COAL], 0.0);
    ASSERT_DOUBLE_EQ(toContainer.total, 0.0);
    ASSERT_DOUBLE_EQ(toContainer.content[ee_economy::RESOURCE_TYPE::COAL], 0.0);
    ASSERT_EQ(delivererDeliverer.contracts.size(), 1);
}

TEST_F(SystemsTest, DeliverySinglePoor)
{
    // [Miner -> Storage] by Deliverer

    ee_economy::Enterprise &delivererEnterprise = registry.get<ee_economy::Enterprise>(deliverer);
    ee_economy::Deliverer &delivererDeliverer = registry.get<ee_economy::Deliverer>(deliverer);

    delivererDeliverer.resource = ee_economy::RESOURCE_TYPE::COAL;
    delivererEnterprise.setBaseEfficiency(1.0);
    delivererDeliverer.contracts.push_back({.from = miner, .to = storage, .amount = 10.0});

    ee_economy::Container &fromContainer = registry.get<ee_economy::Container>(miner);
    ee_economy::Container &toContainer = registry.get<ee_economy::Container>(storage);
    
    fromContainer.capacity = 15.0;
    fromContainer.add(ee_economy::RESOURCE_TYPE::COAL, 0.5);

    toContainer.capacity = 15.0;

    ee_economy::delivering(registry);

    ASSERT_DOUBLE_EQ(fromContainer.total, 0.0);
    ASSERT_DOUBLE_EQ(fromContainer.content[ee_economy::RESOURCE_TYPE::COAL], 0.0);
    ASSERT_DOUBLE_EQ(toContainer.total, 0.5);
    ASSERT_DOUBLE_EQ(toContainer.content[ee_economy::RESOURCE_TYPE::COAL], 0.5);
    ASSERT_EQ(delivererDeliverer.contracts.size(), 1);
}

TEST_F(SystemsTest, DeliverySingleFinishContract)
{
    // [Miner -> Storage] by Deliverer

    ee_economy::Enterprise &delivererEnterprise = registry.get<ee_economy::Enterprise>(deliverer);
    ee_economy::Deliverer &delivererDeliverer = registry.get<ee_economy::Deliverer>(deliverer);

    delivererDeliverer.resource = ee_economy::RESOURCE_TYPE::COAL;
    delivererEnterprise.setBaseEfficiency(15.0);
    delivererDeliverer.contracts.push_back({.from = miner, .to = storage, .amount = 10.0});

    ee_economy::Container &fromContainer = registry.get<ee_economy::Container>(miner);
    ee_economy::Container &toContainer = registry.get<ee_economy::Container>(storage);
    
    fromContainer.capacity = 15.0;
    fromContainer.add(ee_economy::RESOURCE_TYPE::COAL, 15.0);

    toContainer.capacity = 15.0;

    ee_economy::delivering(registry);

    ASSERT_DOUBLE_EQ(fromContainer.total, 5.0);
    ASSERT_DOUBLE_EQ(fromContainer.content[ee_economy::RESOURCE_TYPE::COAL], 5.0);
    ASSERT_DOUBLE_EQ(toContainer.total, 10.0);
    ASSERT_DOUBLE_EQ(toContainer.content[ee_economy::RESOURCE_TYPE::COAL], 10.0);
    ASSERT_EQ(delivererDeliverer.contracts.size(), 0);
}

TEST_F(SystemsTest, DeliveryMultipleFinishPoorCycle)
{
    // [Miner -> Storage | Storage -> Miner] by Deliverer

    ee_economy::Enterprise &delivererEnterprise = registry.get<ee_economy::Enterprise>(deliverer);
    ee_economy::Deliverer &delivererDeliverer = registry.get<ee_economy::Deliverer>(deliverer);

    delivererDeliverer.resource = ee_economy::RESOURCE_TYPE::COAL;
    delivererEnterprise.setBaseEfficiency(16.0);
    delivererDeliverer.contracts.push_back({.from = miner, .to = storage, .amount = 20.0});
    delivererDeliverer.contracts.push_back({.from = storage, .to = miner, .amount = 5.0});

    ee_economy::Container &minerContainer = registry.get<ee_economy::Container>(miner);
    ee_economy::Container &storageContainer = registry.get<ee_economy::Container>(storage);
    
    minerContainer.capacity = 15.0;
    minerContainer.add(ee_economy::RESOURCE_TYPE::COAL, 15.0);

    storageContainer.capacity = 15.0;

    ee_economy::delivering(registry);

    ASSERT_DOUBLE_EQ(minerContainer.total, 5.0);
    ASSERT_DOUBLE_EQ(minerContainer.content[ee_economy::RESOURCE_TYPE::COAL], 5.0);
    ASSERT_DOUBLE_EQ(storageContainer.total, 10.0);
    ASSERT_DOUBLE_EQ(storageContainer.content[ee_economy::RESOURCE_TYPE::COAL], 10.0);
    ASSERT_EQ(delivererDeliverer.contracts.size(), 1);
    ASSERT_EQ(delivererDeliverer.contracts[0].from, miner);
}

TEST_F(SystemsTest, DeliveryPayment)
{
    // [Miner -> Storage] by Deliverer for $Miner
    // [Miner -> Storage] by Deliverer

    ee_economy::Enterprise &delivererEnterprise = registry.get<ee_economy::Enterprise>(deliverer);
    ee_economy::Deliverer &delivererDeliverer = registry.get<ee_economy::Deliverer>(deliverer);

    delivererDeliverer.resource = ee_economy::RESOURCE_TYPE::COAL;
    delivererEnterprise.setBaseEfficiency(10.0);
    delivererDeliverer.contracts.push_back({
        .from = miner,
        .to = storage,
        .sponsor = miner,
        .amount = 10.0,
        .cost = 100.0
    });

    ee_economy::Container &fromContainer = registry.get<ee_economy::Container>(miner);
    ee_economy::Enterprise &fromEnterprise = registry.get<ee_economy::Enterprise>(miner);
    ee_economy::Container &toContainer = registry.get<ee_economy::Container>(storage);
    
    fromContainer.capacity = 10.0;
    fromContainer.add(ee_economy::RESOURCE_TYPE::COAL, 10.0);
    fromEnterprise.setMoney(95.0);

    toContainer.capacity = 10.0;

    ee_economy::delivering(registry);

    ASSERT_DOUBLE_EQ(fromContainer.total, 1.0);
    ASSERT_DOUBLE_EQ(fromContainer.content[ee_economy::RESOURCE_TYPE::COAL], 1.0);
    ASSERT_DOUBLE_EQ(toContainer.total, 9.0);
    ASSERT_DOUBLE_EQ(toContainer.content[ee_economy::RESOURCE_TYPE::COAL], 9.0);
    ASSERT_DOUBLE_EQ(fromEnterprise.totalMoney, 5.0);
    ASSERT_DOUBLE_EQ(delivererEnterprise.totalMoney, 90.0);

    ee_economy::DeliverContract contract = delivererDeliverer.contracts[0];
    ASSERT_DOUBLE_EQ(contract.cost, 10.0);
    ASSERT_DOUBLE_EQ(contract.amount, 1.0);
}

TEST_F(SystemsTest, DeliveryBankrupt)
{
    // [Miner -> Storage] by Deliverer

    ee_economy::Enterprise &delivererEnterprise = registry.get<ee_economy::Enterprise>(deliverer);
    ee_economy::Deliverer &delivererDeliverer = registry.get<ee_economy::Deliverer>(deliverer);

    delivererDeliverer.resource = ee_economy::RESOURCE_TYPE::COAL;
    delivererDeliverer.contracts.push_back({.from = miner, .to = storage, .amount = 10.0});
    delivererEnterprise.setBaseEfficiency(1.0);
    delivererEnterprise.isBankrupt = true;

    ee_economy::Container &fromContainer = registry.get<ee_economy::Container>(miner);
    ee_economy::Container &toContainer = registry.get<ee_economy::Container>(storage);
    
    fromContainer.capacity = 15.0;
    fromContainer.add(ee_economy::RESOURCE_TYPE::COAL, 15.0);

    toContainer.capacity = 15.0;

    ee_economy::delivering(registry);

    ASSERT_DOUBLE_EQ(fromContainer.total, 15.0);
    ASSERT_DOUBLE_EQ(fromContainer.content[ee_economy::RESOURCE_TYPE::COAL], 15.0);
    ASSERT_DOUBLE_EQ(toContainer.total, 0.0);
    ASSERT_DOUBLE_EQ(toContainer.content[ee_economy::RESOURCE_TYPE::COAL], 0.0);
    ASSERT_EQ(delivererDeliverer.contracts.size(), 1);
}