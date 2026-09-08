/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "RIBHandler.h"

#include <string>
#include <vector>

#include <boost/make_shared.hpp>

namespace v3d::moya {

namespace {

typedef v3d::render::offline::ParameterList ParameterList;

/**
 * One polygon from a run of the position array, with whatever colour and shading normal
 * the scene gave each corner. A vertex left without either takes the primitive's in
 * addPolygon() - the current colour, and the plane the polygon lies in.
 **/
boost::shared_ptr<Polygon> build(const std::vector<glm::vec3> & points,
    const std::vector<glm::vec3> & colors, const std::vector<glm::vec3> & normals,
    const std::vector<unsigned int> & indices) {
    boost::shared_ptr<Polygon> polygon = boost::make_shared<Polygon>();
    for (unsigned int index : indices) {
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
    if (name != "limits") {
        return;
    }
    const std::vector<float> & bucket = parameters.floats("bucketsize");
    if (bucket.size() >= 2) {
        context().bucketSize(static_cast<unsigned int>(bucket[0]), static_cast<unsigned int>(bucket[1]));
    }
    if (parameters.has("gridsize")) {
        context().gridSize(static_cast<unsigned int>(parameters.number("gridsize", 256.0f)));
    }
}

void RIBHandler::format(unsigned int width, unsigned int height, float pixelAspect) {
    context().imageResolution(static_cast<int>(width), static_cast<int>(height), pixelAspect);
}

void RIBHandler::frameAspectRatio(float aspect) {
    context().frameAspectRatio(aspect);
}

void RIBHandler::screenWindow(float left, float right, float bottom, float top) {
    context().screenWindow(left, right, bottom, top);
}

void RIBHandler::projection(const std::string & name, const ParameterList & parameters) {
    context().projection(name, parameters.number("fov", 90.0f));
}

void RIBHandler::clipping(float hither, float yon) {
    context().clipping(hither, yon);
}

void RIBHandler::output(const std::string & name) {
    output_ = name;
    context().display(name, "file", "rgb");
}

void RIBHandler::display(const std::string & name, const std::string & type, const std::string & mode,
    const ParameterList & parameters) {
    (void)parameters;
    if (!output_.empty()) {
        context().display(output_, "file", mode);
        return;
    }
    context().display(name, type, mode);
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

void RIBHandler::rotate(float angle, float dx, float dy, float dz) {
    context().rotate(angle, dx, dy, dz);
}

void RIBHandler::scale(float sx, float sy, float sz) {
    context().scale(sx, sy, sz);
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
    std::vector<unsigned int> indices;
    for (unsigned int i = 0; i < vertices && i < points.size(); i++) {
        indices.push_back(i);
    }
    if (indices.size() < 3) {
        return;
    }
    context().addPolygon(build(points, colors, normals, indices));
}

void RIBHandler::pointsPolygons(const std::vector<unsigned int> & counts, const std::vector<unsigned int> & indices,
    const ParameterList & parameters) {
    const std::vector<glm::vec3> points = parameters.points("P");
    const std::vector<glm::vec3> colors = parameters.points("Cs");
    const std::vector<glm::vec3> normals = parameters.points("N");
    std::size_t offset = 0;
    for (unsigned int count : counts) {
        if (offset + count > indices.size()) {
            return;
        }
        if (count >= 3) {
            const std::vector<unsigned int> face(indices.begin() + offset, indices.begin() + offset + count);
            context().addPolygon(build(points, colors, normals, face));
        }
        offset += count;
    }
}

};  // namespace v3d::moya
