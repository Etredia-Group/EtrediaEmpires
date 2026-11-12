module;

#include <entt/entt.hpp>

export module EcsContext:TagTable;
import aliases;
import std;

export namespace ee_ecs {

    template <typename TDef, typename TId>
    class TagTable {
    public:
        virtual ~TagTable() = default;

        std::optional<TId> Add(std::vector<TDef> &&batch) {
            if (definitions.size() + batch.size() > MAX_DEFINITIONS) return std::nullopt;

            entt::dense_set<ee::Tag> seen;
            seen.reserve(batch.size());
            for (const TDef &def : batch) {
                if (defByTag.find(def.tag) != defByTag.end()) return std::nullopt;
                if (!seen.insert(def.tag).second) return std::nullopt;
            }

            const TId first = static_cast<TId>(definitions.size());
            definitions.reserve(definitions.size() + batch.size());
            defByTag.reserve(defByTag.size() + batch.size());
            for (TDef &def : batch) {
                defByTag.try_emplace(def.tag, static_cast<TId>(definitions.size()));
                OnAdd(def);
                definitions.push_back(std::move(def));
            }
            return first;
        }

        TId GetId(const ee::Tag tag) const { return defByTag.at(tag); }
        const TDef& Get(const ee::Tag tag) const { return definitions[GetId(tag)]; }
        const TDef& operator[](TId id) const noexcept { return definitions[id]; }

        std::size_t getSize() const noexcept {return definitions.size();}

    protected:
        static constexpr std::size_t MAX_DEFINITIONS = std::size_t{std::numeric_limits<TId>::max()} + 1;

        std::vector<TDef> definitions;
        entt::dense_map<ee::Tag, TId> defByTag;

        virtual void OnAdd(const TDef &definition) {}
    };

}