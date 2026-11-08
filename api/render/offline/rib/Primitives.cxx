/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

// The geometry requests of a RIB reader, which Reader.cxx offers each request it does not
// recognise among the others.

#include "Reader.h"

#include "Handler.h"
#include "Lexer.h"
#include "Parameters.h"
#include "Requests.h"
#include "Token.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace v3d::render::offline::rib {

namespace {

/** A quadric has a varying value at each of the four corners of its parameter space. **/
constexpr unsigned int kQuadricCorners = 4;

};  // namespace

bool Reader::floatArray(Lexer * lexer, std::vector<float> * out) {
    const Token open = lexer->next();
    if (open.kind() != Kind::ARRAY_BEGIN) {
        return fail("expected '['", open);
    }
    for (;;) {
        const Token token = lexer->next();
        if (token.kind() == Kind::ARRAY_END) {
            return true;
        }
        if (token.kind() != Kind::NUMBER) {
            return fail("expected a number", token);
        }
        out->push_back(token.value());
    }
}

bool Reader::strings(Lexer * lexer, std::vector<std::string> * out) {
    const Token open = lexer->next();
    if (open.kind() != Kind::ARRAY_BEGIN) {
        return fail("expected '['", open);
    }
    for (;;) {
        const Token token = lexer->next();
        if (token.kind() == Kind::ARRAY_END) {
            return true;
        }
        if (token.kind() != Kind::STRING) {
            return fail("expected a quoted string", token);
        }
        out->push_back(token.text());
    }
}

/**
 * Reads a Curves request: its type, its counts, its wrap and its parameters.
 **/
Reader::Result Reader::curvesRequest(Lexer * lexer, Handler * handler) {
    ParameterList list;
    std::string type;
    std::vector<unsigned int> vertices;
    std::string wrap;
    const Token at = lexer->peek();
    if (!text(lexer, &type) || !counts(lexer, &vertices) || !text(lexer, &wrap)) {
        return Result::Failed;
    }
    if ((type != "linear" && type != "cubic") || (wrap != "periodic" && wrap != "nonperiodic")) {
        fail("a curve is linear or cubic, and periodic or nonperiodic", at);
        return Result::Failed;
    }
    // the length of a varying parameter depends on the basis's step, which the reader does
    // not track, so no length is checked
    if (!parameters(lexer, 0, &list)) {
        return Result::Failed;
    }
    handler->curves({.cubic = type == "cubic", .vertices = std::move(vertices), .periodic = wrap == "periodic"}, list);
    return Result::Handled;
}

/**
 * Reads a SubdivisionMesh request: its scheme, its faces, its tags and their arguments, and its
 * parameters. The tags' arguments are checked against the counts each tag gives.
 **/
Reader::Result Reader::subdivisionRequest(Lexer * lexer, Handler * handler) {
    SubdivisionMesh mesh;
    const Token at = lexer->peek();
    if (!text(lexer, &mesh.scheme) || !counts(lexer, &mesh.counts) || !counts(lexer, &mesh.indices) || !strings(lexer, &mesh.tags) ||
        !counts(lexer, &mesh.arguments) || !counts(lexer, &mesh.integers) || !floatArray(lexer, &mesh.floats)) {
        return Result::Failed;
    }

    // two counts per tag, or three when RIB gives each tag strings too
    const std::size_t tags = mesh.tags.size();
    const std::size_t each = tags == 0 ? 2 : mesh.arguments.size() / tags;
    if ((each != 2 && each != 3) || mesh.arguments.size() != each * tags) {
        fail("a subdivision mesh gives two or three argument counts for each tag", at);
        return Result::Failed;
    }
    if (each == 3 && !strings(lexer, &mesh.strings)) {
        return Result::Failed;
    }
    std::array<std::size_t, 3> wanted { 0, 0, 0 };
    for (std::size_t i = 0; i < mesh.arguments.size(); ++i) {
        wanted.at(i % each) += mesh.arguments[i];
    }
    if (wanted[0] != mesh.integers.size() || wanted[1] != mesh.floats.size() || wanted[2] != mesh.strings.size()) {
        fail("a subdivision mesh's tags take more or fewer arguments than it gives", at);
        return Result::Failed;
    }

    ParameterList list;
    if (!parameters(lexer, 0, &list)) {
        return Result::Failed;
    }
    handler->subdivisionMesh(mesh, list);
    return Result::Handled;
}

