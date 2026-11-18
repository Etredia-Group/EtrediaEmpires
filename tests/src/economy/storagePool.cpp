module;

#include <gtest/gtest.h>

module tests;
import aliases;
import entt;
import Economy;
import std;

TEST(EconomyStoragePool, AllocateFree) {
    constexpr ee::ResourceTypesRange rowSize = std::numeric_limits<ee::ResourceTypesRange>::max();
    ee_economy::StoragePool pool(rowSize);
    ASSERT_EQ(pool.getRowSize(), rowSize);

    pool.Allocate(); // slots [0]
    ee::StorageSlot slot = pool.Allocate(); // slots [0 1]
    ASSERT_EQ(slot, 1);
    const std::span<const ee::ResourceAmount> resources = pool.getResources(slot);
    ASSERT_EQ(resources.size(), rowSize);
    for (const ee::ResourceAmount resource : resources) {
        ASSERT_EQ(resource, 0);
    }
    ASSERT_EQ(pool.getUsage(slot), 0);

    pool.Free(0); // slots [1]
    ASSERT_NO_FATAL_FAILURE(pool.getResources(slot));
    slot = pool.Allocate(); // slots [0 1]
    ASSERT_EQ(slot, 0);
}

TEST(EconomyStoragePool, SetResources) {
    ee_economy::StoragePool pool(2);
    const ee::StorageSlot slot1 = pool.Allocate();
    const ee::StorageSlot slot2 = pool.Allocate();
    const std::span<const ee::ResourceAmount> resources1 = pool.getResources(slot1);
    const std::span<const ee::ResourceAmount> resources2 = pool.getResources(slot2);

    pool.SetResource(slot1, static_cast<ee::ResourceIdentity>(1), 10, 100);
    ASSERT_EQ(resources1[0], 0);
    ASSERT_EQ(resources1[1], 100);
    ASSERT_EQ(resources2[0], 0);
    ASSERT_EQ(resources2[1], 0);
    ASSERT_EQ(pool.getUsage(slot1), 100 * 10);
    ASSERT_EQ(pool.getUsage(slot2), 0);

    pool.SetResource(slot2, static_cast<ee::ResourceIdentity>(0), 255, 500);
    ASSERT_EQ(resources1[0], 0);
    ASSERT_EQ(resources1[1], 100);
    ASSERT_EQ(resources2[0], 500);
    ASSERT_EQ(resources2[1], 0);
    ASSERT_EQ(pool.getUsage(slot1), 100 * 10);
    ASSERT_EQ(pool.getUsage(slot2), 500 * 255);
}

TEST(EconomyStoragePool, CreatePools) {
    entt::registry registry;
    registry.ctx().emplace<ee_economy::ResourceTable>();
    ee_economy::ProductionRecipeTable &recipes = registry.ctx().emplace<ee_economy::ProductionRecipeTable>();
    ee_economy::FirmDefinitionTable &firms =
        registry.ctx().emplace<ee_economy::FirmDefinitionTable>(registry, ee::ResourceLayout{0, 1});

    recipes.Add({
        {
            .tag = static_cast<ee_economy::ProductionRecipe::Tag>(100),
            .in = {
                {static_cast<ee::ResourceIdentity>(10)},
                {static_cast<ee::ResourceIdentity>(11)}
            }, // 2 elements
            .out = {
                {static_cast<ee::ResourceIdentity>(12)}
            } // 1 element
        },
        {
            .tag = static_cast<ee_economy::ProductionRecipe::Tag>(200),
            .in = {
                {static_cast<ee::ResourceIdentity>(0)}
            }, // 1 element but same as shared
            .out = {} // 0 elements
        }
    });

    firms.Add({
        {
            .tag = static_cast<ee_economy::FirmDefinition::Tag>(0),
            .productionRecipe = static_cast<ee_economy::ProductionRecipe::Tag>(100)
        },
        {
            .tag = static_cast<ee_economy::FirmDefinition::Tag>(1),
            .productionRecipe = static_cast<ee_economy::ProductionRecipe::Tag>(200)
        }
    });

    ee_ecs::Linker linker(registry);
    linker.Link(firms);

    ee_economy::StoragePools pools(firms);
    const ee_economy::StoragePool &pool1 = pools[0];
    const ee_economy::StoragePool &pool2 = pools[1];

    ASSERT_EQ(pool1.getRowSize(), 2 + 2 + 1);
    ASSERT_EQ(pool2.getRowSize(), 2 + 0 + 0);
}