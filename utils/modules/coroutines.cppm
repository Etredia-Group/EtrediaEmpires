export module utils:coroutines;
import std;

export namespace ee_utils {
    // ReSharper disable once CppParameterMayBeConst
    std::generator<float> ScaledCoroutine(std::generator<float> routine, const float from, const float to) {
        for (const float point : routine) {
            co_yield from + (to - from) * point;
        }
    }
}