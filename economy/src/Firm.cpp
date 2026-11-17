module Economy;
import std;

ee_economy::FirmDefinitionTable::FirmDefinitionTable(
    entt::registry &registry,
    const ee::ResourceLayout &sharedStorage
) : resourcesContext(registry.ctx().get<ResourceTable>()),
    recipesContext(registry.ctx().get<ProductionRecipeTable>()),
    sharedResourceLayout(sharedStorage) {}

void ee_economy::FirmDefinitionTable::OnLink(const FirmDefinition &definition) {
    ee::ResourceLayout layout = sharedResourceLayout;
    const ProductionRecipe &recipe = recipesContext.Get(std::to_underlying(definition.productionRecipe));

    auto add = [&](const ee::Id id) {
        if (std::ranges::find(layout, id) == layout.end()) {
            layout.push_back(id);
        }
    };

    for (const ProductionRecipe::Entry &entry : recipe.in) add(std::to_underlying(entry.resource));
    for (const ProductionRecipe::Entry &entry : recipe.out) add(std::to_underlying(entry.resource));

    resourceLayouts.push_back(std::move(layout));
}

void ee_economy::FirmDefinitionTable::OnLink() {
    productions.assign(definitions.size(), {});

    for (std::size_t i = 0; i < definitions.size(); i++) {
        const ProductionRecipe &recipe = recipesContext.Get(std::to_underlying(definitions[i].productionRecipe));

        for (const auto &[resource, count] : recipe.in) {
            const ee::ResourceTypesRange resourceIndex = getResourceIndex(i, std::to_underlying(resource)).value();
            productions[i].in.push_back({
                .resource = static_cast<ee::ResourceIdentity>(resourceIndex),
                .count = count
            });
        }

        for (const auto &[resource, count] : recipe.out) {
            const ee::ResourceTypesRange resourceIndex = getResourceIndex(i, std::to_underlying(resource)).value();
            productions[i].out.push_back({
                .resource = static_cast<ee::ResourceIdentity>(resourceIndex),
                .count = count
            });
        }
    }
}

std::span<const ee::Id> ee_economy::FirmDefinitionTable::getResourceLayout(const ee::Id firmId) const noexcept {
    return resourceLayouts[firmId];
}

const ee_economy::ProductionRecipe& ee_economy::FirmDefinitionTable::getProduction(ee::Id firmId) const noexcept {
    return productions[firmId];
}

std::optional<ee::ResourceTypesRange> ee_economy::FirmDefinitionTable::getResourceIndex(
    const ee::Id firmId,
    const ee::Id resource
) const noexcept {
    const ee::ResourceLayout &layout = resourceLayouts[firmId];
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

float ee_economy::CalcFirmsIncomeFactor(const FirmBatchComponent &batch) noexcept {
    return static_cast<float>(batch.count) * (1.0 - batch.damage);
}

float ee_economy::CalcFirmsOutcomeFactor(const FirmBatchComponent &batch) noexcept {
    return static_cast<float>(batch.count) * batch.efficiency * (1.0 - batch.damage);
}
