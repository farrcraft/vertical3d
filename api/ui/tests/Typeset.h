/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/Manager.h>
#include <api/log/Logger.h>
#include <api/render/realtime/resource/Handle.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/ui/paint/GlyphPages.h>
#include <api/ui/paint/TextRenderer.h>
#include <api/ui/paint/Type.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::ui::typeset {

/*
 * Text the ui's cases can predict: a monospaced face at a size where every character is ten
 * pixels wide, and a record of the lines drawn. The face is a fixture beside the executable, in
 * fonts/, which the suite copies there.
 */

/**
 * How wide every character is.
 **/
const float characterWidth = 10.0F;

/**
 * The size that makes it so: Noto Sans Mono advances six tenths of an em.
 **/
const float monoSize = characterWidth / 0.6F;

/**
 * The monospaced face, from the executable's directory.
 **/
const char* const monoFace = "fonts/NotoSansMono-Regular.ttf";

/**
 * A face that sets Hebrew, read right to left, and no Latin.
 **/
const char* const hebrewFace = "fonts/NotoSansHebrew-Regular.ttf";

/**
 * The texture every glyph page is given, which no other quad in a case samples, so what a
 * component drew can be told from its text.
 **/
const std::uint32_t pageTexture = 0xF0F0;

/**
 * A line the ui drew.
 **/
struct Written final {
    std::string text;
    glm::vec2 pen;
    glm::vec4 colour;
    v3d::ui::paint::Weight weight;
    float size;
};

/**
 * @return a text renderer over faces at a size, whose pages go to no device
 **/
inline boost::shared_ptr<v3d::ui::paint::TextRenderer> renderer(const v3d::ui::paint::TextRenderer::Faces& faces, float size) {
    const boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    const boost::shared_ptr<v3d::asset::Manager> assets = boost::make_shared<v3d::asset::Manager>(".", logger);
    v3d::ui::paint::GlyphPages pages(
        [](const unsigned char*, std::uint32_t, std::uint32_t) { return v3d::render::realtime::resource::TextureHandle(pageTexture); },
        [](const v3d::render::realtime::resource::TextureHandle&, const glm::uvec2&, const unsigned char*, const glm::uvec2&, std::uint32_t) {},
        [](const v3d::render::realtime::resource::TextureHandle&) {});
    return boost::make_shared<v3d::ui::paint::TextRenderer>(assets, logger, std::move(pages), size, faces);
}

/**
 * @return the faces of a renderer that sets one face, with no semibold and no fallback
 **/
inline v3d::ui::paint::TextRenderer::Faces only(const std::string& face) {
    v3d::ui::paint::TextRenderer::Faces faces;
    faces.regular = face;
    faces.semiBold.clear();
    faces.fallback.clear();
    faces.emoji.clear();
    return faces;
}

/**
 * @return a renderer of the monospaced face at ten pixels a character, which records each line
 *         it draws when given somewhere to
 **/
inline boost::shared_ptr<v3d::ui::paint::TextRenderer> mono(std::vector<Written>* written = nullptr) {
    const boost::shared_ptr<v3d::ui::paint::TextRenderer> text = renderer(only(monoFace), monoSize);
    if (written != nullptr) {
        text->trace([written](const v3d::ui::paint::TextRenderer::Traced& line) {
            written->push_back(Written{ line.text, line.pen, line.colour, line.weight, line.size });
        });
    }
    return text;
}

/**
 * @return the monospaced face at ten pixels a character as a type, recording each line drawn
 *         through it when given somewhere to
 **/
inline v3d::ui::paint::Type body(std::vector<Written>* written = nullptr) {
    return v3d::ui::paint::Type(mono(written), 0.0F);
}

/**
 * @return whether a batch draws glyphs from a page
 **/
inline bool text(const v3d::render::realtime::canvas::Canvas::Batch& batch) {
    return batch.texture == v3d::render::realtime::resource::TextureHandle(pageTexture);
}

/**
 * @return the batches of a canvas that are not text
 **/
inline std::vector<v3d::render::realtime::canvas::Canvas::Batch> shapes(const v3d::render::realtime::canvas::Canvas& canvas) {
    std::vector<v3d::render::realtime::canvas::Canvas::Batch> found;
    for (const v3d::render::realtime::canvas::Canvas::Batch& batch : canvas.batches()) {
        if (!text(batch)) {
            found.push_back(batch);
        }
    }
    return found;
}

/**
 * @return the vertices of a canvas that are not text, in the order they were added
 **/
inline std::vector<v3d::render::realtime::canvas::Canvas::Vertex> shapeVertices(const v3d::render::realtime::canvas::Canvas& canvas) {
    std::vector<bool> used(canvas.vertices().size(), false);
    for (const v3d::render::realtime::canvas::Canvas::Batch& batch : shapes(canvas)) {
        for (std::uint32_t index = batch.firstIndex; index < batch.firstIndex + batch.indices; index++) {
            used[canvas.indices()[index]] = true;
        }
    }
    std::vector<v3d::render::realtime::canvas::Canvas::Vertex> found;
    for (std::size_t vertex = 0; vertex < used.size(); vertex++) {
        if (used[vertex]) {
            found.push_back(canvas.vertices()[vertex]);
        }
    }
    return found;
}

};  // namespace v3d::ui::typeset
