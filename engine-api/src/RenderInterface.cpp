#include <ee_engine_api/RenderInterface.hpp>
#include <ee_utils/allocator.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Logger.h"

ee_engine::IRenderer::IRenderer(GLFWwindow *window)
{
    this->window = window;
	glfwGetFramebufferSize(window, &screenWidth, &screenHeight);
	screenAspect = static_cast<float>(screenWidth) / static_cast<float>(screenHeight);
}

ee_engine::IRenderer::~IRenderer()
{
	
}

void ee_engine::IRenderer::RegisterMaterials(const ShaderRefs &shaders)
{
	this->shaders = shaders;
}

void ee_engine::IRenderer::RegisterMapMesh(MapMesh* mapMesh)
{
	this->mapMesh = mapMesh;
}

void ee_engine::IRenderer::Draw()
{
	Clear();
	DrawMap();
	DrawUI();
}

void ee_engine::IRenderer::ResizeWindow(int width, int height)
{
	screenWidth = width;
	screenHeight = height;
	screenAspect = static_cast<float>(screenWidth) / static_cast<float>(screenHeight);
}

void ee_engine::IRenderer::SetCamera(const CameraTransform &camera)
{
	if (camera.zoom <= 0)
	{
		LOG(WARNING, "Camera zoom cannot be zero or less!");
		return;
	}

	this->camera = camera;

	float halfHeight = camera.zoom;
	float halfWidth = camera.zoom * screenAspect;

	glm::mat4 P = glm::ortho(
        -halfWidth, +halfWidth,
        -halfHeight, +halfHeight
    );

    glm::mat4 V = glm::translate(
        glm::mat4(1.0f),
        glm::vec3(-camera.position, 0.0f)
    );

	viewProjection = P * V;

	ApplyCamera();
}