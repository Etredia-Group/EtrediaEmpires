module;

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

    using Rml::SystemInterface;
    using Rml::SetSystemInterface;
}

export namespace Rml::Input {
    using Input::KeyIdentifier;
    using Input::KeyModifier;
}