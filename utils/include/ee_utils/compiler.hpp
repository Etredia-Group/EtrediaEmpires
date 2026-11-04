#ifndef ETREDIAEMPIRES_COMPILER_HPP
#define ETREDIAEMPIRES_COMPILER_HPP

#include <type_traits>
#include <cstddef>

#if defined(_MSC_VER)
#define NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
#define NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif

namespace ee_utils {
    struct NoMember {
        constexpr NoMember() = default;
        // ReSharper disable once CppNonExplicitConvertingConstructor
        constexpr NoMember(std::nullptr_t) {}
    };

    template <typename T, bool condition>
    using optional = std::conditional_t<condition, T, NoMember>;
}

#endif //ETREDIAEMPIRES_COMPILER_HPP