#include <ee_magnum/Materials.hpp>
#include "Logger.h"

ee_magnum::BaseMaterial::BaseMaterial(const std::string &vertSrc, const std::string &fragSrc)
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

void ee_magnum::BaseMaterial::setModel(const Matrix4 &m)
{
    setUniform(TRANSFORM_UNIFORM, m);
}

void ee_magnum::BaseMaterial::setViewProjection(const Matrix4 &vp)
{
    setUniform(CAMERA_UNIFORM, vp);
}

ee_magnum::ProvinceMaterial::ProvinceMaterial(const std::string &vertSrc, const std::string &fragSrc): BaseMaterial(vertSrc, fragSrc)
{

}

void ee_magnum::ProvinceMaterial::setId(const uint32_t &id)
{
    setUniform(PROVINCES_ID_UNIFORM, id);
}