#pragma once

#include <ee_economy/economy.hpp>
#include <entt/entt.hpp>
#include <gtest/gtest.h>

struct SystemsTest : testing::Test
{
    entt::registry registry;

    entt::entity state;

    entt::entity miner;
    entt::entity storage;
    entt::entity deliverer;

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

        // Create storage enterprise
        storage = registry.create();
        registry.emplace<ee_economy::enterprise>(storage, false, state);
        registry.emplace<ee_economy::container>(storage);

        // Create delivering enterprise
        deliverer = registry.create();
        registry.emplace<ee_economy::enterprise>(deliverer, false, state);
        registry.emplace<ee_economy::deliverer>(deliverer);
        registry.emplace<ee_economy::container>(deliverer);
    }

    void TearDown()
    {
    }
};