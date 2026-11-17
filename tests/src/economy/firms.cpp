module;

#include <gtest/gtest.h>

module tests;
import Ecs;
import Economy;

struct EconomyFirm : testing::Test {

    ee_economy::Resource resource0_1 { .tag = static_cast<ee_economy::Resource::Tag>(001) };
    ee_economy::Resource resource0_2 { .tag = static_cast<ee_economy::Resource::Tag>(002) };
    ee_economy::Resource resource1_1 { .tag = static_cast<ee_economy::Resource::Tag>(101) };

    ee_economy::ProductionRecipe recipe_resource1_1 {
        .tag = static_cast<ee_economy::ProductionRecipe::Tag>(0),
        .in = {
            {.resource = static_cast<ee_economy::Resource::Tag>(001), .count = 2},
            {.resource = static_cast<ee_economy::Resource::Tag>(002), .count = 1}
        },
        .out = {
            {.resource = static_cast<ee_economy::Resource::Tag>(101), .count = 3}
        }
    };

    ee_ecs::Pipeline pipeline;

    void SetUp() override {
        pipeline.Init();
    }

    void TearDown() override {

    }
};

// TEST_F(EconomyFirm, Produce) {
//
//
//     ASSERT_TRUE(false);
// }