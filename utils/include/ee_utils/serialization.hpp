/*
 * Specializations of non-standart types for reflect-cpp.
 * Include it instead of <rfl/capnproto.hpp>.
 * Do not move this header to module interface to avoid capnptoro compilation errors.
 */

#ifndef SERIALIZATION_HPP
#define SERIALIZATION_HPP

#include <glm/glm.hpp>
#include <rfl/capnproto.hpp>

namespace rfl {
    template <glm::length_t length, typename T>
    struct GlmVecImpl {
        float x; float y;

        static GlmVecImpl from_class(const glm::vec<length, T>& vector) noexcept {
            return {vector.x,vector.y};
        }
        glm::vec<length, T> to_class() const { return {x, y}; }
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
