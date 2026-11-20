/*
 * Specializations of non-standart types for reflect-cpp.
 * Include it instead of <rfl/capnproto.hpp>.
 * Do not move this header to module interface to avoid capnptoro compilation errors.
 */

#ifndef SERIALIZATION_HPP
#define SERIALIZATION_HPP

#include <type_traits>
#include <glm/glm.hpp>
#include <rfl/capnproto.hpp>

namespace rfl {

    template <typename T> concept ScopedEnum = std::is_scoped_enum_v<T>;

    template <ScopedEnum TEnum>
    struct Reflector<TEnum> {
        using ReflType = std::underlying_type_t<TEnum>;
        static ReflType from(const TEnum &tag) noexcept { return static_cast<ReflType>(tag); }
        static TEnum to(const ReflType &value) noexcept { return static_cast<TEnum>(value); }
    };

    template <typename TIdentity> concept IIdentityUnion = std::is_union_v<TIdentity> &&
        std::is_integral_v<decltype(TIdentity::tag)> &&
        std::is_integral_v<decltype(TIdentity::id)>;

    template <IIdentityUnion TIdentity>
    struct Reflector<TIdentity> {
        using ReflType = decltype(TIdentity::id);
        static ReflType from(const TIdentity &identity) noexcept { return identity.tag; }
        static TIdentity to(const ReflType &value) noexcept { return TIdentity{.tag = value}; }
    };

}

namespace rfl {
    template <glm::length_t length, typename Scalar> struct GlmVecImpl;

    template <typename Scalar>
    struct GlmVecImpl<2, Scalar> {
        Scalar x; Scalar y;
        static GlmVecImpl from_class(const glm::vec<2, Scalar> &vector) noexcept { return {vector.x, vector.y}; }
        glm::vec<2, Scalar> to_class() const { return {x, y}; }
    };

    template <typename Scalar>
    struct GlmVecImpl<3, Scalar> {
        Scalar x; Scalar y; Scalar z;
        static GlmVecImpl from_class(const glm::vec<3, Scalar> &vector) noexcept {
            return {vector.x, vector.y, vector.z};
        }
        glm::vec<3, Scalar> to_class() const { return {x, y, z}; }
    };

    template <typename Scalar>
    struct GlmVecImpl<4, Scalar> {
        Scalar x; Scalar y; Scalar z; Scalar w;
        static GlmVecImpl from_class(const glm::vec<4, Scalar> &vector) noexcept {
            return {vector.x, vector.y, vector.z, vector.w};
        }
        glm::vec<4, Scalar> to_class() const { return {x, y, z, w}; }
    };
}

namespace rfl {
    template <class ReaderType, class WriterType, class ProcessorsType>
    struct parsing::Parser<ReaderType, WriterType, glm::vec2, ProcessorsType>
        : CustomParser<ReaderType, WriterType, ProcessorsType, glm::vec2, GlmVecImpl<2, float>> {};

    template <class ReaderType, class WriterType, class ProcessorsType>
    struct parsing::Parser<ReaderType, WriterType, glm::vec3, ProcessorsType>
        : CustomParser<ReaderType, WriterType, ProcessorsType, glm::vec3, GlmVecImpl<3, float>> {};

    template <class ReaderType, class WriterType, class ProcessorsType>
    struct parsing::Parser<ReaderType, WriterType, glm::vec4, ProcessorsType>
        : CustomParser<ReaderType, WriterType, ProcessorsType, glm::vec4, GlmVecImpl<4, float>> {};
}

#endif // SERIALIZATION_HPP
