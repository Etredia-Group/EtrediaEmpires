export module Economy:Production;
import :Resources;
import :Firm;
import :StoragePool;
import entt;
import aliases;
import EcsContext;
import std;

namespace ee_economy {

    struct FirmProduceContext {
        const FirmDefinitionTable &firms;
        const ResourceTable &resouces;
        const ProductionRecipeTable &recipes;
        StoragePools &storagePools;
    };

    void FirmProduceStep(
        const FirmProduceContext &context,
        const FirmDefComponent &firm,
        const FirmBatchComponent &batch
    ) noexcept;

}

namespace ee_ecs::systems {
    template<>
    ee_economy::FirmProduceContext FetchContext<ee_economy::FirmProduceContext>(entt::registry &registry);
}

namespace ee_economy::systems {
    struct FirmProducePolicy final : ee_ecs::systems::SystemPolicy<4'000, ee_utils::execution::par_unseq>{};
}

export namespace ee_economy::systems {

    void FirmsProduce(entt::registry &registry);

}