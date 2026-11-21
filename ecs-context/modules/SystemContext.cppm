export module EcsContext:SystemContext;
import entt;
import std;

export namespace ee_ecs::systems {

    template <typename TComponent> auto FetchEntities(entt::registry &registry) = delete;
    template <typename TSystemContext> TSystemContext FetchContext(entt::registry &registry) = delete;

    /**
     *
     * @tparam TParallelThreshold Amount of entities when system should iterate in parallel.
     * @tparam TPar Parallel policy for system iterating. par or par_unseq.
     */
    template <
        std::size_t TParallelThreshold,
        auto TPar
    >
    struct SystemPolicy {
        // Amount of entities when system should iterate in parallel.
        static constexpr std::size_t parallelThreshold = TParallelThreshold;
        // Parallel policy for system iterating. par or par_unseq.
        static constexpr auto par = TPar;
    };

}