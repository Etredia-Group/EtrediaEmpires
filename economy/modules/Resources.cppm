export module Economy:Resources;
import aliases;
import EcsContext;
import std;

export namespace ee_economy {

    struct Resource {
        using Tag = ee::ResourceIdentity; Tag tag;

        std::uint16_t baseCost;
        ee::ResourceSize size;
        ee::ResourceMass mass;
    };
    using ResourceList = std::vector<Resource>;
    class ResourceTable final : public ee_ecs::TagTable<Resource> {};

    struct ProductionRecipe {
        using Tag = ee::RecipeIdentity; Tag tag;

        struct Entry {
            ee::ResourceIdentity resource;
            ee::ResourceAmount count;
        };
        std::vector<Entry> in;
        std::vector<Entry> out;
    };
    using ProductionRecipeList = std::vector<ProductionRecipe>;
    class ProductionRecipeTable final : public ee_ecs::TagTable<ProductionRecipe> {};

}

template<> struct ee_ecs::table_by_tag<ee::ResourceIdentity>{ using type = ee_economy::ResourceTable; };
template<> struct ee_ecs::table_by_tag<ee::RecipeIdentity>{ using type = ee_economy::ProductionRecipeTable; };