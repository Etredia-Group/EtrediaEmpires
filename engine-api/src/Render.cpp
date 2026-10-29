module;

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

module Render;
import std;
import utils;

ee_engine::IRenderer::IRenderer(GLFWwindow *window) {
    commandsMapping[RendererCommand::UPDATE_REGION] = std::bind_front(&IRenderer::UpdateRegionCmd, this);

    this->window = window;
    glfwGetFramebufferSize(window, &screenWidth, &screenHeight);
    screenAspect = static_cast<float>(screenWidth) / static_cast<float>(screenHeight);
}

ee_engine::IRenderer::~IRenderer() {}

void ee_engine::IRenderer::RegisterAssetManager(AssetManager &assetManager) {
    this->assetManager = &assetManager;
}

void ee_engine::IRenderer::RegisterMaterials(const ShaderRefs& shaders) {
    this->shaders = shaders;
}

void ee_engine::IRenderer::RegisterMapMesh(MapMesh *mapMesh) {
    this->mapMesh = mapMesh;
}

void ee_engine::IRenderer::SetUIContext(Rml::Context *rmlContext) {
    this->rmlContext = rmlContext;
}

void ee_engine::IRenderer::Draw() {
    if (!isDrawing) return;

    if (DrawBegin()) {
        if (isMapLoaded) {
            DrawMap();
        }
        DrawUI();
        DrawEnd();
    }
}

void ee_engine::IRenderer::ResizeWindow(const int width, const int height) {
    screenWidth = width;
    screenHeight = height;
    screenAspect = static_cast<float>(screenWidth) / static_cast<float>(screenHeight);
}

void ee_engine::IRenderer::SetCamera(const CameraTransform& camera) {
    if (camera.zoom <= 0) {
        LOG(WARNING, "Camera zoom cannot be zero or less!");
        return;
    }

    this->camera = camera;

    const float halfHeight = camera.zoom;
    const float halfWidth = camera.zoom * screenAspect;

    const glm::mat4 P = glm::ortho(
        -halfWidth, +halfWidth,
        -halfHeight, +halfHeight
    );

    const glm::mat4 V = glm::translate(
        glm::mat4(1.0f),
        glm::vec3(-camera.position, 0.0f)
    );

    viewProjection = P * V;
    viewProjectionInversed = glm::inverse(viewProjection);

    ApplyCamera();
}

void ee_engine::IRenderer::RequestProvincePick(const float screenX, const float screenY) {
    RequestProvincePickAt(ScreenToWorld(screenX, screenY));
}

void ee_engine::IRenderer::ClearProvincePick() {
    RequestProvincePickAt(std::nullopt);
}

glm::vec2 ee_engine::IRenderer::ScreenToWorld(const float screenX, const float screenY) const {
    return ScreenToWorld(glm::vec2(screenX, screenY));
}

glm::vec2 ee_engine::IRenderer::ScreenToWorld(const glm::vec2& screenPos) const {
    const float x = 2.0f * screenPos.x / screenWidth - 1.0f;
    const float y = 2.0f * screenPos.y / screenHeight - 1.0f;
    //TODO: optional Y inversion for OpenGL backend support

    const glm::vec4 ndc(x, y, 0.0f, 1.0f);
    const glm::vec4 world = viewProjectionInversed * ndc;
    return glm::vec2(world) / world.w;
}

void ee_engine::IRenderer::UpdateRegionCmd(RendererCommand& command) {
    if (!regionsMapping.contains(command.region.id)) {
        LOG(WARNING, "Updating unregistered region with id: %d", command.region.id);
        return;
    }

    if (command.region.id != command.region.data.regionId) {
        LOG(ERROR, "Updating regions ID is not supported! Region: %d Changed ID: %d",
            command.region.id, command.region.data.regionId);
    }

    const auto [offset, provincesCount] = regionsMapping[command.region.id];
    std::vector<ProvinceSSBO> buffer;
    buffer.reserve(provincesCount);
    for (std::size_t i = 0; i < provincesCount; i++) {
        buffer.push_back(command.region.data);
    }

    UpdateProvinces(offset, buffer);

    command.type = RendererCommand::COMPLETED;
}
