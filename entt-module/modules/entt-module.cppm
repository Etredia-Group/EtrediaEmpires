module;

#include <entt/entt.hpp>

export module entt;

export namespace entt {
    // ---- config / fwd ----
    using entt::id_type;

    // ---- core ----
    using entt::any;
    using entt::basic_any;
    using entt::any_policy;
    using entt::compressed_pair;
    using entt::family;
    using entt::basic_hashed_string;
    using entt::hashed_string;
    using entt::hashed_wstring;
    using entt::ident;
    using entt::input_iterator_pointer;
    using entt::iterable_adaptor;
    using entt::allocation_deleter;
    using entt::monostate;
    using entt::type_index;
    using entt::type_hash;
    using entt::type_name;
    using entt::type_info;
    using entt::overloaded;
    using entt::y_combinator;
    using entt::std_sort;
    using entt::insertion_sort;
    using entt::radix_sort;

    // ---- core: type traits ----
    using entt::choice_t;
    using entt::type_list;
    using entt::type_list_element;
    using entt::type_list_element_t;
    using entt::type_list_cat;
    using entt::type_list_cat_t;
    using entt::type_list_unique;
    using entt::type_list_unique_t;
    using entt::type_list_diff;
    using entt::type_list_diff_t;
    using entt::value_list;
    using entt::value_list_element;
    using entt::value_list_element_v;
    using entt::value_list_cat;
    using entt::value_list_cat_t;
    using entt::is_ebco_eligible;
    using entt::is_transparent;

    // ---- container ----
    using entt::dense_map;
    using entt::dense_set;

    // ---- entity ----
    using entt::entity;
    using entt::entt_traits;
    using entt::null_t;
    using entt::tombstone_t;
    using entt::component_traits;
    using entt::deletion_policy;
    using entt::registry;
    using entt::basic_registry;
    using entt::sparse_set;
    using entt::basic_sparse_set;
    using entt::storage;
    using entt::basic_storage;
    using entt::storage_type;
    using entt::storage_type_t;
    using entt::storage_for;
    using entt::storage_for_t;
    using entt::sigh_mixin;
    using entt::view;
    using entt::basic_view;
    using entt::runtime_view;
    using entt::basic_runtime_view;
    using entt::group;
    using entt::basic_group;
    using entt::handle;
    using entt::const_handle;
    using entt::handle_view;
    using entt::const_handle_view;
    using entt::basic_handle;
    using entt::organizer;
    using entt::basic_organizer;
    using entt::snapshot;
    using entt::basic_snapshot;
    using entt::snapshot_loader;
    using entt::basic_snapshot_loader;
    using entt::continuous_loader;
    using entt::basic_continuous_loader;
    using entt::get_t;
    using entt::exclude_t;
    using entt::owned_t;
    using entt::as_view;
    using entt::as_group;
    using entt::sigh_helper;

    // ---- graph ----
    using entt::adjacency_matrix;
    using entt::basic_flow;
    using entt::flow;

    // ---- locator ----
    using entt::locator;

    // ---- meta ----
    using entt::meta_any;
    using entt::meta_associative_container;
    using entt::meta_associative_container_traits;
    using entt::meta_class_template_tag;
    using entt::meta_ctx;
    using entt::meta_data;
    using entt::meta_factory;
    using entt::meta_func;
    using entt::meta_handle;
    using entt::meta_range;
    using entt::meta_sequence_container;
    using entt::meta_sequence_container_traits;
    using entt::meta_template_traits;
    using entt::meta_type;
    using entt::is_meta_pointer_like;
    using entt::as_void_t;
    using entt::as_ref_t;
    using entt::as_cref_t;
    using entt::as_is_t;

    // ---- poly ----
    using entt::poly;
    using entt::poly_base;
    using entt::poly_vtable;

    // ---- process ----
    using entt::process;
    using entt::scheduler;
    using entt::basic_scheduler;

    // ---- resource ----
    using entt::resource;
    using entt::resource_cache;
    using entt::resource_loader;

    // ---- signal ----
    using entt::delegate;
    using entt::connection;
    using entt::scoped_connection;
    using entt::sigh;
    using entt::sink;
    using entt::dispatcher;
    using entt::basic_dispatcher;
    using entt::emitter;
    using entt::connect_arg_t;

    // ================= functions and constants =================

    // ---- core ----
    using entt::any_cast;
    using entt::make_any;
    using entt::forward_as_any;
    using entt::type_id;
    using entt::enum_as_bitmask;
    using entt::fast_mod;
    using entt::allocate_unique;
    using entt::uses_allocator_construction_args;
    using entt::make_obj_using_allocator;
    using entt::uninitialized_construct_using_allocator;
    using entt::propagate_on_container_copy_assignment;
    using entt::propagate_on_container_move_assignment;
    using entt::propagate_on_container_swap;

    // ---- core: type traits ----
    using entt::choice;
    using entt::integral_constant;
    using entt::size_of;
    using entt::size_of_v;
    using entt::type_list_contains;
    using entt::type_list_contains_v;
    using entt::type_list_index;
    using entt::type_list_index_v;
    using entt::is_complete;
    using entt::is_complete_v;
    using entt::is_iterator;
    using entt::is_iterator_v;
    using entt::is_ebco_eligible_v;
    using entt::is_transparent_v;
    using entt::is_equality_comparable;
    using entt::is_equality_comparable_v;
    using entt::is_applicable;
    using entt::is_applicable_v;
    using entt::is_applicable_r;
    using entt::is_applicable_r_v;
    using entt::constness_as;
    using entt::constness_as_t;
    using entt::member_class;
    using entt::member_class_t;

    // ---- entity ----
    using entt::null;
    using entt::tombstone;
    using entt::get;
    using entt::exclude;
    using entt::owned;
    using entt::to_integral;
    using entt::to_entity;
    using entt::to_version;
    using entt::invoke;

    // ---- graph ----
    using entt::dot;

    // ---- meta ----
    using entt::resolve;
    using entt::meta_reset;
    using entt::forward_as_meta;
    using entt::adl_meta_pointer_like;

    // ---- poly ----
    using entt::poly_call;

    // ---- literals (_hs, _hws) ----
    namespace literals {
        using literals::operator""_hs;
        using literals::operator""_hws;
    }

    // namespace internal {
    //     using internal::operator==;
    //     using internal::operator!=;
    //     using internal::operator<;
    //     using internal::operator>;
    //     using internal::operator<=;
    //     using internal::operator>=;
    //     using internal::operator-;
    // }
}
