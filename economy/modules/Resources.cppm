export module Economy:Resources;
import std;

export namespace ee_economy {

    struct Resource {
        std::uint32_t tag;
        std::uint8_t baseCost;
        std::uint8_t size;
        std::uint8_t mass;
    };
    using ResourceList = std::vector<Resource>;

    struct ProductionRecipe {
        std::uint32_t tag;

        struct Entry {
            std::uint32_t tag;
            std::uint8_t cost;
        };
        std::vector<Entry> in;
        std::vector<Entry> out;
    };

}