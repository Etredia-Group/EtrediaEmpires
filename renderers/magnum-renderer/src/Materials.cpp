#include <ee_magnum/Materials.hpp>
#include "Logger.h"

BaseMaterial::BaseMaterial(const std::string &vertSrc, const std::string &fragSrc)
{
    GL::Shader vert {GL::Version::GL430, GL::Shader::Type::Vertex};
    GL::Shader frag {GL::Version::GL430, GL::Shader::Type::Fragment};

    vert.addSource(vertSrc);
    frag.addSource(fragSrc);

    if (!vert.compile())
        LOG(CRITICAL, "Failed to compile vertex shader");
    if (!frag.compile())
        LOG(CRITICAL, "Failed to compile fragment shader");

    attachShader(vert);
    attachShader(frag);
    
    link();
}

void BaseMaterial::setModelViewProjection(const Matrix4 &m, const Matrix4 &vp)
{
    setUniform(TRANSFORM_UNIFORM, m);
    setUniform(CAMERA_UNIFORM, vp);
}

ProvinceMaterial::ProvinceMaterial(const std::string &vertSrc, const std::string &fragSrc): BaseMaterial(vertSrc, fragSrc)
{

}

void ProvinceMaterial::setId(const uint32_t &id)
{
    setUniform(PROVINCES_ID_UNIFORM, id);
}