module;

#define NOMINMAX

#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Version.h>
#include <Magnum/Math/Matrix4.h>

export module MagnumRender:Materials;
import std;

using namespace Magnum;

export namespace ee_magnum {
    class BaseMaterial : public GL::AbstractShaderProgram {
    public:
        inline static constexpr std::int32_t CAMERA_BINDING = 1;
        //TODO: push constant
        inline static constexpr std::int32_t TRANSFORM_BINDING = 2;

        void setModel(const Matrix4& model);
        void setViewProjection(const Matrix4& vewProjection);

    protected:
        explicit BaseMaterial(const std::string& vertSrc, const std::string& fragSrc);
    };

    class ProvinceMaterial final : public BaseMaterial {
    public:
        inline static constexpr std::int32_t PROVINCES_DATA_BINDING = 0;
        inline static constexpr std::int32_t PROVINCES_ID_BINDING = 3;

        explicit ProvinceMaterial(const std::string& vertSrc, const std::string& fragSrc);

        void setId(const std::uint32_t id);
    };
}
