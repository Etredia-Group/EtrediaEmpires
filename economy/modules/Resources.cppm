export module Economy:Resources;
import std;

export namespace ee_economy {

    struct Resource {
        std::uint32_t tag;
        std::uint8_t baseCost;
    };
    using ResourceList = std::vector<Resource>;

    struct ProductionRecipe {
        std::uint32_t tag;

        struct Entry {
            std::uint32_t tag;
            std::uint8_t cost;
        };
        static constexpr std::size_t MAX_ENTRY_COUNT = 4;
        Entry in[MAX_ENTRY_COUNT];
        Entry out[MAX_ENTRY_COUNT];
    };

}