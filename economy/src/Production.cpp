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
    float incomeFactor = CalcFirmsIncomeFactor(batch);
    if (incomeFactor <= 0.0f) return;
    float outcomeFactor = CalcFirmsOutcomeFactor(batch);

    StoragePool &storages = context.storagePools[firm.id];
    const std::span<const ee::ResourceAmount> resources = storages.getResources(batch.storageSlot);
    const FirmDefinition &firmDef = context.firms.Get(firm.id);
    const ProductionRecipe &production = context.firms.getProduction(firm.id);
    const ProductionRecipe &recipe = context.recipes.Get(std::to_underlying(firmDef.productionRecipe));

    const std::size_t inputCount = production.in.size();
    const std::size_t outputCount = production.out.size();

    // Limit 1: input supply. Early exit before touching any space math.
    float minSupplyRatio = std::numeric_limits<float>::max();
    for (const auto &[resource, count] : production.in) {
        minSupplyRatio = std::min(minSupplyRatio, static_cast<float>(resources[std::to_underlying(resource)]) / count);
    }
    float productionScale = std::min(1.0f, minSupplyRatio / incomeFactor);
    if (productionScale <= 0.0f) return;

    // Hoisted so the compiler doesn't reload them after every opaque SetResource call.
    const ee::StorageUsage totalCapacity = firmDef.storageCapacity * batch.count;
    ee::StorageUsage usage = storages.getUsage(batch.storageSlot); // the only getUsage call

    // Limit 2: storage space.
    float consumedSpace = 0.0f;
    for (std::size_t inputIndex = 0; inputIndex < inputCount; inputIndex++) {
        const Resource &resourceInfo = context.resouces.Get(std::to_underlying(recipe.in[inputIndex].resource));
        consumedSpace += static_cast<float>(production.in[inputIndex].count) * static_cast<float>(resourceInfo.size);
    }
    consumedSpace *= incomeFactor;

    float producedSpace = 0.0f;
    for (std::size_t outputIndex = 0; outputIndex < outputCount; outputIndex++) {
        const Resource &resourceInfo = context.resouces.Get(std::to_underlying(recipe.out[outputIndex].resource));
        producedSpace += static_cast<float>(production.out[outputIndex].count) * static_cast<float>(resourceInfo.size);
    }
    producedSpace *= outcomeFactor;

    if (
        const float netSpaceGrowth = producedSpace - consumedSpace;
        netSpaceGrowth > 0.0f
    ) {
        const float freeSpaceBefore = usage < totalCapacity ? static_cast<float>(totalCapacity - usage) : 0.0f;
        productionScale = std::min(productionScale, freeSpaceBefore / netSpaceGrowth);
        if (productionScale <= 0.0f) return;
    }

    incomeFactor *= productionScale;
    outcomeFactor *= productionScale;

    for (std::size_t inputIndex = 0; inputIndex < inputCount; inputIndex++) {
        const auto &[resource, count] = production.in[inputIndex];
        const Resource &resourceInfo = context.resouces.Get(std::to_underlying(recipe.in[inputIndex].resource));

        const ee::ResourceAmount current = resources[std::to_underlying(resource)];
        const ee::ResourceAmount consumed = std::min(
            current,
            static_cast<ee::ResourceAmount>(count * incomeFactor + 0.5f)
        );

        storages.SetResource(
            batch.storageSlot,
            production.in[inputIndex].resource,
            resourceInfo.size,
            current - consumed
        );
        usage -= static_cast<ee::StorageUsage>(consumed) * static_cast<ee::StorageUsage>(resourceInfo.size);
    }

    for (std::size_t outputIndex = 0; outputIndex < outputCount; outputIndex++) {
        const auto &[resource, count] = production.out[outputIndex];
        const Resource &resourceInfo = context.resouces.Get(std::to_underlying(recipe.out[outputIndex].resource));

        ee::ResourceAmount toProduce = static_cast<ee::ResourceAmount>(count * outcomeFactor + 0.5f);
        if (resourceInfo.size > 0) {
            // Rounding safety net, using the tracked usage instead of calling getUsage again.
            const ee::ResourceAmount maxFit = static_cast<ee::ResourceAmount>(
                (totalCapacity - usage) / resourceInfo.size
            );
            toProduce = std::min(toProduce, maxFit);
        }
        if (toProduce == 0) return;

        storages.SetResource(
            batch.storageSlot,
            production.out[outputIndex].resource,
            resourceInfo.size,
            resources[std::to_underlying(resource)] + toProduce
        );
        usage += static_cast<ee::StorageUsage>(toProduce) * static_cast<ee::StorageUsage>(resourceInfo.size);
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