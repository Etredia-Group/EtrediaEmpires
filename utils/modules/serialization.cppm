module;
#include <glm/glm.hpp>
#include <rfl/capnproto.hpp>

export module utils:serialization;

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

export namespace rfl {
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

export namespace ee_utils {

}