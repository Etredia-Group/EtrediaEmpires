#include "Logger.h"
#include <ee_magnum/Renderer.hpp>

#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Shaders/VertexColor.h>

#include <Corrade/Utility/Debug.h>

using namespace Math::Literals;

MagnumRenderer::MagnumRenderer(GLFWwindow *window) : IRenderer(window)
{

}

MagnumRenderer::~MagnumRenderer()
{
    
}

void MagnumRenderer::Clear()
{
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color);
}

void MagnumRenderer::DrawMap()
{
    //TEST TRIANGLE
    struct TriangleVertex {
        Vector2 position;
        Color3 color;
    };
    const TriangleVertex vertices[]{
        {{-0.5f, -0.5f}, 0xff0000_rgbf},    /* Left vertex, red color */
        {{ 0.5f, -0.5f}, 0x00ff00_rgbf},    /* Right vertex, green color */
        {{ 0.0f,  0.5f}, 0x0000ff_rgbf}     /* Top vertex, blue color */
    };

    GL::Mesh mesh;
    mesh.setCount((int)Containers::arraySize(vertices))
        .addVertexBuffer(GL::Buffer{vertices}, 0,
            Shaders::VertexColor2D::Position{},
            Shaders::VertexColor2D::Color3{});

    Shaders::VertexColor2D shader;

    shader.draw(mesh);
}

void MagnumRenderer::DrawUI()
{
    
}