#include <benchmark/benchmark.h>

import utils;

int main(int argc, char **argv) {
    // Parses and removes the --benchmark_* flags from argv
    benchmark::Initialize(&argc, argv);
    if (benchmark::ReportUnrecognizedArguments(argc, argv)) {
        return 1;
    }

    Logger::Init(L"/P7.Sink=Console");
    Logger::RegisterThread(L"Main");

    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();

    Logger::UnregisterThread();
    Logger::Release();

    return 0;
}