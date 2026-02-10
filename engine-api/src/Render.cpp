module;

#include "Logger.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <ee_utils/utils.hpp>
#include <cstdint>
#include <cstddef>
#include <vector>

module Render;

ee_engine::IRenderer::IRenderer(GLFWwindow *window)
{
    commandsMapping[RendererCommand::UPDATE_REGION] = MAP_WORKER_COMMAND(RendererCommand, UpdateRegionCmd);

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

void ee_engine::IRenderer::UpdateRegionCmd(ee_engine::RendererCommand &command)
{
    if (!regionsMapping.contains(command.region.id))
    {
        LOG(WARNING, "Updating unregistered region with id: %d", command.region.id);
        return;
    }

    if (command.region.id != command.region.data.regionId)
    {
        LOG(ERROR, "Updating regions ID is not supported! Region: %d Changed ID: %d",
            command.region.id, command.region.data.regionId);
    }

    RegionIndecies mapping = regionsMapping[command.region.id];
    std::vector<ProvinceSSBO> buffer;
    buffer.reserve(mapping.provincesCount);
    for (std::size_t i = 0; i < mapping.provincesCount; i++)
    {
        buffer.push_back(command.region.data);
    }

    UpdateProvinces(mapping.startIndex, buffer);

    command.type = ee_engine::RendererCommand::COMPLETED;
}