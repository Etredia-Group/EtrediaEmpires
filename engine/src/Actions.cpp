module;

#include <glm/glm.hpp>

module Actions;
import Engine;

ee_engine::InputHandlers Actions::mappings{
    .zooming = Zoom,
    .mapScroll = MapScroll
};

void Actions::Zoom(float dx, float dy) {
    ee_engine::CameraTransform cam = Engine::renderer->getCamera();
    cam.zoom *= pow(1.1f, -dy);
    Engine::renderer->SetCamera(cam);
}

void Actions::MapScroll(float x, float y, float lastX, float lastY) {
    ee_engine::CameraTransform cam = Engine::renderer->getCamera();

    glm::vec2 pos = Engine::renderer->ScreenToWorld(x, y);
    glm::vec2 lastPos = Engine::renderer->ScreenToWorld(lastX, lastY);
    cam.position -= pos - lastPos;

    Engine::renderer->SetCamera(cam);
}
