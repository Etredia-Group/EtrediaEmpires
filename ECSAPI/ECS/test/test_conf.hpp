#pragma once

#include <gtest/gtest.h>
#include <ee_ecs/ecs.hpp>

struct WorldTest : testing::Test
{
    ee_ecs::world world;

    void SetUp()
    {

    }

    void TearDown()
    {

    }
};