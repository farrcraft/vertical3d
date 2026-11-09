/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include "Shapes.h"

#include <api/render/offline/rib/Parameters.h>
#include <api/render/offline/rib/requests/Cone.h>
#include <api/render/offline/rib/requests/Curves.h>
#include <api/render/offline/rib/requests/Cylinder.h>
#include <api/render/offline/rib/requests/Disk.h>
#include <api/render/offline/rib/requests/Hyperboloid.h>
#include <api/render/offline/rib/requests/Paraboloid.h>
#include <api/render/offline/rib/requests/PointsPolygons.h>
#include <api/render/offline/rib/requests/Sphere.h>
#include <api/render/offline/rib/requests/SubdivisionMesh.h>
#include <api/render/offline/rib/requests/Torus.h>
#include <moya/libmoya/ri/internal/Primvars.h>
#include <moya/libmoya/scene/Value.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <glm/vec3.hpp>

#include "Shape.h"
#include "Warnings.h"

namespace v3d::moya::ri::internal {

namespace {

typedef v3d::render::offline::rib::ParameterList ParameterList;

};  // namespace

scene::Value Shapes::pointsOf(const ParameterList & parameters) {
    for (const char * name : { "P", "Pw" }) {
        if (const std::optional<scene::Value> found = valueOf(parameters, name)) {
            return *found;
        }
    }
    return scene::Value::points({});
}

scene::Value Shapes::integersOf(const std::vector<unsigned int> & counts) {
    std::vector<std::int32_t> values;
    values.reserve(counts.size());
    for (const unsigned int count : counts) {
        values.push_back(static_cast<std::int32_t>(count));
    }
    return scene::Value::integers(values);
}

std::string Shapes::boundaryOf(unsigned int rule) {
    if (rule == 0) {
        return "none";
    }
    if (rule == 2) {
        return "edges";
    }
    return "edges-and-corners";
}

bool Shapes::addTag(const std::string & name, const std::vector<unsigned int> & given, const std::vector<float> & sharpness, Edges * edges) {
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

void Shapes::widths(const ParameterList & parameters, Shape * shape) {
    if (parameters.has("width")) {
        shape->parameters.emplace_back("width", scene::Value::floats(parameters.floats("width")));
    }
    if (parameters.has("constantwidth")) {
        shape->parameters.emplace_back("constantWidth", scene::Value::number(parameters.number("constantwidth", 1.0F)));
    }
}

Shape Shapes::quadric(const std::string & kind, const std::string & request, const ParameterList & parameters,
    const std::vector<std::pair<std::string, scene::Value>> & dimensions) {
    return Shape { .kind = kind, .request = request, .parameters = dimensions, .primvars = parameters, .taken = {} };
}

Shape Shapes::polygon(unsigned int vertices, const ParameterList & parameters) {
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

Shape Shapes::mesh(const v3d::render::offline::rib::requests::PointsPolygons & request, const ParameterList & parameters) {
    return Shape { .kind = "mesh",
        .request = "PointsPolygons",
        .parameters = { { "P", pointsOf(parameters) }, { "vertices", integersOf(request.counts) }, { "indices", integersOf(request.indices) } },
        .primvars = parameters,
        .taken = { "P", "Pw", "Pz" } };
}

Shape Shapes::curves(const v3d::render::offline::rib::requests::Curves & request, const ParameterList & parameters, const std::string & basis) {
    Shape shape { .kind = "curves",
        .request = "Curves",
        .parameters = { { "P", pointsOf(parameters) }, { "vertices", integersOf(request.vertices) },
            { "type", scene::Value::string(request.cubic ? "cubic" : "linear") },
            { "wrap", scene::Value::string(request.periodic ? "periodic" : "nonperiodic") },
            { "basis", scene::Value::string(basis) } },
        .primvars = parameters,
        .taken = { "P", "Pw", "Pz", "width", "constantwidth" } };
    widths(parameters, &shape);
    return shape;
}

Shape Shapes::points(const ParameterList & parameters) {
    Shape shape { .kind = "points",
        .request = "Points",
        .parameters = { { "P", pointsOf(parameters) } },
        .primvars = parameters,
        .taken = { "P", "Pw", "Pz", "width", "constantwidth" } };
    widths(parameters, &shape);
    return shape;
}

Shape Shapes::subdivision(const v3d::render::offline::rib::requests::SubdivisionMesh & request, const ParameterList & parameters, Warnings * warnings) {
    std::string scheme = request.scheme;
    if (scheme != "catmull-clark" && scheme != "loop" && scheme != "bilinear") {
        warnings->once("scheme " + scheme, "moya subdivides by catmull-clark, loop or bilinear, so a \"" + scheme + "\" mesh is subdivided by catmull-clark");
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
            warnings->once("tag " + name, "moya does not read the subdivision tag \"" + name + "\", so it changes nothing");
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

Shape Shapes::sphere(const v3d::render::offline::rib::requests::Sphere & request, const ParameterList & parameters) {
    return quadric("sphere", "Sphere", parameters, { { "radius", scene::Value::number(request.radius) },
        { "zmin", scene::Value::number(request.zmin) }, { "zmax", scene::Value::number(request.zmax) },
        { "thetamax", scene::Value::number(request.thetamax) } });
}

Shape Shapes::cone(const v3d::render::offline::rib::requests::Cone & request, const ParameterList & parameters) {
    return quadric("cone", "Cone", parameters, { { "height", scene::Value::number(request.height) },
        { "radius", scene::Value::number(request.radius) }, { "thetamax", scene::Value::number(request.thetamax) } });
}

Shape Shapes::cylinder(const v3d::render::offline::rib::requests::Cylinder & request, const ParameterList & parameters) {
    return quadric("cylinder", "Cylinder", parameters, { { "radius", scene::Value::number(request.radius) },
        { "zmin", scene::Value::number(request.zmin) }, { "zmax", scene::Value::number(request.zmax) },
        { "thetamax", scene::Value::number(request.thetamax) } });
}

Shape Shapes::hyperboloid(const v3d::render::offline::rib::requests::Hyperboloid & request, const ParameterList & parameters) {
    return quadric("hyperboloid", "Hyperboloid", parameters, {
        { "point1", scene::Value::point(glm::vec3(request.point1[0], request.point1[1], request.point1[2])) },
        { "point2", scene::Value::point(glm::vec3(request.point2[0], request.point2[1], request.point2[2])) },
        { "thetamax", scene::Value::number(request.thetamax) } });
}

Shape Shapes::paraboloid(const v3d::render::offline::rib::requests::Paraboloid & request, const ParameterList & parameters) {
    return quadric("paraboloid", "Paraboloid", parameters, { { "rmax", scene::Value::number(request.rmax) },
        { "zmin", scene::Value::number(request.zmin) }, { "zmax", scene::Value::number(request.zmax) },
        { "thetamax", scene::Value::number(request.thetamax) } });
}

Shape Shapes::disk(const v3d::render::offline::rib::requests::Disk & request, const ParameterList & parameters) {
    return quadric("disk", "Disk", parameters, { { "height", scene::Value::number(request.height) },
        { "radius", scene::Value::number(request.radius) }, { "thetamax", scene::Value::number(request.thetamax) } });
}

Shape Shapes::torus(const v3d::render::offline::rib::requests::Torus & request, const ParameterList & parameters) {
    return quadric("torus", "Torus", parameters, { { "majorRadius", scene::Value::number(request.majorRadius) },
        { "minorRadius", scene::Value::number(request.minorRadius) }, { "phimin", scene::Value::number(request.phimin) },
        { "phimax", scene::Value::number(request.phimax) }, { "thetamax", scene::Value::number(request.thetamax) } });
}

};  // namespace v3d::moya::ri::internal
