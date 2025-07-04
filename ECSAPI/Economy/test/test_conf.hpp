#pragma once

#include <ee_economy/economy.hpp>
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
        registry.emplace<ee_economy::state>(state);
        registry.emplace<ee_economy::container>(state);

        // Create miner enterprise
        miner = registry.create();
        registry.emplace<ee_economy::enterprise>(miner, false, state);
        registry.emplace<ee_economy::miner>(miner);
        registry.emplace<ee_economy::container>(miner);
    }

    void TearDown()
    {
    }
};