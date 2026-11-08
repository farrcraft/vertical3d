/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

// The world's requests of a SceneClient: geometry, objects, shaders and lights.

#include "SceneClient.h"

#include <api/log/Logger.h>
#include <api/render/offline/rib/Bases.h>
#include <api/render/offline/rib/Handler.h>
#include <api/render/offline/rib/Parameters.h>
#include <api/render/offline/rib/Requests.h>
#include <moya/libmoya/scene/Placement.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Schema.h>
#include <moya/libmoya/scene/Value.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <boost/make_shared.hpp>
#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "internal/Primvars.h"

namespace v3d::moya::ri {

namespace {

typedef v3d::render::offline::rib::ParameterList ParameterList;

/** RI's spotlight defaults, in radians. **/
constexpr float kConeAngle = 0.5235988F;
constexpr float kConeDelta = 0.0872665F;

/** A point a light's parameters give, or the fallback when they give none. **/
glm::vec3 pointOf(const ParameterList & parameters, const std::string & name, const glm::vec3 & fallback) {
    const std::vector<glm::vec3> found = parameters.points(name);
    return found.empty() ? fallback : found.front();
}

/** The points of a request: its "P", or its "Pw" divided out. **/
scene::Value pointsOf(const ParameterList & parameters) {
    for (const char * name : { "P", "Pw" }) {
        if (const std::optional<scene::Value> found = internal::valueOf(parameters, name)) {
            return *found;
        }
    }
    return scene::Value::points({});
}

/** Counts as the scene's integers. **/
scene::Value integersOf(const std::vector<unsigned int> & counts) {
    std::vector<std::int32_t> values;
    values.reserve(counts.size());
    for (const unsigned int count : counts) {
        values.push_back(static_cast<std::int32_t>(count));
    }
    return scene::Value::integers(values);
}

};  // namespace

std::string SceneClient::boundaryOf(unsigned int rule) {
    if (rule == 0) {
        return "none";
    }
    if (rule == 2) {
        return "edges";
    }
    return "edges-and-corners";
}

bool SceneClient::addTag(const std::string & name, const std::vector<unsigned int> & given, const std::vector<float> & sharpness, Edges * edges) {
    if (name == "crease") {
        edges->creases.insert(edges->creases.end(), given.begin(), given.end());
        edges->creaseLengths.push_back(static_cast<std::int32_t>(given.size()));
        edges->creaseSharpness.push_back(sharpness.empty() ? 0.0F : sharpness.front());
    } else if (name == "corner") {
        for (std::size_t i = 0; i < given.size(); ++i) {
            edges->corners.push_back(static_cast<std::int32_t>(given[i]));
            edges->cornerSharpness.push_back(sharpness.empty() ? 0.0F : sharpness[std::min(i, sharpness.size() - 1)]);
        }
    } else if (name == "hole") {
        edges->holes.insert(edges->holes.end(), given.begin(), given.end());
    } else if (name == "interpolateboundary") {
        edges->boundary = boundaryOf(given.empty() ? 1U : given.front());
    } else {
        return false;
    }
    return true;
}

SceneClient::Deforming::Deforming(SceneClient * client) : client_(client) {
}

void SceneClient::Deforming::polygon(unsigned int vertices, const ParameterList & parameters) {
    client_->deform(polygonShape(vertices, parameters));
}

void SceneClient::Deforming::pointsPolygons(const v3d::render::offline::rib::PointsPolygons & request, const ParameterList & parameters) {
    client_->deform(meshShape(request, parameters));
}

void SceneClient::Deforming::sphere(const v3d::render::offline::rib::Sphere & request, const ParameterList & parameters) {
    client_->deform(quadricShape("sphere", "Sphere", parameters, { { "radius", scene::Value::number(request.radius) },
        { "zmin", scene::Value::number(request.zmin) }, { "zmax", scene::Value::number(request.zmax) },
        { "thetamax", scene::Value::number(request.thetamax) } }));
}

void SceneClient::Deforming::curves(const v3d::render::offline::rib::Curves & request, const ParameterList & parameters) {
    client_->deform(client_->curvesShape(request, parameters));
}

void SceneClient::Deforming::points(unsigned int count, const ParameterList & parameters) {
    (void)count;
    client_->deform(pointsShape(parameters));
}

void SceneClient::Deforming::cone(const v3d::render::offline::rib::Cone & request, const ParameterList & parameters) {
    client_->deform(quadricShape("cone", "Cone", parameters, { { "height", scene::Value::number(request.height) },
        { "radius", scene::Value::number(request.radius) }, { "thetamax", scene::Value::number(request.thetamax) } }));
}

void SceneClient::Deforming::cylinder(const v3d::render::offline::rib::Cylinder & request, const ParameterList & parameters) {
    client_->deform(quadricShape("cylinder", "Cylinder", parameters, { { "radius", scene::Value::number(request.radius) },
        { "zmin", scene::Value::number(request.zmin) }, { "zmax", scene::Value::number(request.zmax) },
        { "thetamax", scene::Value::number(request.thetamax) } }));
}

void SceneClient::Deforming::hyperboloid(const v3d::render::offline::rib::Hyperboloid & request, const ParameterList & parameters) {
    client_->deform(quadricShape("hyperboloid", "Hyperboloid", parameters, {
        { "point1", scene::Value::point(glm::vec3(request.point1[0], request.point1[1], request.point1[2])) },
        { "point2", scene::Value::point(glm::vec3(request.point2[0], request.point2[1], request.point2[2])) },
        { "thetamax", scene::Value::number(request.thetamax) } }));
}

void SceneClient::Deforming::paraboloid(const v3d::render::offline::rib::Paraboloid & request, const ParameterList & parameters) {
    client_->deform(quadricShape("paraboloid", "Paraboloid", parameters, { { "rmax", scene::Value::number(request.rmax) },
        { "zmin", scene::Value::number(request.zmin) }, { "zmax", scene::Value::number(request.zmax) },
        { "thetamax", scene::Value::number(request.thetamax) } }));
}

void SceneClient::Deforming::disk(const v3d::render::offline::rib::Disk & request, const ParameterList & parameters) {
    client_->deform(quadricShape("disk", "Disk", parameters, { { "height", scene::Value::number(request.height) },
        { "radius", scene::Value::number(request.radius) }, { "thetamax", scene::Value::number(request.thetamax) } }));
}

void SceneClient::Deforming::torus(const v3d::render::offline::rib::Torus & request, const ParameterList & parameters) {
    client_->deform(quadricShape("torus", "Torus", parameters, { { "majorRadius", scene::Value::number(request.majorRadius) },
        { "minorRadius", scene::Value::number(request.minorRadius) }, { "phimin", scene::Value::number(request.phimin) },
        { "phimax", scene::Value::number(request.phimax) }, { "thetamax", scene::Value::number(request.thetamax) } }));
}

void SceneClient::Deforming::subdivisionMesh(const v3d::render::offline::rib::SubdivisionMesh & request, const ParameterList & parameters) {
    client_->deform(client_->subdivisionShape(request, parameters));
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the key, then what is logged
void SceneClient::once(const std::string & key, const std::string & message) {
    if (logged_.insert(key).second) {
        logger_->get()->warn("{}", message);
    }
}

std::string SceneClient::nameFor(const std::string & kind, const std::string & request) {
    return unique(kind, current_.name.empty() ? request + " " + std::to_string(++counts_[kind + " " + request]) : current_.name);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the kind, then the name, as in create()
std::string SceneClient::unique(const std::string & kind, const std::string & base) {
    std::string name = base;
    const std::set<std::string> & taken = taken_[kind];
    for (unsigned int again = 2; taken.contains(name); ++again) {
        name = base + " " + std::to_string(again);
    }
    return name;
}

scene::Handle SceneClient::make(const std::string & kind, const std::string & name) {
    const scene::Handle made = scene_.create(kind, name);
    taken_[kind].insert(name);
    made_.push_back(made);
    return made;
}

SceneClient::Shape SceneClient::polygonShape(unsigned int vertices, const ParameterList & parameters) {
    std::vector<std::int32_t> indices;
    indices.reserve(vertices);
    for (unsigned int i = 0; i < vertices; ++i) {
        indices.push_back(static_cast<std::int32_t>(i));
    }
    return Shape { .kind = "mesh",
        .request = "Polygon",
        .parameters = { { "P", pointsOf(parameters) }, { "vertices", scene::Value::integers({ static_cast<std::int32_t>(vertices) }) },
            { "indices", scene::Value::integers(indices) } },
        .primvars = parameters,
        .taken = { "P", "Pw", "Pz" } };
}

SceneClient::Shape SceneClient::meshShape(const v3d::render::offline::rib::PointsPolygons & request, const ParameterList & parameters) {
    return Shape { .kind = "mesh",
        .request = "PointsPolygons",
        .parameters = { { "P", pointsOf(parameters) }, { "vertices", integersOf(request.counts) }, { "indices", integersOf(request.indices) } },
        .primvars = parameters,
        .taken = { "P", "Pw", "Pz" } };
}

SceneClient::Shape SceneClient::curvesShape(const v3d::render::offline::rib::Curves & request, const ParameterList & parameters) const {
    Shape shape { .kind = "curves",
        .request = "Curves",
        .parameters = { { "P", pointsOf(parameters) }, { "vertices", integersOf(request.vertices) },
            { "type", scene::Value::string(request.cubic ? "cubic" : "linear") },
            { "wrap", scene::Value::string(request.periodic ? "periodic" : "nonperiodic") },
            { "basis", scene::Value::string(current_.basis) } },
        .primvars = parameters,
        .taken = { "P", "Pw", "Pz", "width", "constantwidth" } };
    if (parameters.has("width")) {
        shape.parameters.emplace_back("width", scene::Value::floats(parameters.floats("width")));
    }
    if (parameters.has("constantwidth")) {
        shape.parameters.emplace_back("constantWidth", scene::Value::number(parameters.number("constantwidth", 1.0F)));
    }
    return shape;
}

SceneClient::Shape SceneClient::pointsShape(const ParameterList & parameters) {
    Shape shape { .kind = "points",
        .request = "Points",
        .parameters = { { "P", pointsOf(parameters) } },
        .primvars = parameters,
        .taken = { "P", "Pw", "Pz", "width", "constantwidth" } };
    if (parameters.has("width")) {
        shape.parameters.emplace_back("width", scene::Value::floats(parameters.floats("width")));
    }
    if (parameters.has("constantwidth")) {
        shape.parameters.emplace_back("constantWidth", scene::Value::number(parameters.number("constantwidth", 1.0F)));
    }
    return shape;
}

SceneClient::Shape SceneClient::subdivisionShape(const v3d::render::offline::rib::SubdivisionMesh & request, const ParameterList & parameters) {
    std::string scheme = request.scheme;
    if (scheme != "catmull-clark" && scheme != "loop" && scheme != "bilinear") {
        once("scheme " + scheme, "moya subdivides by catmull-clark, loop or bilinear, so a \"" + scheme + "\" mesh is subdivided by catmull-clark");
        scheme = "catmull-clark";
    }

    Edges edges;
    // each tag takes its integers and floats from the arrays in turn, two or three counts a tag
    const std::size_t each = request.tags.empty() ? 2 : request.arguments.size() / request.tags.size();
    std::size_t integer = 0;
    std::size_t real = 0;
    for (std::size_t tag = 0; tag < request.tags.size(); ++tag) {
        const std::size_t ints = request.arguments.at(tag * each);
        const std::size_t floats = request.arguments.at((tag * each) + 1);
        const std::vector<unsigned int> given(request.integers.begin() + static_cast<std::ptrdiff_t>(integer),
            request.integers.begin() + static_cast<std::ptrdiff_t>(integer + ints));
        const std::vector<float> sharpness(request.floats.begin() + static_cast<std::ptrdiff_t>(real),
            request.floats.begin() + static_cast<std::ptrdiff_t>(real + floats));
        integer += ints;
        real += floats;

        const std::string & name = request.tags[tag];
        if (!addTag(name, given, sharpness, &edges)) {
            once("tag " + name, "moya does not read the subdivision tag \"" + name + "\", so it changes nothing");
        }
    }

    return Shape { .kind = "subdivision",
        .request = "SubdivisionMesh",
        .parameters = { { "P", pointsOf(parameters) }, { "vertices", integersOf(request.counts) }, { "indices", integersOf(request.indices) },
            { "scheme", scene::Value::string(scheme) }, { "creases", scene::Value::integers(edges.creases) },
            { "creaseLengths", scene::Value::integers(edges.creaseLengths) }, { "creaseSharpness", scene::Value::floats(edges.creaseSharpness) },
            { "corners", scene::Value::integers(edges.corners) }, { "cornerSharpness", scene::Value::floats(edges.cornerSharpness) },
            { "holes", scene::Value::integers(edges.holes) }, { "boundary", scene::Value::string(edges.boundary) } },
        .primvars = parameters,
        .taken = { "P", "Pw", "Pz" } };
}

SceneClient::Shape SceneClient::quadricShape(const std::string & kind, const std::string & request, const ParameterList & parameters,
    const std::vector<std::pair<std::string, scene::Value>> & dimensions) {
    return Shape { .kind = kind, .request = request, .parameters = dimensions, .primvars = parameters, .taken = {} };
}

void SceneClient::place(const Shape & shape) {
    if (!world_) {
        once("outside", "a primitive outside WorldBegin and WorldEnd is not drawn");
        return;
    }
    const scene::Handle prototype = make(shape.kind, nameFor(shape.kind, shape.request));
    for (const auto & [name, value] : shape.parameters) {
        scene_.set(prototype, name, value);
    }

    // the request's other parameters are primvars, unless the kind holds one of the name
    std::set<std::string> except = shape.taken;
    const scene::Kind & kind = scene_.schema().kind(prototype.kind);
    for (const scene::Declaration & declared : kind.parameters) {
        except.insert(declared.name);
    }
    internal::declare(&scene_, prototype, shape.primvars, except, false);

    // the first pose of a deforming primitive, whose later poses add samples to it
    if (motion_) {
        deforming_ = prototype;
        samples_.clear();
        for (const auto & [name, value] : shape.parameters) {
            const auto declared = std::find_if(kind.parameters.begin(), kind.parameters.end(),
                [&name](const scene::Declaration & each) { return each.name == name; });
            if (declared != kind.parameters.end() && declared->motion) {
                samples_[name].push_back(value);
            }
        }
    }

    const bool inObject = current_.group.valid();
    instance(prototype, nameFor("instance", shape.request), !inObject);

    if (current_.area && !inObject) {
        const scene::Handle glow = make("meshLight", unique("meshLight", current_.area->name));
        scene_.set(glow, "geometry", scene::Value::handle(prototype));
        setSampled(glow, "transform", current_.transformation);
        scene_.set(glow, "colour", scene::Value::colour(pointOf(current_.area->parameters, "lightcolor", glm::vec3(1.0F))));
        scene_.set(glow, "intensity", scene::Value::number(current_.area->parameters.number("intensity", 1.0F)));
    }
}

void SceneClient::deform(const Shape & shape) {
    if (!deforming_.valid() || shape.kind != scene_.schema().kind(deforming_.kind).name) {
        once("deform " + shape.kind, "a pose of a different primitive inside a motion block is left out");
        return;
    }
    for (const auto & [name, value] : shape.parameters) {
        const auto found = samples_.find(name);
        if (found != samples_.end()) {
            found->second.push_back(value);
        }
    }
}

scene::Handle SceneClient::instance(const scene::Handle & prototype, const std::string & name, bool lit) {
    const scene::Handle placed = make("instance", name);
    scene_.set(placed, "prototype", scene::Value::handle(prototype));
    setSampled(placed, "transform", current_.transformation);
    if (current_.group.valid()) {
        // an object's geometry takes the material in force where the object is placed
        scene_.set(placed, "group", scene::Value::handle(current_.group));
    } else {
        scene_.set(placed, "material", scene::Value::handle(material()));
    }
    if (lit) {
        instances_.push_back(placed);
        for (const std::string & on : current_.lights) {
            const auto found = lights_.find(on);
            if (found != lights_.end()) {
                found->second.lit.push_back(placed);
            }
        }
    }
    return placed;
}

scene::Handle SceneClient::shaderNode(const Shader & shader, const std::string & name) {
    const scene::Handle node = make("shader", name);
    scene_.set(node, "shader", scene::Value::string(shader.name));
    scene_.set(node, "language", scene::Value::string("ri"));
    internal::declare(&scene_, node, shader.parameters, { "shader", "language" }, true);
    return node;
}

scene::Handle SceneClient::material() {
    if (current_.material->valid()) {
        return *current_.material;
    }
    // a material and its nodes are named after the surface shader, whatever geometry made them
    const scene::Handle surface = shaderNode(current_.surface, unique("shader", current_.surface.name));
    for (const auto & [input, value] : { std::pair { "Cs", current_.colour }, std::pair { "Os", current_.opacity } }) {
        if (!current_.surface.parameters.has(input)) {
            scene_.declare(surface, scene::Declaration { .name = input, .type = scene::Type::Colour, .fallback = scene::Value::colour(value) });
        }
    }
    const scene::Handle made = make("material", unique("material", current_.surface.name));
    scene_.set(made, "surface", scene::Value::handle(surface));
    if (!current_.displacement.name.empty()) {
        const scene::Handle displacement = shaderNode(current_.displacement, unique("shader", current_.displacement.name));
        scene_.set(made, "displacement", scene::Value::handle(displacement));
    }
    *current_.material = made;
    return made;
}

scene::Handle SceneClient::light(const std::string & kind, const std::string & name, const ParameterList & parameters, const glm::vec3 & defaultTo) {
    const scene::Handle made = make(kind, name);
    const glm::mat4 placed = scene::aimed(pointOf(parameters, "from", glm::vec3(0.0F)), pointOf(parameters, "to", defaultTo));
    Sampled transformation = current_.transformation;
    for (glm::mat4 & matrix : transformation.matrices) {
        matrix = matrix * placed;
    }
    setSampled(made, "transform", transformation);
    scene_.set(made, "colour", scene::Value::colour(pointOf(parameters, "lightcolor", glm::vec3(1.0F))));
    scene_.set(made, "intensity", scene::Value::number(parameters.number("intensity", 1.0F)));
    return made;
}

void SceneClient::coordinateSystem(const std::string & space) {
    if (!space.empty()) {
        spaces_[space] = current_.transformation;
    }
}

void SceneClient::coordSysTransform(const std::string & space) {
    if (space == "world") {
        current_.transformation = Sampled();
        return;
    }
    const auto found = spaces_.find(space);
    if (found == spaces_.end()) {
        once("space " + space, "the space \"" + space + "\" was not named, so CoordSysTransform changes nothing");
        return;
    }
    current_.transformation = found->second;
}

void SceneClient::color(const glm::vec3 & value) {
    current_.colour = value;
    current_.material = boost::make_shared<scene::Handle>();
}

void SceneClient::opacity(const glm::vec3 & value) {
    current_.opacity = value;
    current_.material = boost::make_shared<scene::Handle>();
}

void SceneClient::basis(const v3d::render::offline::rib::Basis & request) {
    std::string name = v3d::render::offline::rib::basisName(request.v);
    if (name.empty()) {
        once("basis", "moya's curves take a basis RI names, so a basis of its own numbers is taken as bezier");
        name = "bezier";
    }
    current_.basis = name;
}

void SceneClient::surface(const std::string & name, const ParameterList & parameters) {
    if (name.empty()) {
        return;
    }
    current_.surface = Shader { .name = name, .parameters = parameters };
    current_.material = boost::make_shared<scene::Handle>();
}

void SceneClient::displacement(const std::string & name, const ParameterList & parameters) {
    if (name.empty()) {
        return;
    }
    current_.displacement = Shader { .name = name, .parameters = parameters };
    current_.material = boost::make_shared<scene::Handle>();
}

void SceneClient::lightSource(const v3d::render::offline::rib::LightSource & request, const ParameterList & parameters) {
    if (request.shader.empty()) {
        return;
    }
    if (!world_) {
        once("light outside", "a light outside WorldBegin and WorldEnd makes no light");
        return;
    }
    const std::string name = current_.name.empty() ? "light " + request.handle : current_.name;
    scene::Handle made;
    if (request.shader == "pointlight") {
        // a point light has no direction, so it looks down z from where it is
        const glm::vec3 from = pointOf(parameters, "from", glm::vec3(0.0F));
        made = light("sphereLight", unique("sphereLight", name), parameters, from + glm::vec3(0.0F, 0.0F, 1.0F));
        scene_.set(made, "radius", scene::Value::number(0.0F));
    } else if (request.shader == "distantlight") {
        made = light("distantLight", unique("distantLight", name), parameters, glm::vec3(0.0F, 0.0F, 1.0F));
        scene_.set(made, "angle", scene::Value::number(0.0F));
    } else if (request.shader == "spotlight") {
        made = light("sphereLight", unique("sphereLight", name), parameters, glm::vec3(0.0F, 0.0F, 1.0F));
        scene_.set(made, "radius", scene::Value::number(0.0F));
        const float cone = parameters.number("coneangle", kConeAngle);
        const float delta = parameters.number("conedeltaangle", kConeDelta);
        scene_.set(made, "coneAngle", scene::Value::number(glm::degrees(cone)));
        scene_.set(made, "coneSoftness", scene::Value::number(cone > 0.0F ? std::clamp(delta / cone, 0.0F, 1.0F) : 0.0F));
    } else {
        once("light " + request.shader, "moya makes no light of the shader \"" + request.shader + "\"");
        unmade_.insert(request.handle);
        return;
    }
    lights_[request.handle] = Light { .light = made, .lit = {} };
    current_.lights.insert(request.handle);
}

void SceneClient::areaLightSource(const v3d::render::offline::rib::LightSource & request, const ParameterList & parameters) {
    if (request.shader.empty()) {
        current_.area.reset();
        return;
    }
    current_.area = Shader { .name = current_.name.empty() ? "light " + request.handle : current_.name, .parameters = parameters };
}

void SceneClient::illuminate(const std::string & handle, bool on) {
    if (unmade_.contains(handle)) {
        return;
    }
    if (!lights_.contains(handle)) {
        once("illuminate " + handle, "Illuminate names the light " + handle + ", which the world has not made");
        return;
    }
    if (on) {
        current_.lights.insert(handle);
    } else {
        current_.lights.erase(handle);
    }
}

v3d::render::offline::rib::Handler* SceneClient::deformation() {
    return &deformations_;
}

void SceneClient::polygon(unsigned int vertices, const ParameterList & parameters) {
    place(polygonShape(vertices, parameters));
}

void SceneClient::pointsPolygons(const v3d::render::offline::rib::PointsPolygons & request, const ParameterList & parameters) {
    place(meshShape(request, parameters));
}

void SceneClient::sphere(const v3d::render::offline::rib::Sphere & request, const ParameterList & parameters) {
    place(quadricShape("sphere", "Sphere", parameters, { { "radius", scene::Value::number(request.radius) },
        { "zmin", scene::Value::number(request.zmin) }, { "zmax", scene::Value::number(request.zmax) },
        { "thetamax", scene::Value::number(request.thetamax) } }));
}

void SceneClient::curves(const v3d::render::offline::rib::Curves & request, const ParameterList & parameters) {
    place(curvesShape(request, parameters));
}

void SceneClient::points(unsigned int count, const ParameterList & parameters) {
    (void)count;
    place(pointsShape(parameters));
}

void SceneClient::cone(const v3d::render::offline::rib::Cone & request, const ParameterList & parameters) {
    place(quadricShape("cone", "Cone", parameters, { { "height", scene::Value::number(request.height) },
        { "radius", scene::Value::number(request.radius) }, { "thetamax", scene::Value::number(request.thetamax) } }));
}

void SceneClient::cylinder(const v3d::render::offline::rib::Cylinder & request, const ParameterList & parameters) {
    place(quadricShape("cylinder", "Cylinder", parameters, { { "radius", scene::Value::number(request.radius) },
        { "zmin", scene::Value::number(request.zmin) }, { "zmax", scene::Value::number(request.zmax) },
        { "thetamax", scene::Value::number(request.thetamax) } }));
}

void SceneClient::hyperboloid(const v3d::render::offline::rib::Hyperboloid & request, const ParameterList & parameters) {
    place(quadricShape("hyperboloid", "Hyperboloid", parameters, {
        { "point1", scene::Value::point(glm::vec3(request.point1[0], request.point1[1], request.point1[2])) },
        { "point2", scene::Value::point(glm::vec3(request.point2[0], request.point2[1], request.point2[2])) },
        { "thetamax", scene::Value::number(request.thetamax) } }));
}

void SceneClient::paraboloid(const v3d::render::offline::rib::Paraboloid & request, const ParameterList & parameters) {
    place(quadricShape("paraboloid", "Paraboloid", parameters, { { "rmax", scene::Value::number(request.rmax) },
        { "zmin", scene::Value::number(request.zmin) }, { "zmax", scene::Value::number(request.zmax) },
        { "thetamax", scene::Value::number(request.thetamax) } }));
}

void SceneClient::disk(const v3d::render::offline::rib::Disk & request, const ParameterList & parameters) {
    place(quadricShape("disk", "Disk", parameters, { { "height", scene::Value::number(request.height) },
        { "radius", scene::Value::number(request.radius) }, { "thetamax", scene::Value::number(request.thetamax) } }));
}

void SceneClient::torus(const v3d::render::offline::rib::Torus & request, const ParameterList & parameters) {
    place(quadricShape("torus", "Torus", parameters, { { "majorRadius", scene::Value::number(request.majorRadius) },
        { "minorRadius", scene::Value::number(request.minorRadius) }, { "phimin", scene::Value::number(request.phimin) },
        { "phimax", scene::Value::number(request.phimax) }, { "thetamax", scene::Value::number(request.thetamax) } }));
}

void SceneClient::subdivisionMesh(const v3d::render::offline::rib::SubdivisionMesh & request, const ParameterList & parameters) {
    place(subdivisionShape(request, parameters));
}

void SceneClient::objectBegin(const std::string & handle) {
    if (!world_) {
        once("object outside", "an object outside WorldBegin and WorldEnd is not kept");
    }
    attributes_.push_back(current_);
    const scene::Handle group = make("group", unique("group", current_.name.empty() ? "object " + handle : current_.name));
    objects_[handle] = group;
    current_.transformation = Sampled();
    current_.group = group;
}

void SceneClient::objectEnd() {
    if (attributes_.empty() || !current_.group.valid()) {
        return;
    }
    current_ = attributes_.back();
    attributes_.pop_back();
}

void SceneClient::objectInstance(const std::string & handle) {
    const auto found = objects_.find(handle);
    if (found == objects_.end()) {
        once("object " + handle, "ObjectInstance names the object " + handle + ", which no ObjectBegin made");
        return;
    }
    instance(found->second, nameFor("instance", "object " + handle), !current_.group.valid());
}

};  // namespace v3d::moya::ri
