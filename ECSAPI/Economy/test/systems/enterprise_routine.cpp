#include <gtest/gtest.h>
#include <ee_economy/economy.hpp>
#include "../test_conf.hpp"

TEST_F(SystemsTest, RoutineNormal)
{
    ee_economy::enterprise &miner_enterprise = registry.get<ee_economy::enterprise>(miner);
    miner_enterprise.set_money(1000000000.0);
    miner_enterprise.salary = 100000.0;
    miner_enterprise.staff_size = 10;

    ee_economy::enterprise_routine(registry);

    ASSERT_DOUBLE_EQ(miner_enterprise.total_money, 999000000.0);
    ASSERT_FALSE(miner_enterprise.is_bankrupt);
}

TEST_F(SystemsTest, RoutineBankruptcy)
{
    ee_economy::enterprise &miner_enterprise = registry.get<ee_economy::enterprise>(miner);
    miner_enterprise.set_money(1000000.0);
    miner_enterprise.salary = 100000.0;
    miner_enterprise.staff_size = 11;

    ee_economy::enterprise_routine(registry);

    ASSERT_DOUBLE_EQ(miner_enterprise.total_money, 0.0);
    ASSERT_TRUE(miner_enterprise.is_bankrupt);
}