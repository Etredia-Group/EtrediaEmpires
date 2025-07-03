#include "components/container.h"
#include "components/enterprise.h"
#include "components/miner.h"
#include "components/tags.h"
#include <entt/entt.hpp>
#include <gtest/gtest.h>

struct SystemsTest : testing::Test
{
    entt::registry registry;

    entt::entity state;

    entt::entity miner;

    void SetUp()
    {
        // Create state
        state = registry.create();
        registry.emplace<ee_ecs::state>(state);
        registry.emplace<ee_ecs::container>(state);

        // Create miner enterprise
        miner = registry.create();
        registry.emplace<ee_ecs::enterprise>(miner, false, state);
        registry.emplace<ee_ecs::miner>(miner);
        registry.emplace<ee_ecs::container>(miner);
    }

    void TearDown()
    {
    }
};