#include <ee_engine_api/RenderInterface.hpp>
#include <ee_utils/allocator.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Logger.h"

IRenderer::IRenderer(GLFWwindow *window)
{
    this->window = window;
	glfwGetFramebufferSize(window, &screenWidth, &screenHeight);
	screenAspect = static_cast<float>(screenWidth) / static_cast<float>(screenHeight);
}

IRenderer::~IRenderer()
{
	
}

void IRenderer::RegisterMaterials(const ShaderRefs &shaders)
{
	this->shaders = shaders;
}

void IRenderer::RegisterMapMesh(MapMesh* mapMesh)
{
	this->mapMesh = mapMesh;
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
	screenAspect = static_cast<float>(screenWidth) / static_cast<float>(screenHeight);
}

void IRenderer::SetCamera(const CameraTransform &camera)
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

void IRenderer::SetMapTransform(const MeshTransform &mapTransform)
{
	this->mapTransform = mapTransform;

	glm::mat4 M(1.0f);
	M = glm::translate(M, glm::vec3(mapTransform.position, 0.0f));
    M = glm::rotate(M, mapTransform.rotation, glm::vec3(0.0f, 0.0f, 1.0f));
    M = glm::scale(M, glm::vec3(mapTransform.scale, 1.0f));

	ApplyMapTransform();
}