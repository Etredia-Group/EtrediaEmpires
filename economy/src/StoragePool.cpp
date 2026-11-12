module Economy;
import utils;

ee_economy::StoragePool::StoragePool(const ee::ResourceTypesRange resourceTypesAmount) :
    stride(resourceTypesAmount) {}

std::span<ee::ResourceAmount> ee_economy::StoragePool::getRow(const ee::StorageSlot slot) noexcept {
    return { getRowPtr(slot), stride };
}

std::span<const ee::ResourceAmount> ee_economy::StoragePool::getRow(const ee::StorageSlot slot) const noexcept {
    return { getRowPtr(slot), stride };
}

ee::StorageSlot ee_economy::StoragePool::Allocate() {
    if (!freeSlots.empty()) {
        const ee::StorageSlot slot = freeSlots.back();
        freeSlots.pop_back();

        const std::span<ee::ResourceAmount> row = getRow(slot);
        std::ranges::fill(row, 0u);

        return slot;
    }

    if (used == STORAGE_SLOT_INVALID) {
        LOG(CRITICAL, "Resource storages overflow! %u slots.", STORAGE_SLOT_INVALID);
        throw std::bad_alloc{};
    }

    const ee::StorageSlot slot = used++;
    if (static_cast<std::size_t>(slot >> PAGE_SHIFT) >= pages.size()) {
        pages.push_back(std::make_unique<ee::ResourceAmount[]>(
            static_cast<std::size_t>(SLOTS_PER_PAGE) * stride
        ));
    }
    return slot;
}

void ee_economy::StoragePool::Free(const ee::StorageSlot slot) {
    freeSlots.push_back(slot);
}

ee::ResourceAmount * ee_economy::StoragePool::getRowPtr(const ee::StorageSlot slot) const noexcept {
    return pages[slot >> PAGE_SHIFT].get() + static_cast<std::size_t>(slot & PAGE_MASK) * stride;
}

ee_economy::StoragePools::StoragePools(const FirmDefinitionTable &definitions) {
    const std::size_t poolsSize = definitions.getSize();
    poolsByDefId.reserve(poolsSize);

    for (std::size_t i = 0; i < poolsSize; i++) {
        const ee::ResourceTypesRange poolRange = static_cast<ee::ResourceTypesRange>(
            definitions.getResourceLayout(static_cast<ee::FirmDefId>(i)).size()
        );
        poolsByDefId.emplace_back(poolRange);
    }
}

ee_economy::StoragePool & ee_economy::StoragePools::operator[](const ee::FirmDefId id) noexcept {
    return poolsByDefId[id];
}
