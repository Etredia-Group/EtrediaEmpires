module;

#include <RmlUi/Core.h>

export module rml;

export namespace Rml {
    using Rml::SetRenderInterface;
    using Rml::Initialise;
    using Rml::Shutdown;
    using Rml::Context;
    using Rml::CreateContext;
    using Rml::LoadFontFace;
    using Rml::ElementDocument;

    using Rml::RenderInterface;
    using Rml::Vertex;
    using Rml::Span;
    using Rml::CompiledGeometryHandle;
    using Rml::Vector2f;
    using Rml::Vector2i;
    using Rml::ColourbPremultiplied;
    using Rml::String;
    using Rml::Rectanglei;
    using Rml::TextureHandle;
}