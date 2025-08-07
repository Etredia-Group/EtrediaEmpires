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
        registry.emplace<ee_economy::State>(state);
        registry.emplace<ee_economy::Container>(state);

        // Create miner enterprise
        miner = registry.create();
        registry.emplace<ee_economy::Enterprise>(miner, false, state);
        registry.emplace<ee_economy::Miner>(miner);
        registry.emplace<ee_economy::Container>(miner);

        // Create storage enterprise
        storage = registry.create();
        registry.emplace<ee_economy::Enterprise>(storage, false, state);
        registry.emplace<ee_economy::Container>(storage);

        // Create delivering enterprise
        deliverer = registry.create();
        registry.emplace<ee_economy::Enterprise>(deliverer, false, state);
        registry.emplace<ee_economy::Deliverer>(deliverer);
        registry.emplace<ee_economy::Container>(deliverer);
    }

    void TearDown()
    {
    }
};