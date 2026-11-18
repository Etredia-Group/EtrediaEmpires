module;

#include <gtest/gtest.h>

module tests;
import entt;
import aliases;
import EcsContext;
import Economy;

struct EconomyFirm : testing::Test {

    ee_economy::Resource resource1_1 { .tag = static_cast<ee_economy::Resource::Tag>(101), .size = 1 };
    ee_economy::Resource resource1_2 { .tag = static_cast<ee_economy::Resource::Tag>(102), .size = 1 };
    ee_economy::Resource resource2_1 { .tag = static_cast<ee_economy::Resource::Tag>(201), .size = 2 };
    ee_economy::Resource resource3_1 { .tag = static_cast<ee_economy::Resource::Tag>(301), .size = 1 };
    ee_economy::Resource resource3_2 { .tag = static_cast<ee_economy::Resource::Tag>(302), .size = 1 };

    ee::ResourceLayout sharedResources;

    ee_economy::ProductionRecipe recipe_resource2_1 {
        .tag = static_cast<ee_economy::ProductionRecipe::Tag>(100),
        .in = {
            {.resource = static_cast<ee_economy::Resource::Tag>(101), .count = 2},
            {.resource = static_cast<ee_economy::Resource::Tag>(102), .count = 1}
        },
        .out = {
            {.resource = static_cast<ee_economy::Resource::Tag>(201), .count = 3}
        }
    };

    ee_economy::FirmDefinition firm_recipe_100 {
        .tag = static_cast<ee_economy::FirmDefinition::Tag>(0),
        .productionRecipe = static_cast<ee_economy::ProductionRecipe::Tag>(100),
        .storageCapacity = 10
    };

    entt::registry registry;
    ee_economy::ResourceTable *resourceTable = nullptr;
    ee_economy::ProductionRecipeTable *productionRecipeTable = nullptr;

    void SetUp() override {
        resourceTable = &registry.ctx().emplace<ee_economy::ResourceTable>();
        productionRecipeTable = &registry.ctx().emplace<ee_economy::ProductionRecipeTable>();

        std::vector resourcesList {
            {static_cast<ee_economy::Resource::Tag>(0)}, // padding for layout test
            {static_cast<ee_economy::Resource::Tag>(1)}, // padding for layout test
            {static_cast<ee_economy::Resource::Tag>(2)}, // padding for layout test
            resource1_1, resource1_2, resource2_1, resource3_1, resource3_2
        };
        resourceTable->Add(std::move(resourcesList));
        productionRecipeTable->Add({recipe_resource2_1});

        ee_ecs::Linker linker(registry);
        linker.Link(*productionRecipeTable);

        // Ids of resources 3.1 and 3.2
        sharedResources.push_back(resourcesList.size() - 2);
        sharedResources.push_back(resourcesList.size() - 1);
    }

    void TearDown() override {

    }
};

TEST_F(EconomyFirm, CreateTable) {
    ee_economy::FirmDefinitionTable &table =
        registry.ctx().emplace<ee_economy::FirmDefinitionTable>(registry, sharedResources);
    table.Add({firm_recipe_100});

    ee_ecs::Linker linker(registry);
    linker.Link(table);

    // firm_recipe_0 must have layout:
    // [ 3.1 3.2 1.1 1.2 2.1] tags
    // [  6   7   3   4   5 ] ids
    // [  0   1   2   3   4 ] layout index

    const std::span<const ee::Id> layout = table.getResourceLayout(0);
    ASSERT_EQ(layout.size(), 5);
    ASSERT_EQ(layout[0], 6);
    ASSERT_EQ(layout[1], 7);
    ASSERT_EQ(layout[2], 3);
    ASSERT_EQ(layout[3], 4);
    ASSERT_EQ(layout[4], 5);

    const ee_economy::ProductionRecipe production = table.getProduction(0);
    ASSERT_EQ(production.tag, static_cast<ee::RecipeIdentity>(productionRecipeTable->FindId(100)));
    ASSERT_EQ(production.in[0].resource, static_cast<ee::ResourceIdentity>(2));
    ASSERT_EQ(production.in[1].resource, static_cast<ee::ResourceIdentity>(3));
    ASSERT_EQ(production.out[0].resource, static_cast<ee::ResourceIdentity>(4));
}

TEST_F(EconomyFirm, Merge) {
    ee_economy::FirmBatchComponent batch {
        .count = 100,
        .damage = 0.5f,
        .efficiency = 1.5f
    };
    constexpr ee_economy::FirmBatchComponent add {
        .count = 2,
        .damage = 1.0f,
        .efficiency = 0.5f
    };

    ee_economy::MergeFirmBatches(batch, add);
    ASSERT_EQ(batch.count, 102);
    EXPECT_NEAR(batch.damage, 0.5098f, 1e-4f);
    EXPECT_NEAR(batch.efficiency, 1.4804f, 1e-4f);
}

TEST_F(EconomyFirm, Produce) {
    ee_economy::FirmDefinitionTable &table =
        registry.ctx().emplace<ee_economy::FirmDefinitionTable>(registry, sharedResources);
    table.Add({firm_recipe_100});

    ee_ecs::Linker linker(registry);
    linker.Link(table);
    ee_economy::StoragePools pools(table);
    ee_economy::StoragePool &storages = pools[0];
    const ee::StorageSlot storageSlot = storages.Allocate();
    const ee_economy::ProductionRecipe production = table.getProduction(0);
    const std::span<const ee::ResourceAmount> resources = storages.getResources(storageSlot);

    ee_economy::FirmDefComponent def {.id = 0};
    ee_economy::FirmBatchComponent batch {
        .count = 15'000,
        .storageSlot = storageSlot
    };
    const entt::entity e = registry.create();
    registry.emplace<ee_economy::FirmDefComponent>(e, def);
    registry.emplace<ee_economy::FirmBatchComponent>(e, batch);

    // TEST: Limit by storage capacity
    storages.SetResource(storageSlot, production.in[0].resource, 1, 30'000); // Production: -15'000
    storages.SetResource(storageSlot, production.in[1].resource, 1, 12'000); // Production: -7'500
    storages.SetResource(storageSlot, production.out[0].resource, 2, 42'750); // Production: +22'500

    ee_economy::systems::FirmsProduce(registry);
    ASSERT_EQ(resources[0], 0);
    ASSERT_EQ(resources[1], 0);
    ASSERT_EQ(resources[2], 15'000);
    ASSERT_EQ(resources[3], 4'500);
    ASSERT_EQ(resources[4], 65'250);
    ASSERT_EQ(storages.getUsage(storageSlot), firm_recipe_100.storageCapacity * batch.count);

    // TEST: Limit by supply lack
    storages.SetResource(storageSlot, production.in[0].resource, 1, 30'000); // Production: -12'000
    storages.SetResource(storageSlot, production.in[1].resource, 1, 6'000); // Production: -6'000
    storages.SetResource(storageSlot, production.out[0].resource, 2, 42'750); // Production: +18'000

    ee_economy::systems::FirmsProduce(registry);
    ASSERT_EQ(resources[0], 0);
    ASSERT_EQ(resources[1], 0);
    ASSERT_EQ(resources[2], 18'000);
    ASSERT_EQ(resources[3], 0);
    ASSERT_EQ(resources[4], 60'000);
    ASSERT_EQ(storages.getUsage(storageSlot), 138'000);
}