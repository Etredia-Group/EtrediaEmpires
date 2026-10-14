export module Economy:Corporation;
import std;

export namespace ee_economy {

    struct EnterpriseDefinition {
        std::uint32_t productionRecipe;
        std::uint32_t constructionRecipe;
        std::uint8_t constructionSpeed;
    };
    using EnterpriseList = std::vector<EnterpriseDefinition>;

    class Enterprise {
    public:
        EnterpriseDefinition definition;
        std::uint32_t stateId;
        std::uint8_t damage;
    };

    class Corporation {
    public:
        std::vector<Enterprise> enterprises;
    };

}