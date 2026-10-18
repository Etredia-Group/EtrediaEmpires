module;

#include <entt/entt.hpp>

module Economy;
import :Firm;

ee_economy::FirmDefinitionTable::FirmDefinitionTable(FirmList &definitions) :
    definitions(definitions) {}

std::optional<ee_economy::FirmDefId> ee_economy::FirmDefinitionTable::Find(
    const ee::FirmTag tag
) const noexcept {
    const auto entry = defByTag.find(tag);
    if (entry == defByTag.end()) {
        return std::nullopt;
    }
    return entry->second;
}

const ee_economy::FirmDefinition & ee_economy::FirmDefinitionTable::operator[](
    const FirmDefId id
) const noexcept {
    return definitions[id];
}

void ee_economy::MergeFirmBatches(FirmBatchComponent &target, const FirmBatchComponent &source) noexcept {
    const std::uint32_t targetCount = target.count + source.count;
    if (targetCount == 0) {
        target = {};
        return;
    }

    target.damage = (target.damage * target.count + source.damage * source.count) / targetCount;
    target.efficiency = (target.efficiency * target.count + source.efficiency * source.count) / targetCount;
    target.count = targetCount;
}
