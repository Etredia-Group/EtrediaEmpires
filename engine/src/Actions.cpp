module;

#include <glm/glm.hpp>

module Actions;
import Engine;

ee_engine::InputHandlers Actions::mappings{
    .zooming = Zoom,
    .mapScroll = MapScroll
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
