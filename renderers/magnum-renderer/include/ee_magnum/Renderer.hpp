#pragma once

#undef min
#undef max

#include <Magnum/Platform/GLContext.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Buffer.h>
#include <GLFW/glfw3.h>

#include "Materials.hpp"
#include <ee_engine_api/RenderInterface.hpp>

using namespace Magnum;

class MagnumRenderer : public IRenderer
{
public:
    MagnumRenderer(GLFWwindow *window);
    ~MagnumRenderer();
    
    void Init() override;
    void Shutdown() override;

protected:
    void Clear() override;
    void DrawMap() override;
    void DrawUI() override;

    void ApplyCamera() override;
    void ApplyMapTransform() override;

private:
    Platform::GLContext context;

    std::vector<GL::Mesh> provinceMeshes;
    GL::Buffer provinceSSBO;
    ProvinceMaterial *provinceMaterial;

    void ValidateSupport();
    void LoadShaders();
    void LoadProvinces();

    void ApplyMapMVP();
};