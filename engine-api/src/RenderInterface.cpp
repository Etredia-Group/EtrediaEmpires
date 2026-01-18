#include <ee_engine_api/RenderInterface.hpp>
#include <ee_utils/allocator.hpp>

IRenderer::IRenderer(GLFWwindow *window)
{
    this->window = window;
	glfwGetFramebufferSize(window, &screenWidth, &screenHeight);
}

IRenderer::~IRenderer()
{
	
}

void IRenderer::Draw()
{
	Clear();
	DrawMap();
	DrawUI();
}

void IRenderer::ResizeWindow(int width, int height)
{
	screenWidth = width;
	screenHeight = height;
}