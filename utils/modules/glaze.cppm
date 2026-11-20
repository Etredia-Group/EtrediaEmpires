module;
#include <glaze/glaze.hpp>
export module glaze;

export namespace glz {
    using namespace unicode;
    using glz::json_t;
    using glz::error_ctx;
    using glz::read_json;
}

namespace glz {

    template <typename TIdentity> concept IIdentityUnion = std::is_union_v<TIdentity> &&
        std::is_integral_v<decltype(TIdentity::tag)> &&
        std::is_integral_v<decltype(TIdentity::id)>;

}

export namespace glz {

    template <IIdentityUnion TIdentity>
    struct from<JSON, TIdentity> {
        template <auto Opts>
        static void op(TIdentity &value, is_context auto &&ctx, auto &&it, auto &&end) {
            parse<JSON>::op<Opts>(value.tag, ctx, it, end);
        }
    };

    template <IIdentityUnion TIdentity>
    struct to<JSON, TIdentity> {
        template <auto Opts>
        static void op(const TIdentity &value, is_context auto &&ctx, auto &&buffer, auto &&index) noexcept {
            serialize<JSON>::op<Opts>(value.tag, ctx, buffer, index);
        }
    };

}