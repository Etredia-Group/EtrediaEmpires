#include <gtest/gtest.h>

import utils;

int main(int argc, char **argv)
{
    Logger::Init(L"/P7.Sink=Console");
    Logger::RegisterThread(L"Main");
    testing::InitGoogleTest(&argc, argv);

    const int result = RUN_ALL_TESTS();

    Logger::UnregisterThread();
    Logger::Release();

    return result;
}