export module aliases;
import std;

export namespace ee {
    using Id = std::uint32_t;
    using RegionId = Id;

    using Tag = Id; // Tag must be same as Id for Identity linkage

    using RecipeLayoutIndex = Id; // Must be same as Id for Layouts cache
    using ProductionLayoutIndex = RecipeLayoutIndex;
    using ResourceLayout = std::vector<Id>; // Resources storage definition
    using ResourceTypesRange = std::uint16_t; // Range must be <= Id range
    using ResourceAmount = std::uint16_t;
    using ResourceSize = std::uint8_t;
    using ResourceMass = std::uint8_t;
    using StorageSlot = std::uint32_t; // Range of slots depends on amount of batches per firm type
    using StorageUsage = std::uint32_t;
}

export namespace ee {

    union ResourceIdentity {Tag tag; Id id; RecipeLayoutIndex layoutIndex;};
    union RecipeIdentity {Tag tag; Id id;};
    union FirmIdentity {Tag tag; Id id;};

    template<typename T> concept IIdentity = std::is_union_v<std::remove_cvref_t<T>> &&
        requires(std::remove_cvref_t<T> v) {
            { v.tag } -> std::same_as<Tag&>;
            { v.id }  -> std::same_as<Id&>;
        };
}