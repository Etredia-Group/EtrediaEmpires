module;

#include <benchmark/benchmark.h>

module benchmarks;
import aliases;
import entt;
import EcsContext;
import Economy;
import std;

namespace {
    entt::registry BuildRegistry(const std::size_t firmBatchesCount) {
        entt::registry registry;

        constexpr ee_economy::Resource resource1_1 { .tag = {.tag = 101}, .size = 0 };
        constexpr ee_economy::Resource resource1_2 { .tag = {.tag = 102}, .size = 0 };
        constexpr ee_economy::Resource resource2_1 { .tag = {.tag = 201}, .size = 0 };
        constexpr ee_economy::Resource resource3_1 { .tag = {.tag = 301}, .size = 0 };
        constexpr ee_economy::Resource resource3_2 { .tag = {.tag = 302}, .size = 0 };

        ee::ResourceLayout sharedResources;

        ee_economy::ProductionRecipe recipe_resource2_1 {
            .tag = {.tag = 100},
            .in = {
                {.resource = {.tag = 101}, .count = 2},
                {.resource = {.tag = 102}, .count = 1}
            },
            .out = {
                {.resource = {.tag = 201}, .count = 3}
            }
        };

        ee_economy::ResourceTable &resourceTable =
            registry.ctx().emplace<ee_economy::ResourceTable>();
        ee_economy::ProductionRecipeTable &productionRecipeTable =
            registry.ctx().emplace<ee_economy::ProductionRecipeTable>();

        std::vector resourcesList {
            {.tag = {.tag = 0}}, // padding for layout test
            {.tag = {.tag = 1}}, // padding for layout test
            {.tag = {.tag = 2}}, // padding for layout test
            resource1_1, resource1_2, resource2_1, resource3_1, resource3_2
        };
        resourceTable.Add(std::move(resourcesList));
        productionRecipeTable.Add({recipe_resource2_1});

        ee_ecs::Linker linker(registry);
        linker.Link(productionRecipeTable);

        // Ids of resources 3.1 and 3.2
        sharedResources.push_back(resourceTable.getSize() - 2);
        sharedResources.push_back(resourceTable.getSize() - 1);

        ee_economy::FirmDefinitionTable &firmDefsTable =
            registry.ctx().emplace<ee_economy::FirmDefinitionTable>(registry, sharedResources);
        std::vector<ee_economy::FirmDefinition> firmDefsList;
        firmDefsList.resize(255);
        for (std::size_t i = 0; i < firmDefsList.size(); i++) {
            firmDefsList[i] = {
                .tag = {.tag = static_cast<std::uint32_t>(i)},
                .productionRecipe = {.tag = 100},
                .storageCapacity = 1
            };
        }
        firmDefsTable.Add(std::move(firmDefsList));
        linker.Link(firmDefsTable);
        ee_economy::StoragePools &storagePools = registry.ctx().emplace<ee_economy::StoragePools>(firmDefsTable);

        // Create entities
        for (std::size_t i = 0; i < firmBatchesCount; i++) {
            const std::size_t firmDefId = i % firmDefsTable.getSize();
            const entt::entity entity = registry.create();
            const ee::StorageSlot storageSlot = storagePools[firmDefId].Allocate();
            const ee_economy::ProductionRecipe &production = firmDefsTable.getProduction(firmDefId);

            registry.emplace<ee_economy::FirmDefComponent>(entity, ee_economy::FirmDefComponent{
                .id = static_cast<std::uint8_t>(firmDefId)
            });
            registry.emplace<ee_economy::FirmBatchComponent>(entity, ee_economy::FirmBatchComponent{
                .count = 15'000,
                .storageSlot = storageSlot
            });

            storagePools[firmDefId].SetResource(
                storageSlot,
                production.in[0].resource.layoutIndex,
                0,
                std::numeric_limits<ee::ResourceAmount>::max()
            );
            storagePools[firmDefId].SetResource(
                storageSlot,
                production.in[1].resource.layoutIndex,
                0,
                std::numeric_limits<ee::ResourceAmount>::max()
            );
        }

        // Warm up owning group
        ee_ecs::systems::FetchEntities<ee_economy::FirmDefComponent>(registry);

        return registry;
    }
}

namespace {

    void BenchmarkFirmsProduce(benchmark::State& state) {
        const std::size_t firmBatchesCount = state.range(0);
        entt::registry registry = BuildRegistry(firmBatchesCount);

        for ([[maybe_unused]] benchmark::State::StateIterator::Value iteration : state) {
            ee_economy::systems::FirmsProduce(registry);
            benchmark::ClobberMemory();
        }

        state.SetItemsProcessed(state.iterations() * firmBatchesCount);
    }

}

BENCHMARK(BenchmarkFirmsProduce)
    ->RangeMultiplier(4)
    ->Range(64, 1 << 20)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime();
