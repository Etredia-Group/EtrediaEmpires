module;

#include <ee_utils/compiler.hpp>
#include <rfl/to_view.hpp>
#include <rfl/type_name_t.hpp>
#include <RmlUi/Core.h>
#include <RmlUi/Core/TextInputContext.h>

export module rml;

// ---------------------------------------------------------------------------
// Free functions (lifecycle, contexts, fonts, interfaces, plugins)
// ---------------------------------------------------------------------------
export namespace Rml {
    using Rml::Initialise;
    using Rml::Shutdown;
    using Rml::GetVersion;

    using Rml::CreateContext;
    using Rml::RemoveContext;
    using Rml::GetContext;
    using Rml::GetNumContexts;

    using Rml::LoadFontFace;

    using Rml::SetSystemInterface;
    using Rml::GetSystemInterface;
    using Rml::SetRenderInterface;
    using Rml::GetRenderInterface;
    using Rml::SetFileInterface;
    using Rml::GetFileInterface;
    using Rml::SetFontEngineInterface;
    using Rml::GetFontEngineInterface;
    using Rml::SetTextInputHandler;
    using Rml::GetTextInputHandler;

    using Rml::RegisterPlugin;
    using Rml::RegisterEventType;

    using Rml::ReleaseTextures;
    using Rml::ReleaseCompiledGeometry;

    using Rml::Factory;
}

// ---------------------------------------------------------------------------
// Basic types, containers, handles, math
// ---------------------------------------------------------------------------
export namespace Rml {
    using Rml::byte;
    using Rml::Character;

    using Rml::String;
    using Rml::StringView;
    using Rml::StringList;
    using Rml::Span;
    using Rml::Variant;
    using Rml::Dictionary;
    using Rml::VariantList;

    using Rml::FileHandle;
    using Rml::TextureHandle;
    using Rml::CompiledGeometryHandle;
    using Rml::CompiledFilterHandle;
    using Rml::CompiledShaderHandle;
    using Rml::FontFaceHandle;

    using Rml::Vector2i;
    using Rml::Vector2f;
    using Rml::Vector3i;
    using Rml::Vector3f;
    using Rml::Vector4i;
    using Rml::Vector4f;
    using Rml::Matrix4f;
    using Rml::Rectanglei;
    using Rml::Rectanglef;

    using Rml::Colourb;
    using Rml::Colourf;
    using Rml::ColourbPremultiplied;

    using Rml::Vertex;
    using Rml::Mesh;
    using Rml::Texture;
    using Rml::CallbackTexture;
    using Rml::CallbackTextureInterface;
    using Rml::CallbackTextureFunction;
    using Rml::Geometry;
    using Rml::RenderManager;

    using Rml::ObserverPtr;
}

// ---------------------------------------------------------------------------
// Properties, units, box model, style
// ---------------------------------------------------------------------------
export namespace Rml {
    using Rml::Property;
    using Rml::PropertyId;
    using Rml::PropertyIdSet;
    using Rml::PropertyDictionary;
    using Rml::PropertyDefinition;
    using Rml::PropertySpecification;
    using Rml::ShorthandId;
    using Rml::Unit;
    using Rml::NumericValue;

    using Rml::Box;
    using Rml::BoxArea;
    using Rml::BoxDirection;

    using Rml::StyleSheet;
    using Rml::StyleSheetContainer;
    using Rml::StyleSheetSpecification;
    using Rml::Spritesheet;
    using Rml::Sprite;

    using Rml::Transform;
    using Rml::TransformPtr;
    using Rml::Tween;

    using Rml::Decorator;
    using Rml::DecoratorInstancer;
    using Rml::DecoratorInstancerInterface;
    using Rml::Filter;
    using Rml::FilterInstancer;
}

// ---------------------------------------------------------------------------
// Elements, documents, contexts, events
// ---------------------------------------------------------------------------
export namespace Rml {
    using Rml::Context;
    using Rml::ContextInstancer;

    using Rml::Element;
    using Rml::ElementPtr;
    using Rml::ElementDocument;
    using Rml::ElementList;
    using Rml::ElementAttributes;
    using Rml::ElementInstancer;
    using Rml::ElementScroll;
    using Rml::ElementUtilities;

    using Rml::ModalFlag;
    using Rml::FocusFlag;
    using Rml::ScrollBehavior;
    using Rml::ScrollAlignment;
    using Rml::ScrollIntoViewOptions;

    using Rml::Event;
    using Rml::EventId;
    using Rml::EventListener;
    using Rml::EventListenerInstancer;
    using Rml::EventPhase;
    using Rml::DefaultActionPhase;
}

// ---------------------------------------------------------------------------
// Data binding
// ---------------------------------------------------------------------------
export namespace Rml {
    using Rml::DataModelHandle;
    using Rml::DataModelConstructor;
    using Rml::DataVariable;
    using Rml::DataAddress;
    using Rml::DataTypeRegister;
    using Rml::DataGetFunc;
    using Rml::DataSetFunc;
    using Rml::DataEventFunc;
    using Rml::StructHandle;
}

