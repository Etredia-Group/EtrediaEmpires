#include "Actions.hpp"
#include "Engine.hpp"
#include <cmath>

ee_engine::InputHandlers Actions::mappings {
    .zooming = Zoom,
    .mapScroll = MapScroll
};

void Actions::Zoom(float dx, float dy)
{
    ee_engine::CameraTransform cam = Engine::renderer->getCamera();
    cam.zoom *= pow(1.1f, -dy);
    Engine::renderer->SetCamera(cam);
}

void Actions::MapScroll(float dx, float dy)
{

}