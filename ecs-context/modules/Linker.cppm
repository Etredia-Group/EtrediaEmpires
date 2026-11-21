module;

#include <ee_utils/reflection.hpp>

export module EcsContext:Linker;
import :TagTable;
import utils;
import entt;

namespace ee_ecs {

    template <typename T> concept IString = requires(T t) {
        t.starts_with(std::string_view{});
    };

    template <typename T> concept IContainer = requires(T t) {
        t.begin();
    } && !IString<T>;

    template <typename T> concept IMap = IContainer<T> && requires { typename T::mapped_type; };

}

export namespace ee_ecs {

    class Linker {
    public:
        explicit Linker(entt::registry &registry) : registry(registry) {}

        /**
         * Replace hash tags with runtime ids.
         * @note All tag tables used in game must be registered in context and filled before linkage.
         * @warning Repeating linkage is UB.
         * @tparam TDef Type of the table data.
         * @param table instance of the table.
         */
        template <IDefinition TDef> void Link(TagTable<TDef> &table) {
            LOG(INFO, L"Linking %hs table.", typeid(TDef).name());

            std::span<TDef> definitions = table.GetAll();
            for (TDef &definition : definitions) {
                LinkStruct(definition);
                table.OnLink(definition);
            }
            table.OnLink();

            LOG(INFO, L"Linked %zu definitions.", definitions.size());
        }

    private:
        entt::registry &registry;

        template <typename T>
        void LinkField(T &field) {
            if constexpr (ee::IIdentity<T>) {
                using TableType = table_by_identity_t<T>;

                TableType &table = registry.ctx().get<TableType>();
                field.id = table.FindId(field.tag);
            }
            else if constexpr (IMap<T>) {
                static_assert(
                    !ee::IIdentity<typename T::key_type>,
                    "Tag as a map key cannot be linked in place; rebuild the container instead."
                );
                for (auto &entry : field) {
                    LinkField(entry.second);
                }
            }
            else if constexpr (IContainer<T> || std::is_array_v<T>) {
                for (auto &element : field) {
                    LinkField(element);
                }
            }
            else if constexpr (std::is_aggregate_v<T>) {
                LinkStruct(field);
            }
            else if constexpr (!IString<T>) {
                static_assert(std::is_trivial_v<T>, "Type is not supported for structs registered in Tag Table.");
            }
        }

        template <typename T>
        void LinkStruct(T &data) {
            rfl::to_view(data).apply([&](const auto &field) {
                auto *value = field.value();
                if constexpr (requires { typename T::Tag; }) {
                    if (static_cast<const void*>(value) == static_cast<const void*>(&data.tag)) return;
                }
                LinkField(*value);
            });
        }
    };

}
