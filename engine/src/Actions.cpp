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

    const std::uint32_t &regionId = hit->regionId;
    Rml::Binded<ee_engine::RegionInfo> &regionInfo = Engine::ui->regionInfo;
    if (regionId == regionInfo.data.id) return;

    if (regionInfo.data.id != 0) {
        ee_engine::ProvinceSSBO lastRegionRenderData = Engine::renderer->getRegionData(regionInfo.data.id);
        ee_engine::RendererCommand updateLastRegionCmd;
        updateLastRegionCmd.type = ee_engine::RendererCommand::UPDATE_REGION;
        updateLastRegionCmd.region.id = regionInfo.data.id;
        lastRegionRenderData.isSelected = false;
        updateLastRegionCmd.region.data = lastRegionRenderData;
        Engine::renderer->Request(updateLastRegionCmd);
    }

    ee_engine::RendererCommand updateRegionCmd;
    ee_engine::ProvinceSSBO regionRenderData = Engine::renderer->getRegionData(regionId);
    updateRegionCmd.type = ee_engine::RendererCommand::UPDATE_REGION;
    updateRegionCmd.region.id = regionId;
    regionRenderData.isSelected = true;
    updateRegionCmd.region.data = regionRenderData;
    Engine::renderer->Request(updateRegionCmd);

    Rml::ElementDocument *popup = Engine::ui->Popup("assets/layout/popups/region-info/region-info.rml");
    regionInfo.data.id = regionId;
    regionInfo.data.name = Engine::localizer->Localize(
        "regions",
        regionId
    );
    regionInfo.model.DirtyAllVariables();
}
