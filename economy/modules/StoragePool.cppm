export module Economy:StoragePool;
import :Firm;
import aliases;
import std;

export namespace ee_economy {

    class StoragePool {
    public:
        static constexpr ee::StorageSlot STORAGE_SLOT_INVALID = std::numeric_limits<ee::StorageSlot>::max();

        explicit StoragePool(ee::ResourceTypesRange resourceTypesAmount);
        StoragePool(const StoragePool &) = delete;
        StoragePool &operator=(const StoragePool &) = delete;
        StoragePool(StoragePool &&) noexcept = default;
        StoragePool &operator=(StoragePool &&) noexcept = default;

        void SetResource(
            ee::StorageSlot slot,
            ee::ProductionLayoutIndex resource,
            ee::ResourceSize resourceSize,
            ee::ResourceAmount value
        ) noexcept;

        ee::ResourceTypesRange getRowSize() const noexcept {return stride;}
        std::span<const ee::ResourceAmount> getResources(ee::StorageSlot slot) const noexcept;
        ee::StorageUsage getUsage(ee::StorageSlot slot) const noexcept;

        ee::StorageSlot Allocate();
        void Free(ee::StorageSlot slot);

    private:
        static constexpr std::size_t HEADER_SIZE = sizeof(ee::StorageUsage);
        static constexpr ee::StorageSlot PAGE_SHIFT = 10;
        static constexpr ee::StorageSlot SLOTS_PER_PAGE = 1u << PAGE_SHIFT;
        static constexpr ee::StorageSlot PAGE_MASK = SLOTS_PER_PAGE - 1;

        ee::ResourceTypesRange stride;
        std::size_t rowSize;
        ee::StorageSlot used = 0;
        std::vector<std::unique_ptr<std::byte[]>> pages;
        std::vector<ee::StorageSlot> freeSlots;

        std::byte* getRowPtr(ee::StorageSlot slot) const noexcept;
        std::span<ee::ResourceAmount> getResourcesMutable(ee::StorageSlot slot) noexcept;
    };

    class StoragePools {
    public:
        explicit StoragePools(const FirmDefinitionTable &definitions);
        StoragePools(const StoragePools &) = delete;
        StoragePools &operator=(const StoragePools &) = delete;
        StoragePools(StoragePools &&) noexcept = default;
        StoragePools &operator=(StoragePools &&) noexcept = default;

        StoragePool &operator[](ee::Id firmId) noexcept;

    private:
        std::vector<StoragePool> poolsByDefId;
    };

}