#include <ee_ecs/ecs.hpp>
#include "test_conf.hpp"
#include <gtest/gtest.h>
#include <chrono>

struct timer final {
    timer()
        : start{std::chrono::system_clock::now()} {}

    void elapsed() {
        auto now = std::chrono::system_clock::now();
        std::cout << std::chrono::duration<double>(now - start).count() << " seconds" << std::endl;
    }

private:
    std::chrono::time_point<std::chrono::system_clock> start;
};

template<typename Func, typename... Args>
void generic_with(Func func) {
    timer timer;
    func();
    timer.elapsed();
}

TEST_F(WorldBenchmark, UpdateSystems)
{
    generic_with([&]() {
        world.update();
    });
}