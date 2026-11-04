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
    ee_engine::CameraTransform cam = ee_engine::Engine::renderer->getCamera();
    cam.zoom *= pow(1.1f, -dy);
    ee_engine::Engine::renderer->SetCamera(cam);
}

void Actions::MapScroll(const float x, const float y, const float lastX, const float lastY) {
    ee_engine::CameraTransform cam = ee_engine::Engine::renderer->getCamera();

    const glm::vec2 pos = ee_engine::Engine::renderer->ScreenToWorld(x, y);
    const glm::vec2 lastPos = ee_engine::Engine::renderer->ScreenToWorld(lastX, lastY);
    cam.position -= pos - lastPos;

    ee_engine::Engine::renderer->SetCamera(cam);
}

void Actions::MapHover(const float x, const float y) {
    ee_engine::Engine::renderer->RequestProvincePick(x, y);
}

void Actions::MapHoverEnd() {
    ee_engine::Engine::renderer->ClearProvincePick();
}

void Actions::Click(const float x, const float y) {
    const std::optional<ee_engine::ProvinceHit> hit = ee_engine::Engine::renderer->getHoveredProvince();
    if (!hit.has_value()) return;

    const std::uint32_t &regionId = hit->regionId;
    ee_engine::UI::Binded<ee_engine::view::RegionInfo> &regionInfo =
        ee_engine::Engine::ui->GetController<ee_engine::view::RegionInfo>();
    if (regionId == regionInfo.data.id) return;

    if (regionInfo.data.id != 0) {
        ee_engine::ProvinceSSBO lastRegionRenderData = ee_engine::Engine::renderer->getRegionData(regionInfo.data.id);
        ee_engine::RendererCommand updateLastRegionCmd;
        updateLastRegionCmd.type = ee_engine::RendererCommand::UPDATE_REGION;
        updateLastRegionCmd.region.id = regionInfo.data.id;
        lastRegionRenderData.isSelected = false;
        updateLastRegionCmd.region.data = lastRegionRenderData;
        ee_engine::Engine::renderer->Request(updateLastRegionCmd);
    }

    ee_engine::RendererCommand updateRegionCmd;
    ee_engine::ProvinceSSBO regionRenderData = ee_engine::Engine::renderer->getRegionData(regionId);
    updateRegionCmd.type = ee_engine::RendererCommand::UPDATE_REGION;
    updateRegionCmd.region.id = regionId;
    regionRenderData.isSelected = true;
    updateRegionCmd.region.data = regionRenderData;
    ee_engine::Engine::renderer->Request(updateRegionCmd);

    Rml::ElementDocument *popup = ee_engine::Engine::ui->Popup("popups/region-info");
    regionInfo.data.id = regionId;
    regionInfo.data.name = ee_engine::Engine::localizer->Localize(
        "regions",
        regionId
    );
    regionInfo.model.DirtyAllVariables();
}
