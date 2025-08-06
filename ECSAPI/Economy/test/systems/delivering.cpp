#include <gtest/gtest.h>
#include <ee_economy/economy.hpp>
#include "../test_conf.hpp"

TEST_F(SystemsTest, DeliverySingleNormal)
{
    // [Miner -> Storage] by Deliverer

    ee_economy::enterprise &deliverer_enterprise = registry.get<ee_economy::enterprise>(deliverer);
    ee_economy::deliverer &deliverer_deliverer = registry.get<ee_economy::deliverer>(deliverer);

    deliverer_deliverer.resource = ee_economy::RESOURCE_TYPE::COAL;
    deliverer_enterprise.set_base_efficiency(1.0);
    deliverer_deliverer.contracts.push_back({.from = miner, .to = storage, .amount = 10.0});

    ee_economy::container &from_container = registry.get<ee_economy::container>(miner);
    ee_economy::container &to_container = registry.get<ee_economy::container>(storage);
    
    from_container.capacity = 15.0;
    from_container.add(ee_economy::RESOURCE_TYPE::COAL, 15.0);

    to_container.capacity = 15.0;

    ee_economy::delivering(registry);

    ASSERT_DOUBLE_EQ(from_container.total, 14.0);
    ASSERT_DOUBLE_EQ(from_container.content[ee_economy::RESOURCE_TYPE::COAL], 14.0);
    ASSERT_DOUBLE_EQ(to_container.total, 1.0);
    ASSERT_DOUBLE_EQ(to_container.content[ee_economy::RESOURCE_TYPE::COAL], 1.0);
    ASSERT_EQ(deliverer_deliverer.contracts.size(), 1);
}

TEST_F(SystemsTest, DeliverySingleEmpty)
{
    // [Miner -> Storage] by Deliverer

    ee_economy::enterprise &deliverer_enterprise = registry.get<ee_economy::enterprise>(deliverer);
    ee_economy::deliverer &deliverer_deliverer = registry.get<ee_economy::deliverer>(deliverer);

    deliverer_deliverer.resource = ee_economy::RESOURCE_TYPE::COAL;
    deliverer_enterprise.set_base_efficiency(1.0);
    deliverer_deliverer.contracts.push_back({.from = miner, .to = storage, .amount = 10.0});

    ee_economy::container &from_container = registry.get<ee_economy::container>(miner);
    ee_economy::container &to_container = registry.get<ee_economy::container>(storage);
    
    from_container.capacity = 15.0;
    to_container.capacity = 15.0;

    ee_economy::delivering(registry);

    ASSERT_DOUBLE_EQ(from_container.total, 0.0);
    ASSERT_DOUBLE_EQ(from_container.content[ee_economy::RESOURCE_TYPE::COAL], 0.0);
    ASSERT_DOUBLE_EQ(to_container.total, 0.0);
    ASSERT_DOUBLE_EQ(to_container.content[ee_economy::RESOURCE_TYPE::COAL], 0.0);
    ASSERT_EQ(deliverer_deliverer.contracts.size(), 1);
}

TEST_F(SystemsTest, DeliverySinglePoor)
{
    // [Miner -> Storage] by Deliverer

    ee_economy::enterprise &deliverer_enterprise = registry.get<ee_economy::enterprise>(deliverer);
    ee_economy::deliverer &deliverer_deliverer = registry.get<ee_economy::deliverer>(deliverer);

    deliverer_deliverer.resource = ee_economy::RESOURCE_TYPE::COAL;
    deliverer_enterprise.set_base_efficiency(1.0);
    deliverer_deliverer.contracts.push_back({.from = miner, .to = storage, .amount = 10.0});

    ee_economy::container &from_container = registry.get<ee_economy::container>(miner);
    ee_economy::container &to_container = registry.get<ee_economy::container>(storage);
    
    from_container.capacity = 15.0;
    from_container.add(ee_economy::RESOURCE_TYPE::COAL, 0.5);

    to_container.capacity = 15.0;

    ee_economy::delivering(registry);

    ASSERT_DOUBLE_EQ(from_container.total, 0.0);
    ASSERT_DOUBLE_EQ(from_container.content[ee_economy::RESOURCE_TYPE::COAL], 0.0);
    ASSERT_DOUBLE_EQ(to_container.total, 0.5);
    ASSERT_DOUBLE_EQ(to_container.content[ee_economy::RESOURCE_TYPE::COAL], 0.5);
    ASSERT_EQ(deliverer_deliverer.contracts.size(), 1);
}

TEST_F(SystemsTest, DeliverySingleFinishContract)
{
    // [Miner -> Storage] by Deliverer

    ee_economy::enterprise &deliverer_enterprise = registry.get<ee_economy::enterprise>(deliverer);
    ee_economy::deliverer &deliverer_deliverer = registry.get<ee_economy::deliverer>(deliverer);

    deliverer_deliverer.resource = ee_economy::RESOURCE_TYPE::COAL;
    deliverer_enterprise.set_base_efficiency(15.0);
    deliverer_deliverer.contracts.push_back({.from = miner, .to = storage, .amount = 10.0});

    ee_economy::container &from_container = registry.get<ee_economy::container>(miner);
    ee_economy::container &to_container = registry.get<ee_economy::container>(storage);
    
    from_container.capacity = 15.0;
    from_container.add(ee_economy::RESOURCE_TYPE::COAL, 15.0);

    to_container.capacity = 15.0;

    ee_economy::delivering(registry);

    ASSERT_DOUBLE_EQ(from_container.total, 5.0);
    ASSERT_DOUBLE_EQ(from_container.content[ee_economy::RESOURCE_TYPE::COAL], 5.0);
    ASSERT_DOUBLE_EQ(to_container.total, 10.0);
    ASSERT_DOUBLE_EQ(to_container.content[ee_economy::RESOURCE_TYPE::COAL], 10.0);
    ASSERT_EQ(deliverer_deliverer.contracts.size(), 0);
}

