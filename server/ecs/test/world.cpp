#include <gtest/gtest.h>
#include <ee_ecs/ecs.hpp>
#include <ee_economy/economy.hpp>
#include <stdexcept>

TEST(WorldTest, RegisterState)
{
    ee_ecs::World world;

    ee_economy::Container res {.capacity = 100.0};
    res.add(ee_economy::RESOURCE_TYPE::COAL, 98.0);
    entt::entity state = world.registerState(10, res);

    ee_economy::State registeredState = world.getRegistry().get<ee_economy::State>(state);
    ee_economy::Container registeredContainer = world.getRegistry().get<ee_economy::Container>(state);

    ASSERT_EQ(registeredState.externalId, 10);
    ASSERT_DOUBLE_EQ(registeredContainer.content[ee_economy::RESOURCE_TYPE::COAL], 98.0);
}

TEST(WorldTest, RegisterStateDuplicate)
{
    ee_ecs::World world;

    world.registerState(10, {});

    ASSERT_THROW({
        world.registerState(10, {});
    }, std::invalid_argument);
}

TEST(WorldTest, SpawnMiner)
{
    ee_ecs::World world;

    entt::entity state = world.getRegistry().create();
    world.getRegistry().emplace<ee_economy::State>(state, (int16_t)0);
    world.getRegistry().emplace<ee_economy::Container>(state, 100.0);

    ee_economy::EnterpriseDefaults defaults {
        .container {.capacity = 100.0 },
        .resourceTarget = ee_economy::RESOURCE_TYPE::COAL,
        .baseEfficiency = 1.0,
    };

    entt::entity enterprise = world.spawnEnterprise(false, ee_economy::ACTION_TYPE::MINING, state, defaults);

    ee_economy::Enterprise registeredEnterprise = world.getRegistry().get<ee_economy::Enterprise>(enterprise);
    ee_economy::Miner registeredAction = world.getRegistry().get<ee_economy::Miner>(enterprise);
    ee_economy::Container registeredContainer = world.getRegistry().get<ee_economy::Container>(enterprise);

    ASSERT_DOUBLE_EQ(registeredEnterprise.baseEfficiency, 1.0);
    ASSERT_EQ(registeredAction.resource, ee_economy::RESOURCE_TYPE::COAL);
    ASSERT_DOUBLE_EQ(registeredContainer.capacity, 100.0);
}

TEST(WorldTest, SpawnDeliverer)
{
    ee_ecs::World world;

    entt::entity state = world.getRegistry().create();
    world.getRegistry().emplace<ee_economy::State>(state, (int16_t)0);
    world.getRegistry().emplace<ee_economy::Container>(state, 100.0);

    ee_economy::EnterpriseDefaults defaults {
        .container {.capacity = 100.0 },
        .resourceTarget = ee_economy::RESOURCE_TYPE::COAL,
        .baseEfficiency = 1.0,
    };

    entt::entity enterprise = world.spawnEnterprise(false, ee_economy::ACTION_TYPE::DELIVERY, state, defaults);

    ee_economy::Enterprise registeredEnterprise = world.getRegistry().get<ee_economy::Enterprise>(enterprise);
    ee_economy::Deliverer registeredAction = world.getRegistry().get<ee_economy::Deliverer>(enterprise);
    ee_economy::Container registeredContainer = world.getRegistry().get<ee_economy::Container>(enterprise);

    ASSERT_DOUBLE_EQ(registeredEnterprise.baseEfficiency, 1.0);
    ASSERT_EQ(registeredAction.resource, ee_economy::RESOURCE_TYPE::COAL);
    ASSERT_DOUBLE_EQ(registeredContainer.capacity, 100.0);
}