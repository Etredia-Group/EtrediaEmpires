module;

#include <glm/glm.hpp>

module Actions;
import Engine;
import utils;
import std;

ee_engine::InputHandlers Actions::mappings{
    .click = Click,
    .zooming = Zoom,
    .mapScroll = MapScroll,
    .mapHover = MapHover,
    .mapHoverEnd = MapHoverEnd
};

void Actions::Zoom(const float dx, const float dy) {
    ee_engine::CameraTransform cam = Engine::renderer->getCamera();
    cam.zoom *= pow(1.1f, -dy);
    Engine::renderer->SetCamera(cam);
}

void Actions::MapScroll(const float x, const float y, const float lastX, const float lastY) {
    ee_engine::CameraTransform cam = Engine::renderer->getCamera();

    const glm::vec2 pos = Engine::renderer->ScreenToWorld(x, y);
    const glm::vec2 lastPos = Engine::renderer->ScreenToWorld(lastX, lastY);
    cam.position -= pos - lastPos;

    Engine::renderer->SetCamera(cam);
}

void Actions::MapHover(const float x, const float y) {
    Engine::renderer->RequestProvincePick(x, y);
}

void Actions::MapHoverEnd() {
    Engine::renderer->ClearProvincePick();
}

void Actions::Click(const float x, const float y) {
    const std::optional<ee_engine::ProvinceHit> hit = Engine::renderer->getHoveredProvince();
    if (!hit.has_value()) return;

    LOG(INFO, "Clicked region %u", hit->regionId);
}
