#include <gtest/gtest.h>
#include <ee_ecs/ecs.hpp>
#include <ee_economy/economy.hpp>
#include <stdexcept>

TEST(WorldTest, RegisterState)
{
    ee_ecs::world world;

    ee_economy::container res = {.capacity = 100.0};
    res.add(ee_economy::RESOURCE_TYPE::COAL, 98.0);
    entt::entity state = world.register_state(10, res);

    ee_economy::state registered_state = world.get_registry().get<ee_economy::state>(state);
    ee_economy::container registered_container = world.get_registry().get<ee_economy::container>(state);

    ASSERT_EQ(registered_state.external_id, 10);
    ASSERT_DOUBLE_EQ(registered_container.content[ee_economy::RESOURCE_TYPE::COAL], 98.0);
}

TEST(WorldTest, RegisterStateDuplicate)
{
    ee_ecs::world world;

    world.register_state(10, {});

    ASSERT_THROW({
        world.register_state(10, {});
    }, std::invalid_argument);
}

TEST(WorldTest, SpawnMiner)
{
    ee_ecs::world world;

    entt::entity state = world.get_registry().create();
    world.get_registry().emplace<ee_economy::state>(state, (int16_t)0);
    world.get_registry().emplace<ee_economy::container>(state, 100.0);

    ee_economy::enterprise_defaults defaults = {
        .container = {.capacity = 100.0 },
        .resource_target = ee_economy::RESOURCE_TYPE::COAL,
        .base_efficiency = 1.0,
    };

    entt::entity enterprise = world.spawn_enterprise(false, ee_economy::ACTION_TYPE::MINING, state, defaults);

    ee_economy::enterprise registered_enterprise = world.get_registry().get<ee_economy::enterprise>(enterprise);
    ee_economy::miner registered_action = world.get_registry().get<ee_economy::miner>(enterprise);
    ee_economy::container registered_container = world.get_registry().get<ee_economy::container>(enterprise);

    ASSERT_DOUBLE_EQ(registered_enterprise.base_efficiency, 1.0);
    ASSERT_EQ(registered_action.resource, ee_economy::RESOURCE_TYPE::COAL);
    ASSERT_DOUBLE_EQ(registered_container.capacity, 100.0);
}

TEST(WorldTest, SpawnDeliverer)
{
    ee_ecs::world world;

    entt::entity state = world.get_registry().create();
    world.get_registry().emplace<ee_economy::state>(state, (int16_t)0);
    world.get_registry().emplace<ee_economy::container>(state, 100.0);

    ee_economy::enterprise_defaults defaults = {
        .container = {.capacity = 100.0 },
        .resource_target = ee_economy::RESOURCE_TYPE::COAL,
        .base_efficiency = 1.0,
    };

    entt::entity enterprise = world.spawn_enterprise(false, ee_economy::ACTION_TYPE::DELIVERY, state, defaults);

    ee_economy::enterprise registered_enterprise = world.get_registry().get<ee_economy::enterprise>(enterprise);
    ee_economy::deliverer registered_action = world.get_registry().get<ee_economy::deliverer>(enterprise);
    ee_economy::container registered_container = world.get_registry().get<ee_economy::container>(enterprise);

    ASSERT_DOUBLE_EQ(registered_enterprise.base_efficiency, 1.0);
    ASSERT_EQ(registered_action.resource, ee_economy::RESOURCE_TYPE::COAL);
    ASSERT_DOUBLE_EQ(registered_container.capacity, 100.0);
}