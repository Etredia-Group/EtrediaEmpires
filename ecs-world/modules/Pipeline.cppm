export module Ecs:Pipeline;
import entt;
import EcsContext;
import Economy;

export namespace ee_ecs {

    class Pipeline {
    public:
        Pipeline();
        void Init();

        void Tick();

        entt::registry& getRegistry() noexcept { return registry; }

    private:
        entt::registry registry;
        Linker linker;

        void RegisterContext();
    };

}