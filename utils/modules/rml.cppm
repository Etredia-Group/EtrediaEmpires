module;

#include <rfl/to_view.hpp>
#include <rfl/type_name_t.hpp>
#include <RmlUi/Core.h>

export module rml;

export namespace Rml {
    using Rml::Initialise;
    using Rml::Shutdown;
    using Rml::Context;
    using Rml::CreateContext;
    using Rml::LoadFontFace;
    using Rml::ElementDocument;

    using Rml::RenderInterface;
    using Rml::SetRenderInterface;
    using Rml::Vertex;
    using Rml::Span;
    using Rml::CompiledGeometryHandle;
    using Rml::Character;
    using Rml::Vector2f;
    using Rml::Vector2i;
    using Rml::Matrix4f;
    using Rml::ColourbPremultiplied;
    using Rml::String;
    using Rml::Rectanglei;
    using Rml::TextureHandle;
    using Rml::PropertyId;
    using Rml::Property;
    using Rml::Unit;

    using Rml::SystemInterface;
    using Rml::SetSystemInterface;

    using Rml::Element;
    using Rml::ElementPtr;
    using Rml::ModalFlag;
    using Rml::FocusFlag;
    using Rml::DataModelHandle;
    using Rml::DataModelConstructor;
}

export namespace Rml::Input {
    using Input::KeyIdentifier;
    using Input::KeyModifier;
}


export namespace Rml {
    template <typename T>
    String ModelName() {
        String name = rfl::type_name_t<T>().str();
        if (const std::size_t pos = name.rfind("::"); pos != String::npos)
            name.erase(0, pos + 2);
        return name;
    }

    template <typename T>
    DataModelHandle BindModel(Context &ctx, T &model) {
        DataModelConstructor constructor = ctx.CreateDataModel(ModelName<T>());
        rfl::to_view(model).apply([&](const auto &field) {
            constructor.Bind(String(field.name()), field.value());
        });

        return constructor.GetModelHandle();
    }

    template <typename T>
    struct Binded {
        DataModelHandle model;
        T data;

        // ReSharper disable once CppNonExplicitConvertingConstructor
        Binded(Context &ctx) {
            model = BindModel(ctx, data);
        }
    };
}