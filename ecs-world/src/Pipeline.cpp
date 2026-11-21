module Ecs;
import aliases;
import utils;

ee_ecs::Pipeline::Pipeline() :
    linker(registry) {}

void ee_ecs::Pipeline::Init() {
    RegisterContext();
}

void ee_ecs::Pipeline::Tick() {

    ee_economy::systems::FirmsProduce(registry);

}

void ee_ecs::Pipeline::RegisterContext() {
    LOG(INFO, L"Initializing ECS registry context.");

    LOG(INFO, L"Initializing tag tables.");
    registry.ctx().emplace<ee_economy::ResourceTable>();
    ee_economy::ProductionRecipeTable &recipes = registry.ctx().emplace<ee_economy::ProductionRecipeTable>();
    const ee::ResourceLayout sharedResources {}; //TODO
    ee_economy::FirmDefinitionTable &firms =
        registry.ctx().emplace<ee_economy::FirmDefinitionTable>(registry, sharedResources);

    LOG(INFO, L"Linking tag tables with runtime identifiers.");
    linker.Link(recipes);
    linker.Link(firms);

    LOG(INFO, L"Initializing data cache.");
    registry.ctx().emplace<ee_economy::StoragePools>(firms);
    registry.ctx().emplace<RegionIndex>();

    LOG(INFO, L"ECS registry context is ready.");
}
