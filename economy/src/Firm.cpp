module;

#include <entt/entt.hpp>

module Economy;


ee_economy::FirmDefinitionTable::FirmDefinitionTable(
    entt::registry &registry,
    const ee::ResourceLayout &sharedStorage
) : resourcesContext(registry.ctx().get<ResourceTable>()),
    recipesContext(registry.ctx().get<ProductionRecipeTable>()),
    sharedResourceLayout(sharedStorage) {}

void ee_economy::FirmDefinitionTable::OnAdd(const FirmDefinition &definition) {
    ee::ResourceLayout layout = sharedResourceLayout;
    const ProductionRecipe &recipe = recipesContext.Get(definition.productionRecipe);

    auto add = [&](const ee::ResourceTag tag) {
        if (
            const ee::ResourceId id = resourcesContext.GetId(tag);
            std::ranges::find(layout, id) == layout.end()
        ) {
            layout.push_back(id);
        }
    };

    for (const ProductionRecipe::Entry &entry : recipe.in) add(entry.tag);
    for (const ProductionRecipe::Entry &entry : recipe.out) add(entry.tag);

    resourceLayouts.push_back(std::move(layout));
}

std::span<const ee::ResourceId> ee_economy::FirmDefinitionTable::getResourceLayout(
    const ee::FirmDefId id
) const noexcept {
    return resourceLayouts[id];
}

std::optional<ee::ResourceTypesRange> ee_economy::FirmDefinitionTable::getResourceIndex(
    const ee::FirmDefId id,
    const ee::ResourceId resource
) const noexcept {
    const ee::ResourceLayout &layout = resourceLayouts[id];
    const auto entry = std::ranges::find(layout, resource);
    if (entry == layout.end()) return std::nullopt;

    return static_cast<ee::ResourceTypesRange>(entry - layout.begin());
}

void ee_economy::MergeFirmBatches(FirmBatchComponent &target, const FirmBatchComponent &source) noexcept {
    const std::uint32_t targetCount = target.count + source.count;
    if (targetCount == 0) {
        return;
    }

    target.damage = (target.damage * target.count + source.damage * source.count) / targetCount;
    target.efficiency = (target.efficiency * target.count + source.efficiency * source.count) / targetCount;
    target.count = targetCount;
}

ee::ResourceAmount ee_economy::FirmsScaleIncome(
    const FirmBatchComponent &batch,
    const ee::ResourceAmount value
) noexcept {
    return value; //TODO
}

ee::ResourceAmount ee_economy::FirmsScaleOutcome(
    const FirmBatchComponent &batch,
    const ee::ResourceAmount value
) noexcept {
    return value; //TODO
}
