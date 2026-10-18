/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Canvas.h"

#include <api/font/TextBuffer.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace v3d::render::realtime::canvas {

namespace {

// two triangles
constexpr uint32_t QUAD_INDICES = 6;
// the sweep of a whole circle, in radians
constexpr float FULL_TURN = 6.283185307179586F;

};  // namespace

/**
 **/
Canvas::Batch::Batch() noexcept :
text(false),
clipped(false),
clip(0.0F),
firstIndex(0),
indices(0) {
}

/**
 **/
Canvas::Canvas() :
    width_(0),
    height_(0),
    space_(0.0F, 0.0F),
    fit_(Fit::Stretch) {
}

/**
 **/
void Canvas::clear() {
    vertices_.clear();
    indices_.clear();
    batches_.clear();
    transforms_.reset();
    clips_.reset();
}

/**
 **/
void Canvas::resize(const glm::uvec2& size) {
    width_ = size.x;
    height_ = size.y;
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
void Canvas::space(const glm::vec2& size, Fit fit) {
    space_ = size.x > 0.0F && size.y > 0.0F ? size : glm::vec2(0.0F, 0.0F);
    fit_ = fit;
}

/**
 **/
glm::vec2 Canvas::space() const noexcept {
    return space_;
}

/**
 **/
glm::vec4 Canvas::viewport() const noexcept {
    const float width = static_cast<float>(width_);
    const float height = static_cast<float>(height_);
    if (space_.x <= 0.0F || fit_ == Fit::Stretch) {
        return glm::vec4(0.0F, 0.0F, width, height);
    }
    const float scale = std::min(width / space_.x, height / space_.y);
    const glm::vec2 size = space_ * scale;
    return glm::vec4((width - size.x) * 0.5F, (height - size.y) * 0.5F, size.x, size.y);
}

/**
 **/
glm::vec2 Canvas::toSpace(const glm::vec2& pixel) const noexcept {
    const glm::vec4 area = viewport();
    if (space_.x <= 0.0F || area.z <= 0.0F || area.w <= 0.0F) {
        return pixel;
    }
    return (pixel - glm::vec2(area.x, area.y)) * space_ / glm::vec2(area.z, area.w);
}

/**
 **/
glm::vec2 Canvas::toPixels(const glm::vec2& point) const noexcept {
    if (space_.x <= 0.0F) {
        return point;
    }
    const glm::vec4 area = viewport();
    return glm::vec2(area.x, area.y) + point * glm::vec2(area.z, area.w) / space_;
}

/**
 **/
glm::mat4 Canvas::projection() const {
    // pixels to clip space, origin top left: scaled to the two units clip space spans and
    // shifted back by one. y is not flipped - vulkan's y already points down
    const float width = width_ > 0 ? static_cast<float>(width_) : 1.0F;
    const float height = height_ > 0 ? static_cast<float>(height_) : 1.0F;

    glm::mat4 projection(1.0F);
    const glm::vec4 area = viewport();
    if (space_.x <= 0.0F || area.z <= 0.0F || area.w <= 0.0F) {
        projection[0][0] = 2.0F / width;
        projection[1][1] = 2.0F / height;
        projection[3][0] = -1.0F;
        projection[3][1] = -1.0F;
        return projection;
    }
    // the space into its viewport's pixels, then those pixels to clip space as above
    projection[0][0] = 2.0F * area.z / (space_.x * width);
    projection[1][1] = 2.0F * area.w / (space_.y * height);
    projection[3][0] = (2.0F * area.x / width) - 1.0F;
    projection[3][1] = (2.0F * area.y / height) - 1.0F;
    return projection;
}

/**
 **/
void Canvas::push() {
    transforms_.push();
}

/**
 **/
void Canvas::pop() {
    transforms_.pop();
}

/**
 **/
void Canvas::translate(const glm::vec2& offset) {
    glm::mat4& current = transforms_.top();
    current[3][0] += offset.x;
    current[3][1] += offset.y;
}

/**
 **/
void Canvas::scale(const glm::vec2& factor) {
    glm::mat4& current = transforms_.top();
    current[0][0] *= factor.x;
    current[0][1] *= factor.x;
    current[1][0] *= factor.y;
    current[1][1] *= factor.y;
}

/**
 **/
void Canvas::clip(const glm::vec2& min, const glm::vec2& max) {
    const glm::mat4& transform = transforms_.top();
    const glm::vec2 first = toPixels(glm::vec2(
        (transform[0][0] * min.x) + (transform[1][0] * min.y) + transform[3][0],
        (transform[0][1] * min.x) + (transform[1][1] * min.y) + transform[3][1]));
    const glm::vec2 second = toPixels(glm::vec2(
        (transform[0][0] * max.x) + (transform[1][0] * max.y) + transform[3][0],
        (transform[0][1] * max.x) + (transform[1][1] * max.y) + transform[3][1]));

    // a negative scale swaps the corners, so the stack orders them after the transform
    clips_.push(first, second);
}

/**
 **/
void Canvas::unclip() {
    clips_.pop();
}

/**
 **/
void Canvas::open(const TextureHandle& texture, bool text) {
    const bool clipped = clips_.clipped();
    const glm::vec4 clip = clips_.top();

    if (!batches_.empty() && batches_.back().texture == texture && batches_.back().text == text &&
        batches_.back().clipped == clipped && batches_.back().clip == clip) {
        return;
    }
    Batch batch;
    batch.texture = texture;
    batch.text = text;
    batch.clipped = clipped;
    batch.clip = clip;
    batch.firstIndex = static_cast<uint32_t>(indices_.size());
    batch.indices = 0;
    batches_.push_back(batch);
}

/**
 **/
void Canvas::vertex(const Vertex& untransformed) {
    const glm::mat4& transform = transforms_.top();
    const glm::vec2& position = untransformed.position;

    Vertex added;
    added.position.x = (transform[0][0] * position.x) + (transform[1][0] * position.y) + transform[3][0];
    added.position.y = (transform[0][1] * position.x) + (transform[1][1] * position.y) + transform[3][1];
    added.uv = untransformed.uv;
    added.colour = untransformed.colour;
    vertices_.push_back(added);
}

/**
 **/
void Canvas::quad(uint32_t first) {
    const uint32_t order[QUAD_INDICES] = {0, 1, 2, 2, 3, 0};
    for (std::size_t index = 0; index < QUAD_INDICES; index++) {
        indices_.push_back(first + order[index]);
    }
    batches_.back().indices += QUAD_INDICES;
}

/**
 **/
void Canvas::rect(const glm::vec2& min, const glm::vec2& max, const glm::vec4& colour) {
    // an unset handle means the renderer's white texture, so untextured quads all batch
    // together
    rect(min, max, glm::vec2(0.0F, 0.0F), glm::vec2(1.0F, 1.0F), colour, TextureHandle());
}

/**
 **/
void Canvas::rect(const glm::vec2& min, const glm::vec2& max, const glm::vec2& uv0, const glm::vec2& uv1,
    const glm::vec4& colour, const TextureHandle& texture) {
    open(texture);

    const uint32_t first = static_cast<uint32_t>(vertices_.size());
    vertex({.position = glm::vec2(min.x, min.y), .uv = glm::vec2(uv0.x, uv0.y), .colour = colour});
    vertex({.position = glm::vec2(min.x, max.y), .uv = glm::vec2(uv0.x, uv1.y), .colour = colour});
    vertex({.position = glm::vec2(max.x, max.y), .uv = glm::vec2(uv1.x, uv1.y), .colour = colour});
    vertex({.position = glm::vec2(max.x, min.y), .uv = glm::vec2(uv1.x, uv0.y), .colour = colour});
    quad(first);
}

/**
 **/
void Canvas::circle(const glm::vec2& centre, float radius, unsigned int sides, const glm::vec4& colour) {
    arc({.centre = centre, .radius = radius, .sides = sides, .start = 0.0F, .sweep = FULL_TURN}, colour);
}

void Canvas::ring(const Ring& shape, const glm::vec4& colour) {
    const glm::vec2& centre = shape.centre;
    const float outer = shape.outer;
    const float inner = shape.inner;
    const unsigned int sides = shape.sides;
    const float start = shape.start;
    const float sweep = shape.sweep;
    if (sides < 1 || outer <= 0.0F) {
        return;
    }
    if (inner <= 0.0F) {
        arc({.centre = centre, .radius = outer, .sides = sides, .start = start, .sweep = sweep}, colour);
        return;
    }
    open(TextureHandle());

    const uint32_t first = static_cast<uint32_t>(vertices_.size());
    const float step = sweep / static_cast<float>(sides);
    for (unsigned int side = 0; side <= sides; side++) {
        const float angle = start + (step * static_cast<float>(side));
        const glm::vec2 direction(std::cos(angle), std::sin(angle));
        vertex({.position = centre + direction * outer, .uv = glm::vec2(0.5F, 0.5F), .colour = colour});
        vertex({.position = centre + direction * inner, .uv = glm::vec2(0.5F, 0.5F), .colour = colour});
    }

    // two triangles per segment, over the pair of vertices at each end of it
    for (unsigned int side = 0; side < sides; side++) {
        const uint32_t edge = first + (side * 2);
        indices_.push_back(edge);
        indices_.push_back(edge + 2);
        indices_.push_back(edge + 1);
        indices_.push_back(edge + 1);
        indices_.push_back(edge + 2);
        indices_.push_back(edge + 3);
    }
    batches_.back().indices += sides * QUAD_INDICES;
}

void Canvas::arc(const Arc& shape, const glm::vec4& colour) {
    const glm::vec2& centre = shape.centre;
    const float radius = shape.radius;
    const unsigned int sides = shape.sides;
    const float start = shape.start;
    const float sweep = shape.sweep;
    if (sides < 3) {
        return;
    }
    open(TextureHandle());

    const uint32_t centreIndex = static_cast<uint32_t>(vertices_.size());
    vertex({.position = centre, .uv = glm::vec2(0.5F, 0.5F), .colour = colour});

    const float step = sweep / static_cast<float>(sides);
    for (unsigned int side = 0; side <= sides; side++) {
        const float angle = start + (step * static_cast<float>(side));
        vertex({.position = centre + glm::vec2(std::cos(angle) * radius, std::sin(angle) * radius), .uv = glm::vec2(0.5F, 0.5F), .colour = colour});
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

    open(atlas, true);

    const uint32_t first = static_cast<uint32_t>(vertices_.size());
    for (std::size_t index = 0; index < positions.size(); index++) {
        // glyphs are laid out in three dimensions; a 2D canvas has no use for the third
        vertex({.position = glm::vec2(positions[index].x, positions[index].y), .uv = uvs[index], .colour = colours[index]});
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

};  // namespace v3d::render::realtime::canvas
