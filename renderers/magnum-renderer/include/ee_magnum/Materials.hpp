#pragma once

#undef min
#undef max

#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Version.h>
#include <Magnum/Math/Matrix4.h>
#include <glm/glm.hpp>

#define PROVINCES_SSBO_BINDING 0
#define CAMERA_UNIFORM 1
//TODO: push constant
#define TRANSFORM_UNIFORM 2
#define PROVINCES_ID_UNIFORM 3

using namespace Magnum;

namespace ee_magnum
{
    class BaseMaterial : public GL::AbstractShaderProgram
    {
    public:
        void setModel(const Matrix4 &model);
        void setViewProjection(const Matrix4 &vewProjection);
    
    protected:
        explicit BaseMaterial(const std::string &vertSrc, const std::string &fragSrc);
    };
    
    class ProvinceMaterial : public BaseMaterial
    {
    public:
        explicit ProvinceMaterial(const std::string &vertSrc, const std::string &fragSrc);
        
        void setId(const uint32_t &id);
    };
}