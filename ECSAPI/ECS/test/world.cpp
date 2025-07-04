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
    ASSERT_EQ(registered_container.content[ee_economy::RESOURCE_TYPE::COAL], 98.0);
}

TEST(WorldTest, RegisterStateDuplicate)
{
    ee_ecs::world world;

    world.register_state(10, {});

    ASSERT_THROW({
        world.register_state(10, {});
    }, std::invalid_argument);
}

TEST(WorldTest, SpawnEnterprise)
{
    ASSERT_TRUE(false);
}