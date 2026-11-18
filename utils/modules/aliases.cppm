export module aliases;
import std;

export namespace ee {
    using Id = std::uint32_t;
    using RegionId = Id;

    using ResourceLayout = std::vector<Id>; // Resources storage definition
    using ResourceTypesRange = std::uint16_t; // Range must be <= Id range
    using ResourceAmount = std::uint16_t;
    using ResourceSize = std::uint8_t;
    using ResourceMass = std::uint8_t;
    using StorageSlot = std::uint32_t; // Range of slots depends on amount of batches per firm type
    using StorageUsage = std::uint32_t;
}

export namespace ee {
    using Tag = Id; // Tag must be same as Id for Identity linkage
    enum class ResourceIdentity : Id {};
    enum class FirmIdentity : Id {};
    enum class RecipeIdentity : Id {};

    union RIdentity{
        Tag tag;
        ResourceIdentity id;
    };

    //TODO: codegen
    template<typename T> concept ITag =
        std::same_as<std::remove_cvref_t<T>, ResourceIdentity> ||
        std::same_as<std::remove_cvref_t<T>, FirmIdentity> ||
        std::same_as<std::remove_cvref_t<T>, RecipeIdentity>;
}