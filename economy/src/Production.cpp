module Economy;

template<>
ee_economy::FirmProduceContext ee_ecs::systems::FetchContext<ee_economy::FirmProduceContext>(entt::registry &registry) {
    auto &ctx = registry.ctx();
    return {
        .firms = ctx.get<ee_economy::FirmDefinitionTable>(),
        .resouces = ctx.get<ee_economy::ResourceTable>(),
        .recipes = ctx.get<ee_economy::ProductionRecipeTable>(),
        .storagePools = ctx.get<ee_economy::StoragePools>()
    };
}

// Thread-safe
void ee_economy::FirmProduceStep(
    const FirmProduceContext &context,
    const FirmDefComponent &firm,
    const FirmBatchComponent &batch
) noexcept {
    StoragePool &storages = context.storagePools[firm.id];
    const std::span<const ee::ResourceAmount> resources = storages.getResources(batch.storageSlot);
    const FirmDefinition &firmDef = context.firms.Get(firm.id);
    const ProductionRecipe &production = context.firms.getProduction(firm.id);
    const ProductionRecipe &recipe = context.recipes.Get(std::to_underlying(firmDef.productionRecipe));

    float incomeFactor = CalcFirmsIncomeFactor(batch);
    const float outcomeFactor = CalcFirmsOutcomeFactor(batch);
    if (incomeFactor <= 0.0f) return;

    float minSupplyRatio = std::numeric_limits<float>::max();
    for (const auto &[resource, count] : production.in) {
        minSupplyRatio = std::min(minSupplyRatio, static_cast<float>(resources[std::to_underlying(resource)]) / count);
    }
    const float used = std::min(1.0f, minSupplyRatio / incomeFactor);
    if (used <= 0.0f) return;

    incomeFactor *= used;
    for (std::size_t i = 0; i < production.in.size(); i++) {
        const auto &[resource, count] = production.in[i];
        const Resource &resourceInfo = context.resouces.Get(std::to_underlying(recipe.in[i].resource));

        storages.SetResource(
            batch.storageSlot,
            recipe.in[i].resource,
            resourceInfo.size,

            resources[std::to_underlying(resource)] - std::min(
                resources[std::to_underlying(resource)],
                static_cast<ee::ResourceAmount>(count * incomeFactor + 0.5f)
            )
        );
    }

    for (std::size_t i = 0; i < production.out.size(); i++) {
        const auto &[resource, count] = production.out[i];
        const Resource &resourceInfo = context.resouces.Get(std::to_underlying(recipe.out[i].resource));

        const ee::StorageUsage freeSpace =
            firmDef.storageCapacity * batch.count - storages.getUsage(batch.storageSlot);

        ee::ResourceAmount toProduce = static_cast<ee::ResourceAmount>(count * outcomeFactor + 0.5f);
        if (resourceInfo.size > 0) {
            const ee::ResourceAmount maxFit = static_cast<ee::ResourceAmount>(freeSpace / resourceInfo.size);
            toProduce = std::min(toProduce, maxFit);
        }

        storages.SetResource(
            batch.storageSlot,
            recipe.out[i].resource,
            resourceInfo.size,
            resources[std::to_underlying(resource)] + toProduce
        );
    }
}

void ee_economy::systems::FirmsProduce(entt::registry &registry) {
    const FirmProduceContext ctx = ee_ecs::systems::FetchContext<FirmProduceContext>(registry);

    if (
        const auto group = registry.group<const FirmDefComponent, const FirmBatchComponent>();
        group.size() < FirmProducePolicy::parallelThreshold
    ) {
        group.each([&](const FirmDefComponent &firm, const FirmBatchComponent &batch) {
            FirmProduceStep(ctx, firm, batch);
        });
    }
    else {
        std::for_each(FirmProducePolicy::par, group.begin(), group.end(), [&](const entt::entity entity) {
            const auto [firm, batch] = group.get(entity);
            FirmProduceStep(ctx, firm, batch);
        });
    }
}