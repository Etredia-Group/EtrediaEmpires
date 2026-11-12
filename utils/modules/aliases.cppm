export module aliases;
import std;

export namespace ee {
    using Tag = std::uint32_t;
    using ResourceTag = Tag;
    using FirmTag = Tag;
    using RecipeTag = Tag;

    using RegionId = std::uint32_t;
    using ResourceId = std::uint8_t;
    using RecipeId = std::uint16_t;
    using FirmDefId = std::uint8_t;

    using ResourceLayout = std::vector<ResourceId>; // Resources storage definition
    using ResourceTypesRange = std::uint16_t;
    using ResourceAmount = std::uint32_t;
    using StorageSlot = std::uint32_t; // Range of slots depends on amount of batches per firm type
}