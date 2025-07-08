#include <gtest/gtest.h>
#include <ee_economy/economy.hpp>
#include "../test_conf.hpp"

TEST_F(SystemsTest, DeliverySingleNormal)
{
    // [Miner -> Storage] by Deliverer

    ee_economy::enterprise &deliverer_enterprise = registry.get<ee_economy::enterprise>(deliverer);
    ee_economy::deliverer &deliverer_deliverer = registry.get<ee_economy::deliverer>(deliverer);

    deliverer_deliverer.resource = ee_economy::RESOURCE_TYPE::COAL;
    deliverer_enterprise.add_base_efficiency(1.0);
    deliverer_deliverer.contracts.push_back({.from = miner, .to = storage, .value = 10.0});

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
}

TEST_F(SystemsTest, DeliverySingleEmpty)
{
    // [Miner -> Storage] by Deliverer

    ee_economy::enterprise &deliverer_enterprise = registry.get<ee_economy::enterprise>(deliverer);
    ee_economy::deliverer &deliverer_deliverer = registry.get<ee_economy::deliverer>(deliverer);

    deliverer_deliverer.resource = ee_economy::RESOURCE_TYPE::COAL;
    deliverer_enterprise.add_base_efficiency(1.0);
    deliverer_deliverer.contracts.push_back({.from = miner, .to = storage, .value = 10.0});

    ee_economy::container &from_container = registry.get<ee_economy::container>(miner);
    ee_economy::container &to_container = registry.get<ee_economy::container>(storage);
    
    from_container.capacity = 15.0;
    to_container.capacity = 15.0;

    ee_economy::delivering(registry);

    ASSERT_DOUBLE_EQ(from_container.total, 0.0);
    ASSERT_DOUBLE_EQ(from_container.content[ee_economy::RESOURCE_TYPE::COAL], 0.0);
    ASSERT_DOUBLE_EQ(to_container.total, 0.0);
    ASSERT_DOUBLE_EQ(to_container.content[ee_economy::RESOURCE_TYPE::COAL], 0.0);
}

TEST_F(SystemsTest, DeliverySinglePoor)
{
    // [Miner -> Storage] by Deliverer

    ee_economy::enterprise &deliverer_enterprise = registry.get<ee_economy::enterprise>(deliverer);
    ee_economy::deliverer &deliverer_deliverer = registry.get<ee_economy::deliverer>(deliverer);

    deliverer_deliverer.resource = ee_economy::RESOURCE_TYPE::COAL;
    deliverer_enterprise.add_base_efficiency(1.0);
    deliverer_deliverer.contracts.push_back({.from = miner, .to = storage, .value = 10.0});

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
}

TEST_F(SystemsTest, DeliverySingleFinishContract)
{
    // [Miner -> Storage] by Deliverer

    ee_economy::enterprise &deliverer_enterprise = registry.get<ee_economy::enterprise>(deliverer);
    ee_economy::deliverer &deliverer_deliverer = registry.get<ee_economy::deliverer>(deliverer);

    deliverer_deliverer.resource = ee_economy::RESOURCE_TYPE::COAL;
    deliverer_enterprise.add_base_efficiency(16.0);
    deliverer_deliverer.contracts.push_back({.from = miner, .to = storage, .value = 10.0});

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