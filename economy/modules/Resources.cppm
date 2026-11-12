export module Economy:Resources;
import aliases;
import EcsContext;
import std;

export namespace ee_economy {

    struct Resource {
        ee::ResourceTag tag;
        std::uint8_t baseCost;
        std::uint8_t size;
        std::uint8_t mass;
    };
    using ResourceList = std::vector<Resource>;
    class ResourceTable final : public ee_ecs::TagTable<Resource, ee::ResourceId> {};

    struct ProductionRecipe {
        ee::RecipeTag tag;

        struct Entry {
            ee::ResourceTag tag;
            ee::ResourceAmount count;
        };
        std::vector<Entry> in;
        std::vector<Entry> out;
    };
    using ProductionRecipeList = std::vector<ProductionRecipe>;
    class ProductionRecipeTable final : public ee_ecs::TagTable<ProductionRecipe, ee::RecipeId> {};
}