TEST_F(SystemsTest, DeliveryMultipleFinishPoorCycle)
{
    // [Miner -> Storage | Storage -> Miner] by Deliverer

    ee_economy::enterprise &deliverer_enterprise = registry.get<ee_economy::enterprise>(deliverer);
    ee_economy::deliverer &deliverer_deliverer = registry.get<ee_economy::deliverer>(deliverer);

    deliverer_deliverer.resource = ee_economy::RESOURCE_TYPE::COAL;
    deliverer_enterprise.set_base_efficiency(16.0);
    deliverer_deliverer.contracts.push_back({.from = miner, .to = storage, .amount = 20.0});
    deliverer_deliverer.contracts.push_back({.from = storage, .to = miner, .amount = 5.0});

    ee_economy::container &miner_container = registry.get<ee_economy::container>(miner);
    ee_economy::container &storage_container = registry.get<ee_economy::container>(storage);
    
    miner_container.capacity = 15.0;
    miner_container.add(ee_economy::RESOURCE_TYPE::COAL, 15.0);

    storage_container.capacity = 15.0;

    ee_economy::delivering(registry);

    ASSERT_DOUBLE_EQ(miner_container.total, 5.0);
    ASSERT_DOUBLE_EQ(miner_container.content[ee_economy::RESOURCE_TYPE::COAL], 5.0);
    ASSERT_DOUBLE_EQ(storage_container.total, 10.0);
    ASSERT_DOUBLE_EQ(storage_container.content[ee_economy::RESOURCE_TYPE::COAL], 10.0);
    ASSERT_EQ(deliverer_deliverer.contracts.size(), 1);
    ASSERT_EQ(deliverer_deliverer.contracts[0].from, miner);
}

TEST_F(SystemsTest, DeliveryPayment)
{
    // [Miner -> Storage] by Deliverer for $Miner
    // [Miner -> Storage] by Deliverer

    ee_economy::enterprise &deliverer_enterprise = registry.get<ee_economy::enterprise>(deliverer);
    ee_economy::deliverer &deliverer_deliverer = registry.get<ee_economy::deliverer>(deliverer);

    deliverer_deliverer.resource = ee_economy::RESOURCE_TYPE::COAL;
    deliverer_enterprise.set_base_efficiency(10.0);
    deliverer_deliverer.contracts.push_back({
        .from = miner,
        .to = storage,
        .sponsor = miner,
        .amount = 10.0,
        .cost = 100.0
    });

    ee_economy::container &from_container = registry.get<ee_economy::container>(miner);
    ee_economy::enterprise &from_enterprise = registry.get<ee_economy::enterprise>(miner);
    ee_economy::container &to_container = registry.get<ee_economy::container>(storage);
    
    from_container.capacity = 10.0;
    from_container.add(ee_economy::RESOURCE_TYPE::COAL, 10.0);
    from_enterprise.set_money(95.0);

    to_container.capacity = 10.0;

    ee_economy::delivering(registry);

    ASSERT_DOUBLE_EQ(from_container.total, 1.0);
    ASSERT_DOUBLE_EQ(from_container.content[ee_economy::RESOURCE_TYPE::COAL], 1.0);
    ASSERT_DOUBLE_EQ(to_container.total, 9.0);
    ASSERT_DOUBLE_EQ(to_container.content[ee_economy::RESOURCE_TYPE::COAL], 9.0);
    ASSERT_DOUBLE_EQ(from_enterprise.total_money, 5.0);
    ASSERT_DOUBLE_EQ(deliverer_enterprise.total_money, 90.0);

    ee_economy::deliver_contract contract = deliverer_deliverer.contracts[0];
    ASSERT_DOUBLE_EQ(contract.cost, 10.0);
    ASSERT_DOUBLE_EQ(contract.amount, 1.0);
}

TEST_F(SystemsTest, DeliveryBankrupt)
{
    // [Miner -> Storage] by Deliverer

    ee_economy::enterprise &deliverer_enterprise = registry.get<ee_economy::enterprise>(deliverer);
    ee_economy::deliverer &deliverer_deliverer = registry.get<ee_economy::deliverer>(deliverer);

    deliverer_deliverer.resource = ee_economy::RESOURCE_TYPE::COAL;
    deliverer_deliverer.contracts.push_back({.from = miner, .to = storage, .amount = 10.0});
    deliverer_enterprise.set_base_efficiency(1.0);
    deliverer_enterprise.is_bankrupt = true;

    ee_economy::container &from_container = registry.get<ee_economy::container>(miner);
    ee_economy::container &to_container = registry.get<ee_economy::container>(storage);
    
    from_container.capacity = 15.0;
    from_container.add(ee_economy::RESOURCE_TYPE::COAL, 15.0);

    to_container.capacity = 15.0;

    ee_economy::delivering(registry);

    ASSERT_DOUBLE_EQ(from_container.total, 15.0);
    ASSERT_DOUBLE_EQ(from_container.content[ee_economy::RESOURCE_TYPE::COAL], 15.0);
    ASSERT_DOUBLE_EQ(to_container.total, 0.0);
    ASSERT_DOUBLE_EQ(to_container.content[ee_economy::RESOURCE_TYPE::COAL], 0.0);
    ASSERT_EQ(deliverer_deliverer.contracts.size(), 1);
}