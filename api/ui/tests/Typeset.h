/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/Manager.h>
#include <api/log/Logger.h>
#include <api/render/realtime/Handle.h>
#include <api/ui/paint/GlyphPages.h>
#include <api/ui/paint/TextRenderer.h>

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
        [](const unsigned char*, std::uint32_t) { return v3d::render::realtime::TextureHandle(1); },
        [](const v3d::render::realtime::TextureHandle&, const glm::uvec2&, const unsigned char*, const glm::uvec2&) {},
        [](const v3d::render::realtime::TextureHandle&) {});
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

};  // namespace v3d::ui::typeset
