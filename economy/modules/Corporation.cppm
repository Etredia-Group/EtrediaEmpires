export module Economy:Corporation;
import std;

export namespace ee_economy {

    struct EnterpriseDefinition {
        std::uint32_t productionRecipe;
        std::uint32_t constructionRecipe;
        std::uint8_t constructionSpeed;
    };
    using EnterpriseList = std::vector<EnterpriseDefinition>;

    class EnterpriseBatch {
    public:
        EnterpriseDefinition definition;
        std::uint32_t regionId;
        std::uint8_t count;
        std::uint8_t damage;
        std::uint8_t efficiency;
    };

    class Corporation {
    public:
        std::vector<EnterpriseBatch> enterprises;
    };

}