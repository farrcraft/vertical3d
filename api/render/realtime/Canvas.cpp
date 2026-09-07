/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Canvas.h"

#include <cmath>
#include <cstddef>
#include <vector>

#include "../../font/TextBuffer.h"

namespace v3d::render::realtime {

/**
 **/
Canvas::Batch::Batch() noexcept :
firstIndex(0),
indices(0) {
}

/**
 **/
Canvas::Canvas() :
    width_(0),
    height_(0) {
    transforms_.push_back(glm::mat4(1.0f));
}

/**
 **/
void Canvas::clear() {
    vertices_.clear();
    indices_.clear();
    batches_.clear();
    transforms_.clear();
    transforms_.push_back(glm::mat4(1.0f));
}

/**
 **/
void Canvas::resize(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;
}

/**
 **/
uint32_t Canvas::width() const noexcept {
    return width_;
}

/**
 **/
uint32_t Canvas::height() const noexcept {
    return height_;
}

/**
 **/
glm::mat4 Canvas::projection() const {
    // pixels to clip space, origin top left: scaled to the two units clip space spans and
    // shifted back by one. y is not flipped - vulkan's y already points down
    const float width = width_ > 0 ? static_cast<float>(width_) : 1.0f;
    const float height = height_ > 0 ? static_cast<float>(height_) : 1.0f;

    glm::mat4 projection(1.0f);
    projection[0][0] = 2.0f / width;
    projection[1][1] = 2.0f / height;
    projection[3][0] = -1.0f;
    projection[3][1] = -1.0f;
    return projection;
}

/**
 **/
void Canvas::push() {
    transforms_.push_back(transforms_.back());
}

/**
 **/
void Canvas::pop() {
    // the identity at the bottom of the stack is the canvas's own and not a caller's to pop
    if (transforms_.size() > 1) {
        transforms_.pop_back();
    }
}

/**
 **/
void Canvas::translate(const glm::vec2& offset) {
    glm::mat4& current = transforms_.back();
    current[3][0] += offset.x;
    current[3][1] += offset.y;
}

/**
 **/
void Canvas::open(const TextureHandle& texture) {
    if (!batches_.empty() && batches_.back().texture == texture) {
        return;
    }
    Batch batch;
    batch.texture = texture;
    batch.firstIndex = static_cast<uint32_t>(indices_.size());
    batch.indices = 0;
    batches_.push_back(batch);
}

/**
 **/
void Canvas::vertex(const glm::vec2& position, const glm::vec2& uv, const glm::vec4& colour) {
    const glm::mat4& transform = transforms_.back();

    Vertex added;
    added.position.x = transform[0][0] * position.x + transform[1][0] * position.y + transform[3][0];
    added.position.y = transform[0][1] * position.x + transform[1][1] * position.y + transform[3][1];
    added.uv = uv;
    added.colour = colour;
    vertices_.push_back(added);
}

/**
 **/
void Canvas::quad(uint32_t first) {
    const uint32_t order[6] = {0, 1, 2, 2, 3, 0};
    for (std::size_t index = 0; index < 6; index++) {
        indices_.push_back(first + order[index]);
    }
    batches_.back().indices += 6;
}

/**
 **/
void Canvas::rect(const glm::vec2& min, const glm::vec2& max, const glm::vec4& colour) {
    // an unset handle means the renderer's white texture, so untextured quads all batch
    // together
    rect(min, max, glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f), colour, TextureHandle());
}

/**
 **/
void Canvas::rect(const glm::vec2& min, const glm::vec2& max, const glm::vec2& uv0, const glm::vec2& uv1,
    const glm::vec4& colour, const TextureHandle& texture) {
    open(texture);

    const uint32_t first = static_cast<uint32_t>(vertices_.size());
    vertex(glm::vec2(min.x, min.y), glm::vec2(uv0.x, uv0.y), colour);
    vertex(glm::vec2(min.x, max.y), glm::vec2(uv0.x, uv1.y), colour);
    vertex(glm::vec2(max.x, max.y), glm::vec2(uv1.x, uv1.y), colour);
    vertex(glm::vec2(max.x, min.y), glm::vec2(uv1.x, uv0.y), colour);
    quad(first);
}

/**
 **/
void Canvas::circle(const glm::vec2& centre, float radius, unsigned int sides, const glm::vec4& colour) {
    arc(centre, radius, sides, 0.0f, 6.283185307179586f, colour);
}

void Canvas::arc(const glm::vec2& centre, float radius, unsigned int sides, float start, float sweep,
    const glm::vec4& colour) {
    if (sides < 3) {
        return;
    }
    open(TextureHandle());

    const uint32_t centreIndex = static_cast<uint32_t>(vertices_.size());
    vertex(centre, glm::vec2(0.5f, 0.5f), colour);

    const float step = sweep / static_cast<float>(sides);
    for (unsigned int side = 0; side <= sides; side++) {
        const float angle = start + step * static_cast<float>(side);
        vertex(centre + glm::vec2(std::cos(angle) * radius, std::sin(angle) * radius), glm::vec2(0.5f, 0.5f), colour);
    }

    // a fan written out as triangles, so it batches with the quads around it. A full turn
    // repeats its first rim vertex last, so the wrap needs no special case
    for (unsigned int side = 0; side < sides; side++) {
        indices_.push_back(centreIndex);
        indices_.push_back(centreIndex + 1 + side);
        indices_.push_back(centreIndex + 2 + side);
    }
    batches_.back().indices += sides * 3;
}

/**
 **/
void Canvas::text(const v3d::font::TextBuffer& text, const TextureHandle& atlas) {
    const std::vector<glm::vec3>& positions = text.vertices();
    const std::vector<glm::vec2>& uvs = text.uvs();
    const std::vector<glm::vec4>& colours = text.colors();
    const std::vector<unsigned int>& order = text.indices();

    if (positions.empty() || order.empty()) {
        return;
    }
    // the three streams are filled a vertex at a time, so a mismatch means a partly built
    // buffer - draw nothing rather than read off the end of the shorter one
    if (uvs.size() < positions.size() || colours.size() < positions.size()) {
        return;
    }

    open(atlas);

    const uint32_t first = static_cast<uint32_t>(vertices_.size());
    for (std::size_t index = 0; index < positions.size(); index++) {
        // glyphs are laid out in three dimensions; a 2D canvas has no use for the third
        vertex(glm::vec2(positions[index].x, positions[index].y), uvs[index], colours[index]);
    }

    for (std::size_t index = 0; index < order.size(); index++) {
        indices_.push_back(first + order[index]);
    }
    batches_.back().indices += static_cast<uint32_t>(order.size());
}

/**
 **/
const std::vector<Canvas::Vertex>& Canvas::vertices() const noexcept {
    return vertices_;
}

/**
 **/
const std::vector<uint32_t>& Canvas::indices() const noexcept {
    return indices_;
}

/**
 **/
const std::vector<Canvas::Batch>& Canvas::batches() const noexcept {
    return batches_;
}

/**
 **/
bool Canvas::empty() const noexcept {
    return indices_.empty();
}

};  // namespace v3d::render::realtime
