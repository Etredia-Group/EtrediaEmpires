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

        ee::ResourceTypesRange getRowSize() const noexcept {return stride;}
        std::span<ee::ResourceAmount> getRow(ee::StorageSlot slot) noexcept;
        std::span<const ee::ResourceAmount> getRow(ee::StorageSlot slot) const noexcept;

        ee::StorageSlot Allocate();
        void Free(ee::StorageSlot slot);

    private:
        static constexpr ee::StorageSlot PAGE_SHIFT = 10;
        static constexpr ee::StorageSlot SLOTS_PER_PAGE = 1u << PAGE_SHIFT;
        static constexpr ee::StorageSlot PAGE_MASK = SLOTS_PER_PAGE - 1;

        ee::ResourceTypesRange stride;
        ee::StorageSlot used = 0;
        std::vector<std::unique_ptr<ee::ResourceAmount[]>> pages;
        std::vector<ee::StorageSlot> freeSlots;

        ee::ResourceAmount* getRowPtr(ee::StorageSlot slot) const noexcept;
    };

    class StoragePools {
    public:
        explicit StoragePools(const FirmDefinitionTable &definitions);
        StoragePools(const StoragePools &) = delete;
        StoragePools &operator=(const StoragePools &) = delete;
        StoragePools(StoragePools &&) noexcept = default;
        StoragePools &operator=(StoragePools &&) noexcept = default;

        StoragePool &operator[](ee::FirmDefId id) noexcept;

    private:
        std::vector<StoragePool> poolsByDefId;
    };

}