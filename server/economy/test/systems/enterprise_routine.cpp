#include <gtest/gtest.h>
#include <ee_economy/economy.hpp>
#include "../test_conf.hpp"

TEST_F(SystemsTest, RoutineNormal)
{
    ee_economy::Enterprise &minerEnterprise = registry.get<ee_economy::Enterprise>(miner);
    minerEnterprise.setMoney(1000000000.0);
    minerEnterprise.salary = 100000.0;
    minerEnterprise.staffSize = 10;

    ee_economy::enterpriseRoutine(registry);

    ASSERT_DOUBLE_EQ(minerEnterprise.totalMoney, 999000000.0);
    ASSERT_FALSE(minerEnterprise.isBankrupt);
}

TEST_F(SystemsTest, RoutineBankruptcy)
{
    ee_economy::Enterprise &minerEnterprise = registry.get<ee_economy::Enterprise>(miner);
    minerEnterprise.setMoney(1000000.0);
    minerEnterprise.salary = 100000.0;
    minerEnterprise.staffSize = 11;

    ee_economy::enterpriseRoutine(registry);

    ASSERT_DOUBLE_EQ(minerEnterprise.totalMoney, 0.0);
    ASSERT_TRUE(minerEnterprise.isBankrupt);
}