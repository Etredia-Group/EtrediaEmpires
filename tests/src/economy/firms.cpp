#include <gtest/gtest.h>

import Economy;

struct EconomyFirm : testing::Test {

    ee_economy::Resource resource0_1 { .tag = 001 };
    ee_economy::Resource resource0_2 { .tag = 002 };
    ee_economy::Resource resource1_1 { .tag = 101 };

    ee_economy::ProductionRecipe recipe_resource1_1 {
        .tag = 0,
        .in = {
            {.tag = 001, .count = 2},
            {.tag = 002, .count = 1}
        },
        .out = {
            {.tag = 101, .count = 3}
        }
    };

    void SetUp() override {

    }

    void TearDown() override {

    }
};

TEST_F(EconomyFirm, Produce) {


    ASSERT_TRUE(false);
}