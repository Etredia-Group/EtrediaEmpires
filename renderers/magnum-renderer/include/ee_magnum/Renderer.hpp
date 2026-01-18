#pragma once

#undef min
#undef max

#include <Magnum/Platform/GLContext.h>
#include <GLFW/glfw3.h>

#include <ee_engine_api/RenderInterface.hpp>

using namespace Magnum;

class MagnumRenderer : public IRenderer
{
public:
    MagnumRenderer(GLFWwindow *window);
    ~MagnumRenderer();

protected:
    void Clear() override;
    void DrawMap() override;
    void DrawUI() override;

private:
    Platform::GLContext context;
};