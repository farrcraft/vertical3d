/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include "Primitives.h"

#include <api/rib/Handler.h>
#include <api/rib/Lexer.h>
#include <api/rib/Parameters.h>
#include <api/rib/Token.h>
#include <api/rib/requests/Hyperboloid.h>
#include <api/rib/requests/SubdivisionMesh.h>
#include <api/rib/requests/Volume.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "Operands.h"

namespace v3d::rib::internal {

Primitives::Primitives(Operands * operands) : operands_(operands) {
}

unsigned int Primitives::quadricArguments(const std::string & name) {
    static const std::array<std::pair<const char *, unsigned int>, 7> kArguments { {
        { "Sphere", 4 }, { "Cone", 3 }, { "Cylinder", 4 }, { "Hyperboloid", 7 }, { "Paraboloid", 4 }, { "Disk", 3 }, { "Torus", 5 } } };
    for (const auto & [quadric, arguments] : kArguments) {
        if (name == quadric) {
            return arguments;
        }
    }
    return 0;
}

bool Primitives::recognises(const std::string & name) {
    return name == "Polygon" || name == "PointsPolygons" || name == "Points" || name == "Curves" || name == "SubdivisionMesh" ||
        name == "Volume" || quadricArguments(name) != 0;
}

bool Primitives::read(const std::string & name, Lexer * lexer, Handler * handler) {
    if (name == "Polygon") {
        return polygon(lexer, handler);
    }
    if (name == "PointsPolygons") {
        return pointsPolygons(lexer, handler);
    }
    if (name == "Points") {
        return points(lexer, handler);
    }
    if (name == "Curves") {
        return curves(lexer, handler);
    }
    if (name == "SubdivisionMesh") {
        return subdivision(lexer, handler);
    }
    if (name == "Volume") {
        return volume(lexer, handler);
    }
    return quadric(name, lexer, handler);
}

bool Primitives::polygon(Lexer * lexer, Handler * handler) {
    // RIB carries no vertex count: it is the length of "P", "Pw" or "Pz"
    ParameterList list;
    if (!operands_->parameters(lexer, 0, &list)) {
        return false;
    }
    unsigned int vertices = static_cast<unsigned int>(list.floats("P").size() / 3);
    if (vertices == 0) {
        vertices = static_cast<unsigned int>(list.floats("Pw").size() / 4);
    }
    if (vertices == 0) {
        vertices = static_cast<unsigned int>(list.floats("Pz").size());
    }
    handler->polygon(vertices, list);
    return true;
}

bool Primitives::pointsPolygons(Lexer * lexer, Handler * handler) {
    ParameterList list;
    std::vector<unsigned int> perPolygon;
    std::vector<unsigned int> indices;
    if (!operands_->counts(lexer, &perPolygon) || !operands_->counts(lexer, &indices) || !operands_->parameters(lexer, 0, &list)) {
        return false;
    }
    handler->pointsPolygons({.counts = std::move(perPolygon), .indices = std::move(indices)}, list);
    return true;
}

bool Primitives::points(Lexer * lexer, Handler * handler) {
    // RIB carries no count: it is the length of "P", or of "Pz"
    ParameterList list;
    if (!operands_->parameters(lexer, 0, &list)) {
        return false;
    }
    unsigned int count = static_cast<unsigned int>(list.floats("P").size() / 3);
    if (count == 0) {
        count = static_cast<unsigned int>(list.floats("Pz").size());
    }
    handler->points(count, list);
    return true;
}

bool Primitives::curves(Lexer * lexer, Handler * handler) {
    ParameterList list;
    std::string type;
    std::vector<unsigned int> vertices;
    std::string wrap;
    const Token at = lexer->peek();
    if (!operands_->text(lexer, &type) || !operands_->counts(lexer, &vertices) || !operands_->text(lexer, &wrap)) {
        return false;
    }
    if ((type != "linear" && type != "cubic") || (wrap != "periodic" && wrap != "nonperiodic")) {
        return operands_->fail("a curve is linear or cubic, and periodic or nonperiodic", at);
    }
    // the length of a varying parameter depends on the basis's step, which the reader does
    // not track, so no length is checked
    if (!operands_->parameters(lexer, 0, &list)) {
        return false;
    }
    handler->curves({.cubic = type == "cubic", .vertices = std::move(vertices), .periodic = wrap == "periodic"}, list);
    return true;
}

bool Primitives::subdivision(Lexer * lexer, Handler * handler) {
    requests::SubdivisionMesh mesh;
    const Token at = lexer->peek();
    if (!operands_->text(lexer, &mesh.scheme) || !operands_->counts(lexer, &mesh.counts) || !operands_->counts(lexer, &mesh.indices) ||
        !operands_->strings(lexer, &mesh.tags) || !operands_->counts(lexer, &mesh.arguments) || !operands_->counts(lexer, &mesh.integers) ||
        !operands_->floatArray(lexer, &mesh.floats)) {
        return false;
    }

    // two counts per tag, or three when RIB gives each tag strings too
    const std::size_t tags = mesh.tags.size();
    const std::size_t each = tags == 0 ? 2 : mesh.arguments.size() / tags;
    if ((each != 2 && each != 3) || mesh.arguments.size() != each * tags) {
        return operands_->fail("a subdivision mesh gives two or three argument counts for each tag", at);
    }
    if (each == 3 && !operands_->strings(lexer, &mesh.strings)) {
        return false;
    }
    std::array<std::size_t, 3> wanted { 0, 0, 0 };
    for (std::size_t i = 0; i < mesh.arguments.size(); ++i) {
        wanted.at(i % each) += mesh.arguments[i];
    }
    if (wanted[0] != mesh.integers.size() || wanted[1] != mesh.floats.size() || wanted[2] != mesh.strings.size()) {
        return operands_->fail("a subdivision mesh's tags take more or fewer arguments than it gives", at);
    }

    ParameterList list;
    if (!operands_->parameters(lexer, 0, &list)) {
        return false;
    }
    handler->subdivisionMesh(mesh, list);
    return true;
}

bool Primitives::volume(Lexer * lexer, Handler * handler) {
    requests::Volume request;
    std::vector<float> bound;
    std::vector<unsigned int> voxels;
    const Token at = lexer->peek();
    if (!operands_->text(lexer, &request.type) || !operands_->numbers(lexer, static_cast<unsigned int>(request.bound.size()), &bound) || !operands_->counts(lexer, &voxels)) {
        return false;
    }
    if (voxels.size() != request.voxels.size()) {
        return operands_->fail("a volume gives three voxel counts", at);
    }
    std::copy(bound.begin(), bound.end(), request.bound.begin());
    std::copy(voxels.begin(), voxels.end(), request.voxels.begin());
    // a varying value per voxel, unless there are too many to count, when no length is checked
    const std::uint64_t each = static_cast<std::uint64_t>(voxels[0]) * voxels[1] * voxels[2];
    const unsigned int count = each <= std::numeric_limits<unsigned int>::max() ? static_cast<unsigned int>(each) : 0U;
    ParameterList list;
    if (!operands_->parameters(lexer, count, &list)) {
        return false;
    }
    handler->volume(request, list);
    return true;
}

bool Primitives::quadric(const std::string & name, Lexer * lexer, Handler * handler) {
    std::vector<float> values;
    ParameterList list;
    if (!operands_->numbers(lexer, quadricArguments(name), &values) || !operands_->parameters(lexer, kQuadricCorners, &list)) {
        return false;
    }
    if (name == "Sphere") {
        handler->sphere({.radius = values[0], .zmin = values[1], .zmax = values[2], .thetamax = values[3]}, list);
    } else if (name == "Cone") {
        handler->cone({.height = values[0], .radius = values[1], .thetamax = values[2]}, list);
    } else if (name == "Cylinder") {
        handler->cylinder({.radius = values[0], .zmin = values[1], .zmax = values[2], .thetamax = values[3]}, list);
    } else if (name == "Hyperboloid") {
        // two points of three numbers each, then the sweep
        requests::Hyperboloid hyperboloid { .thetamax = values.back() };
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
    return true;
}

};  // namespace v3d::rib::internal
