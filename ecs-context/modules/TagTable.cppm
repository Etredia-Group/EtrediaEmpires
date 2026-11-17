export module EcsContext:TagTable;
import std;
import entt;
import aliases;

export namespace ee_ecs {

    template <typename T> concept IDefinition = requires { ee::ITag<typename T::Tag>; };

    class TagTableBase {};

    template <IDefinition TDef>
    class TagTable : public TagTableBase {
    public:
        virtual ~TagTable() = default;

        std::optional<ee::Id> Add(std::vector<TDef> &&batch) {
            if (definitions.size() + batch.size() > MAX_DEFINITIONS) return std::nullopt;

            entt::dense_set<ee::Tag> seen;
            seen.reserve(batch.size());
            for (const TDef &def : batch) {
                if (defByTag.find(std::to_underlying(def.tag)) != defByTag.end()) return std::nullopt;
                if (!seen.insert(std::to_underlying(def.tag)).second) return std::nullopt;
            }

            const ee::Id first = static_cast<ee::Id>(definitions.size());
            definitions.reserve(definitions.size() + batch.size());
            defByTag.reserve(defByTag.size() + batch.size());
            for (TDef &def : batch) {
                defByTag.try_emplace(std::to_underlying(def.tag), static_cast<ee::Id>(definitions.size()));
                definitions.push_back(std::move(def));
                OnAdd(definitions.back());
            }
            return first;
        }

        ee::Id FindId(const ee::Tag tag) const { return defByTag.at(tag); }
        const TDef& FindItem(const ee::Tag tag) const { return definitions[FindId(tag)]; }
        const TDef& Get(const ee::Id id) const noexcept { return definitions[id]; }
        std::span<TDef> GetAll() { return std::span(definitions); }

        std::size_t getSize() const noexcept {return definitions.size();}

        virtual void OnLink(const TDef &definition) {}
        virtual void OnLink() {}

    protected:
        static constexpr std::size_t MAX_DEFINITIONS = std::size_t{std::numeric_limits<ee::Id>::max()} + 1;

        std::vector<TDef> definitions;
        entt::dense_map<ee::Tag, ee::Id> defByTag;

        virtual void OnAdd(const TDef &definition) {}
    };

    template <typename T> concept ITagTable = std::is_base_of_v<TagTableBase, T>;
    template <ee::ITag T> struct table_by_tag;
    template <ee::ITag T> using table_by_tag_t = table_by_tag<T>::type;
}