/**
 * Reads the quadrics: each a fixed run of numbers, then a parameter list of one value per
 * corner of its parameter space.
 **/
Reader::Result Reader::quadricRequest(const std::string & name, Lexer * lexer, Handler * handler) {
    static const std::array<std::pair<const char *, unsigned int>, 7> kArguments { {
        { "Sphere", 4 }, { "Cone", 3 }, { "Cylinder", 4 }, { "Hyperboloid", 7 }, { "Paraboloid", 4 }, { "Disk", 3 }, { "Torus", 5 } } };
    unsigned int count = 0;
    for (const auto & [quadric, arguments] : kArguments) {
        if (name == quadric) {
            count = arguments;
        }
    }
    if (count == 0) {
        return Result::Unhandled;
    }

    std::vector<float> values;
    ParameterList list;
    if (!numbers(lexer, count, &values) || !parameters(lexer, kQuadricCorners, &list)) {
        return Result::Failed;
    }
    if (name == "Sphere") {
        handler->sphere({.radius = values[0], .zmin = values[1], .zmax = values[2], .thetamax = values[3]}, list);
    } else if (name == "Cone") {
        handler->cone({.height = values[0], .radius = values[1], .thetamax = values[2]}, list);
    } else if (name == "Cylinder") {
        handler->cylinder({.radius = values[0], .zmin = values[1], .zmax = values[2], .thetamax = values[3]}, list);
    } else if (name == "Hyperboloid") {
        // two points of three numbers each, then the sweep
        Hyperboloid hyperboloid { .thetamax = values.back() };
        std::copy_n(values.begin(), hyperboloid.point1.size(), hyperboloid.point1.begin());
        std::copy_n(values.begin() + static_cast<std::ptrdiff_t>(hyperboloid.point1.size()), hyperboloid.point2.size(), hyperboloid.point2.begin());
        handler->hyperboloid(hyperboloid, list);
    } else if (name == "Paraboloid") {
        handler->paraboloid({.rmax = values[0], .zmin = values[1], .zmax = values[2], .thetamax = values[3]}, list);
    } else if (name == "Disk") {
        handler->disk({.height = values[0], .radius = values[1], .thetamax = values[2]}, list);
    } else {
        handler->torus({.majorRadius = values[0], .minorRadius = values[1], .phimin = values[2], .phimax = values[3], .thetamax = values[4]}, list);
    }
    return Result::Handled;
}

/**
 * Reads the geometry.
 **/
Reader::Result Reader::primitiveRequest(const std::string & name, Lexer * lexer, Handler * handler) {
    ParameterList list;

    if (name == "Polygon") {
        // RIB carries no vertex count: it is the length of "P", "Pw" or "Pz"
        if (!parameters(lexer, 0, &list)) {
            return Result::Failed;
        }
        unsigned int vertices = static_cast<unsigned int>(list.floats("P").size() / 3);
        if (vertices == 0) {
            vertices = static_cast<unsigned int>(list.floats("Pw").size() / 4);
        }
        if (vertices == 0) {
            vertices = static_cast<unsigned int>(list.floats("Pz").size());
        }
        handler->polygon(vertices, list);
        return Result::Handled;
    }
    if (name == "PointsPolygons") {
        std::vector<unsigned int> perPolygon;
        std::vector<unsigned int> indices;
        if (!counts(lexer, &perPolygon) || !counts(lexer, &indices) || !parameters(lexer, 0, &list)) {
            return Result::Failed;
        }
        handler->pointsPolygons({.counts = std::move(perPolygon), .indices = std::move(indices)}, list);
        return Result::Handled;
    }
    if (name == "Points") {
        // RIB carries no count: it is the length of "P", or of "Pz"
        if (!parameters(lexer, 0, &list)) {
            return Result::Failed;
        }
        unsigned int count = static_cast<unsigned int>(list.floats("P").size() / 3);
        if (count == 0) {
            count = static_cast<unsigned int>(list.floats("Pz").size());
        }
        handler->points(count, list);
        return Result::Handled;
    }
    if (name == "Curves") {
        return curvesRequest(lexer, handler);
    }
    if (name == "SubdivisionMesh") {
        return subdivisionRequest(lexer, handler);
    }
    return quadricRequest(name, lexer, handler);
}

};  // namespace v3d::render::offline::rib
