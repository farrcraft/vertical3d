/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "RIBHandler.h"

#include <api/render/offline/Sampling.h>
#include <api/render/offline/rib/Parameters.h>
#include <api/render/offline/rib/Requests.h>
#include <api/type/Checked.h>

#include "Polygon.h"
#include "RenderContext.h"
#include "Renderer.h"
#include "Vertex.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>

namespace v3d::moya {

namespace {

/**
 * A number from a scene as a count of pixels or micropolygons: at least one and at most
 * largestLimit. Anything else is refused.
 **/
std::optional<uint32_t> count(float value) {
    return v3d::type::toCount(value, {.minimum = 1, .maximum = RIBHandler::largestLimit});
}

/**
 * A picture side as the context takes it. A side too large for an int is held just above
 * largestResolution, which the context refuses.
 **/
int side(unsigned int value) {
    return static_cast<int>(std::min(value, v3d::render::offline::largestResolution + 1));
}

/**
 * The deepest a scene may ask trace() to go. A shader that traces twice at every hit doubles
 * the rays at every level, and every level holds a shader on the stack.
 **/
const float DEEPEST_TRACE = 16.0F;

typedef v3d::render::offline::rib::ParameterList ParameterList;

/**
 * One polygon from the corners that indices names in the position array, with whatever
 * colour, shading normal and texture coordinates the scene gave each corner. An index past
 * the position array is skipped. addPolygon() gives a vertex left without a colour the
 * current colour, and a vertex left without a normal the plane the polygon lies in.
 **/
boost::shared_ptr<Polygon> build(const std::vector<glm::vec3> & points,
    const std::vector<glm::vec3> & colors, const std::vector<glm::vec3> & normals,
    const std::vector<float> & st, const std::vector<unsigned int> & indices) {
    boost::shared_ptr<Polygon> polygon = boost::make_shared<Polygon>();
    for (const unsigned int index : indices) {
        if (index >= points.size()) {
            continue;
        }
        Vertex vertex;
        vertex.point(points[index]);
        if (index < colors.size()) {
            vertex.color(colors[index]);
        }
        if (index < normals.size()) {
            vertex.normal(normals[index]);
        }
        const std::size_t pair = 2 * static_cast<std::size_t>(index);
        if (pair + 2 <= st.size()) {
            vertex.st(glm::vec2(st[pair], st[pair + 1]));
        }
        polygon->addVertex(vertex);
    }
    return polygon;
}

};  // namespace

RIBHandler::RIBHandler(Renderer * renderer) : renderer_(renderer) {
    renderer_->createRenderContext("");
}

RIBHandler::~RIBHandler() {
    renderer_->destroyActiveRenderContext();
}

RenderContext & RIBHandler::context() {
    return renderer_->activeRenderContext();
}

void RIBHandler::option(const std::string & name, const ParameterList & parameters) {
    if (name == "searchpath") {
        // RI writes it as Option "searchpath" "shader" ["./shaders:&"]. The renderer looks
        // up shaders and textures only, so every other path is ignored
        if (parameters.has("shader")) {
            context().searchpath(parameters.string("shader"));
        }
        if (parameters.has("texture")) {
            context().textures().searchpath(parameters.string("texture"));
        }
        return;
    }
    if (name == "trace" && parameters.has("maxdepth")) {
        const float depth = parameters.number("maxdepth", 0.0F);
        const bool usable = std::isfinite(depth) && depth >= 0.0F;
        if (!usable) {
            context().logger()->get()->warn("a trace depth of {} is not a depth, and is not used", depth);
            return;
        }
        if (depth > DEEPEST_TRACE) {
            context().logger()->get()->warn("a trace depth of {} is deeper than {}, and {} is used", depth, DEEPEST_TRACE, DEEPEST_TRACE);
        }
        context().traced().traceDepth(static_cast<unsigned int>(std::min(depth, DEEPEST_TRACE)));
        return;
    }
    if (name != "limits") {
        return;
    }
    const std::vector<float> & bucket = parameters.floats("bucketsize");
    if (bucket.size() >= 2) {
        const std::optional<uint32_t> across = count(bucket[0]);
        const std::optional<uint32_t> down = count(bucket[1]);
        if (across && down) {
            context().bucketSize(*across, *down);
        } else {
            context().logger()->get()->warn("a bucket size of {} by {} is not a size, and is not used", bucket[0], bucket[1]);
        }
    }
    if (parameters.has("gridsize")) {
        const float grid = parameters.number("gridsize", 256.0F);
        const std::optional<uint32_t> size = count(grid);
        if (size) {
            context().gridSize(*size);
        } else {
            context().logger()->get()->warn("a grid size of {} is not a size, and is not used", grid);
        }
    }
}

void RIBHandler::hider(const std::string & name, const ParameterList & parameters) {
    (void)parameters;
    context().hider(name);
}

void RIBHandler::surface(const std::string & name, const ParameterList & parameters) {
    context().surface(name, parameters);
}

void RIBHandler::lightSource(const v3d::render::offline::rib::LightSource & request, const ParameterList & parameters) {
    context().lightSource(request, parameters);
}

void RIBHandler::displacement(const std::string & name, const ParameterList & parameters) {
    context().displacement(name, parameters);
}

void RIBHandler::attribute(const std::string & name, const ParameterList & parameters) {
    context().attribute(name, parameters);
}

void RIBHandler::areaLightSource(const v3d::render::offline::rib::LightSource & request,
    const ParameterList & parameters) {
    context().areaLightSource(request, parameters);
}

void RIBHandler::illuminate(const std::string & handle, bool on) {
    context().illuminate(handle, on);
}

void RIBHandler::imager(const std::string & name, const ParameterList & parameters) {
    context().imager(name, parameters);
}

void RIBHandler::format(const v3d::render::offline::rib::Format & request) {
    unsigned int width = request.width;
    unsigned int height = request.height;
    if (width_ > 0 && height_ > 0) {
        width = width_;
        height = height_;
    }
    context().imageResolution(glm::ivec2(side(width), side(height)), request.pixelAspect);
}

void RIBHandler::frameAspectRatio(float aspect) {
    context().frameAspectRatio(aspect);
}

void RIBHandler::screenWindow(const v3d::render::offline::rib::ScreenWindow & request) {
    context().screenWindow(request);
}

void RIBHandler::projection(const std::string & name, const ParameterList & parameters) {
    context().projection(name, parameters.number("fov", RenderContext::defaultFov));
}

void RIBHandler::clipping(const v3d::render::offline::rib::Clipping & request) {
    context().clipping(request);
}

void RIBHandler::depthOfField(const v3d::render::offline::rib::DepthOfField & request) {
    context().sampling().fstop = request.fstop;
    context().sampling().focalLength = request.focalLength;
    context().sampling().focalDistance = request.focalDistance;
}

void RIBHandler::shutter(const v3d::render::offline::rib::Shutter & request) {
    context().sampling().shutter = glm::vec2(request.open, request.close);
}

void RIBHandler::pixelSamples(const glm::uvec2 & samples) {
    context().sampling().samples = samples;
}

void RIBHandler::pixelFilter(v3d::render::offline::Filter filter, float xwidth, float ywidth) {
    context().sampling().filter = filter;
    context().sampling().width = glm::vec2(xwidth, ywidth);
}

void RIBHandler::pixelVariance(float variation) {
    context().sampling().variance = variation;
}

void RIBHandler::output(const std::string & name) {
    output_ = name;
    context().display({.name = name, .type = "file", .mode = "rgb"});
}

bool RIBHandler::resolution(int width, int height) {
    // the context reads a side of zero or less as its default, so such a side is refused here
    if (width < 1 || height < 1 || !context().imageResolution(glm::ivec2(width, height), 1.0F)) {
        return false;
    }
    width_ = static_cast<unsigned int>(width);
    height_ = static_cast<unsigned int>(height);
    return true;
}

void RIBHandler::display(const v3d::render::offline::rib::Display & request, const ParameterList & parameters) {
    (void)parameters;
    if (!output_.empty()) {
        context().display({.name = output_, .type = "file", .mode = request.mode});
        return;
    }
    context().display(request);
}

void RIBHandler::worldBegin() {
    context().prepareWorld();
}

void RIBHandler::worldEnd() {
    context().render();
}

void RIBHandler::attributeBegin() {
    context().attributeBegin();
}

void RIBHandler::attributeEnd() {
    context().attributeEnd();
}

void RIBHandler::transformBegin() {
    context().pushTransform();
}

void RIBHandler::transformEnd() {
    context().popTransform();
}

void RIBHandler::identity() {
    context().setIdentityTransform();
}

void RIBHandler::transform(const glm::mat4x4 & matrix) {
    context().setTransform(matrix);
}

void RIBHandler::concatTransform(const glm::mat4x4 & matrix) {
    context().concatTransform(matrix);
}

void RIBHandler::translate(float dx, float dy, float dz) {
    context().translate(dx, dy, dz);
}

void RIBHandler::rotate(float angle, const glm::vec3 & axis) {
    context().rotate(angle, axis);
}

void RIBHandler::scale(float sx, float sy, float sz) {
    context().scale(sx, sy, sz);
}

void RIBHandler::motionBegin(const std::vector<float> & times) {
    context().motionBegin(times);
}

void RIBHandler::motionEnd() {
    context().motionEnd();
}

void RIBHandler::color(const glm::vec3 & value) {
    context().color(value);
}

void RIBHandler::opacity(const glm::vec3 & value) {
    context().opacity(value);
}

void RIBHandler::shadingRate(float size) {
    context().shadingRate(size);
}

void RIBHandler::polygon(unsigned int vertices, const ParameterList & parameters) {
    const std::vector<glm::vec3> points = parameters.points("P");
    const std::vector<glm::vec3> colors = parameters.points("Cs");
    const std::vector<glm::vec3> normals = parameters.points("N");
    const std::vector<float> & st = parameters.floats("st");
    std::vector<unsigned int> indices;
    for (unsigned int i = 0; i < vertices && i < points.size(); i++) {
        indices.push_back(i);
    }
    if (indices.size() < 3) {
        return;
    }
    context().addPolygon(build(points, colors, normals, st, indices));
}

void RIBHandler::sphere(const v3d::render::offline::rib::Sphere & request, const ParameterList & parameters) {
    (void)parameters;
    if (!context().addSphere(request) && !spheres_) {
        spheres_ = true;
        context().logger()->get()->warn("the reyes hider does not dice spheres, so this scene renders without them");
    }
}

void RIBHandler::pointsPolygons(const v3d::render::offline::rib::PointsPolygons & request, const ParameterList & parameters) {
    const std::vector<unsigned int> & indices = request.indices;
    const std::vector<glm::vec3> points = parameters.points("P");
    const std::vector<glm::vec3> colors = parameters.points("Cs");
    const std::vector<glm::vec3> normals = parameters.points("N");
    const std::vector<float> & st = parameters.floats("st");
    std::size_t offset = 0;
    for (const unsigned int count : request.counts) {
        if (offset + count > indices.size()) {
            return;
        }
        if (count >= 3) {
            const std::vector<unsigned int> face(indices.begin() + static_cast<std::ptrdiff_t>(offset), indices.begin() + static_cast<std::ptrdiff_t>(offset + count));
            context().addPolygon(build(points, colors, normals, st, face));
        }
        offset += count;
    }
}

};  // namespace v3d::moya