// ---------------------------------------------------------------------------
// Interfaces and plugins
// ---------------------------------------------------------------------------
export namespace Rml {
    using Rml::RenderInterface;
    using Rml::SystemInterface;
    using Rml::FileInterface;
    using Rml::FontEngineInterface;
    using Rml::FontMetrics;
    using Rml::FontGlyph;
    using Rml::FontGlyphMap;
    using Rml::TextInputHandler;
    using Rml::TextInputContext;
    using Rml::Plugin;
}

// ---------------------------------------------------------------------------
// XML, URL
// ---------------------------------------------------------------------------
export namespace Rml {
    using Rml::URL;
    using Rml::XMLParser;
    using Rml::XMLAttributes;
    using Rml::XMLNodeHandler;
    using Rml::BaseXMLParser;
}

export namespace Rml::Input {
    using Input::KeyIdentifier;
    using Input::KeyModifier;
}

export namespace Rml {
    using Rml::Log;
}


export namespace Rml {
    template <typename T>
    void RegisterStruct(DataModelConstructor &constructor) {
        if (StructHandle<T> handle = constructor.RegisterStruct<T>()) {
            static T dummy {};
            rfl::to_view(dummy).apply([&](const auto &field) {
                const std::ptrdiff_t offset =
                    reinterpret_cast<const void*>(field.value()) -
                    reinterpret_cast<const void*>(&dummy);

                std::remove_pointer_t<std::remove_cvref_t<decltype(field.value())>> T::*member;
                static_assert(sizeof(member) == sizeof(offset));
                std::memcpy(&member, &offset, sizeof(member));

                handle.RegisterMember(String(field.name()), member);
            });
        }
    }

    void RegisterLocalizer(
        DataModelConstructor &constructor,
        const std::function<std::string_view(std::string_view table, std::string_view lockey)> &localize
    ) {
        constructor.RegisterTransformFunc(
            "t",
            [localize](const VariantList &args) -> Variant {
                // lockey[0] | t(table[1],lang[2]). lang is for dirty state only.
                constexpr std::size_t lockeyArg = 0;
                constexpr std::size_t tableArg = 1;
                if (args.size() != 3) return {};

                const std::string lockey = args[lockeyArg].Get<std::string>();
                const std::string table = args[tableArg].Get<std::string>();

                const std::string_view localized = localize(table, lockey);
                if (localized.empty()) {
                    return args[lockeyArg];
                }

                return Variant(CreateString(localized.data()));
            }
        );
    }

    void RegisterBase(
        Context &ctx,
        const std::function<std::string_view(std::string_view table, std::string_view lockey)> &localize
    ) {
        DataModelConstructor constructor = ctx.CreateDataModel("__register");

        RegisterLocalizer(constructor, localize);
    }

    template <typename T>
    String ModelName() {
        String name = rfl::type_name_t<T>().str();
        if (const std::size_t pos = name.rfind("::"); pos != String::npos)
            name.erase(0, pos + 2);
        return name;
    }

    template <typename TModel, typename TGlobalModel=void>
    DataModelHandle BindModel(Context &ctx, TModel &model, TGlobalModel *globalModel=nullptr) {
        DataModelConstructor constructor = ctx.CreateDataModel(ModelName<TModel>());
        rfl::to_view(model).apply([&](const auto &field) {
            constructor.Bind(String(field.name()), field.value());
        });

        if constexpr (!std::is_void_v<TGlobalModel>) {
            if (globalModel != nullptr) {
                rfl::to_view(*globalModel).apply([&](const auto &field) {
                    constructor.Bind(String(field.name()), field.value());
                });
            }
        }

        return constructor.GetModelHandle();
    }

    template <bool isGlobal=false>
    using BindedDependents = ee_utils::optional<std::vector<DataModelHandle>, isGlobal>;

    template <typename TModel, typename TGlobal=void>
    struct Binded {
        static constexpr bool isGlobal = std::is_void_v<TGlobal>;

        DataModelHandle model;
        TModel data;
        NO_UNIQUE_ADDRESS BindedDependents<isGlobal> dependents;

        // ReSharper disable once CppNonExplicitConvertingConstructor
        Binded(Context &ctx) requires (isGlobal) {
            model = BindModel(ctx, data);
        }

        Binded(Context &ctx, TGlobal *global) requires (!isGlobal) {
            model = BindModel(ctx, data, &global->data);
            global->dependents.push_back(model);
        }

        void DirtyGlobal(const String &field) requires (isGlobal) {
            model.DirtyVariable(field);
            for (DataModelHandle &dependent : dependents) {
                dependent.DirtyVariable(field);
            }
        }

        void DirtyAllGlobal() requires (isGlobal) {
            model.DirtyAllVariables();
            for (DataModelHandle &dependent : dependents) {
                dependent.DirtyAllVariables();
            }
        }

        Binded(const Binded&) = delete;
        Binded &operator=(const Binded&) = delete;
    };
}
