module Economy;
import utils;

ee_economy::StoragePool::StoragePool(const ee::ResourceTypesRange resourceTypesAmount) :
    stride(resourceTypesAmount),
    rowSize(HEADER_SIZE + stride * sizeof(ee::ResourceAmount)) {}

std::span<ee::ResourceAmount> ee_economy::StoragePool::getResourcesMutable(const ee::StorageSlot slot) noexcept {
    return { reinterpret_cast<ee::ResourceAmount*>(getRowPtr(slot) + HEADER_SIZE), stride };
}

std::span<const ee::ResourceAmount> ee_economy::StoragePool::getResources(const ee::StorageSlot slot) const noexcept {
    return { reinterpret_cast<const ee::ResourceAmount*>(getRowPtr(slot) + HEADER_SIZE), stride };
}

ee::StorageUsage ee_economy::StoragePool::getUsage(const ee::StorageSlot slot) const noexcept {
    ee::StorageUsage used;
    std::memcpy(&used, getRowPtr(slot), sizeof(used));
    return used;
}

void ee_economy::StoragePool::SetResource(
    const ee::StorageSlot slot,
    const ee::ProductionLayoutIndex resource,
    const ee::ResourceSize resourceSize,
    const ee::ResourceAmount value
) noexcept {
    std::span<ee::ResourceAmount> resources = getResourcesMutable(slot);

    const ee::ResourceAmount oldValue = resources[static_cast<std::size_t>(resource)];
    resources[static_cast<std::size_t>(resource)] = value;

    ee::StorageUsage *usage = reinterpret_cast<ee::StorageUsage*>(getRowPtr(slot));
    const ee::StorageUsage newUsage = *usage -
        static_cast<ee::StorageUsage>(oldValue) * resourceSize +
        static_cast<ee::StorageUsage>(value) * resourceSize;
    std::memcpy(usage, &newUsage, sizeof(newUsage));
}

ee::StorageSlot ee_economy::StoragePool::Allocate() {
    if (!freeSlots.empty()) {
        const ee::StorageSlot slot = freeSlots.back();
        freeSlots.pop_back();

        std::memset(getRowPtr(slot), 0, rowSize);
        return slot;
    }

    if (used == STORAGE_SLOT_INVALID) {
        LOG(CRITICAL, "Resource storages overflow! %u slots.", STORAGE_SLOT_INVALID);
        throw std::bad_alloc{};
    }

    const ee::StorageSlot slot = used++;
    if (static_cast<std::size_t>(slot >> PAGE_SHIFT) >= pages.size()) {
        pages.push_back(std::make_unique<std::byte[]>(
            static_cast<std::size_t>(SLOTS_PER_PAGE) * rowSize
        ));
    }
    return slot;
}

void ee_economy::StoragePool::Free(const ee::StorageSlot slot) {
    freeSlots.push_back(slot);
}

std::byte* ee_economy::StoragePool::getRowPtr(const ee::StorageSlot slot) const noexcept {
    return pages[slot >> PAGE_SHIFT].get() + static_cast<std::size_t>(slot & PAGE_MASK) * rowSize;
}

ee_economy::StoragePools::StoragePools(const FirmDefinitionTable &definitions) {
    const std::size_t poolsSize = definitions.getSize();
    poolsByDefId.reserve(poolsSize);

    for (std::size_t i = 0; i < poolsSize; i++) {
        const ee::ResourceTypesRange poolRange = static_cast<ee::ResourceTypesRange>(
            definitions.getResourceLayout(static_cast<ee::Id>(i)).size()
        );
        poolsByDefId.emplace_back(poolRange);
    }
}

ee_economy::StoragePool & ee_economy::StoragePools::operator[](const ee::Id firmId) noexcept {
    return poolsByDefId[firmId];
}
