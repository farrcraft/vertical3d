/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

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
#include <moya/libmoya/scene/Value.h>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "Shape.h"
#include "Warnings.h"

namespace v3d::moya::ri::internal {

/**
 * The shape of each RI primitive request: the scene kind it becomes, and its parameters as that
 * kind names them. A request's points are its `"P"`, or its `"Pw"` divided out.
 **/
class Shapes final {
 public:
    static Shape polygon(unsigned int vertices, const v3d::render::offline::rib::ParameterList & parameters);
    static Shape mesh(const v3d::render::offline::rib::requests::PointsPolygons & request, const v3d::render::offline::rib::ParameterList & parameters);
    /**
     * A curves primitive, with its `"width"` or `"constantwidth"`.
     *
     * @param basis the basis the attribute state gives a cubic curve
     **/
    static Shape curves(const v3d::render::offline::rib::requests::Curves & request, const v3d::render::offline::rib::ParameterList & parameters,
        const std::string & basis);
    /** A points primitive, with its `"width"` or `"constantwidth"`. **/
    static Shape points(const v3d::render::offline::rib::ParameterList & parameters);
    /**
     * A subdivision mesh. A scheme moya does not subdivide by is logged, and catmull-clark is
     * used. A tag RI names, `crease`, `corner`, `hole` or `interpolateboundary`, becomes the
     * mesh's parameters. Another tag is logged.
     **/
    static Shape subdivision(const v3d::render::offline::rib::requests::SubdivisionMesh & request, const v3d::render::offline::rib::ParameterList & parameters,
        Warnings * warnings);

    static Shape sphere(const v3d::render::offline::rib::requests::Sphere & request, const v3d::render::offline::rib::ParameterList & parameters);
    static Shape cone(const v3d::render::offline::rib::requests::Cone & request, const v3d::render::offline::rib::ParameterList & parameters);
    static Shape cylinder(const v3d::render::offline::rib::requests::Cylinder & request, const v3d::render::offline::rib::ParameterList & parameters);
    static Shape hyperboloid(const v3d::render::offline::rib::requests::Hyperboloid & request, const v3d::render::offline::rib::ParameterList & parameters);
    static Shape paraboloid(const v3d::render::offline::rib::requests::Paraboloid & request, const v3d::render::offline::rib::ParameterList & parameters);
    static Shape disk(const v3d::render::offline::rib::requests::Disk & request, const v3d::render::offline::rib::ParameterList & parameters);
    static Shape torus(const v3d::render::offline::rib::requests::Torus & request, const v3d::render::offline::rib::ParameterList & parameters);

 private:
    /** A subdivision mesh's creases, corners, holes and boundary rule, as its tags give them. **/
    struct Edges {
        std::vector<std::int32_t> creases{};
        std::vector<std::int32_t> creaseLengths{};
        std::vector<float> creaseSharpness{};
        std::vector<std::int32_t> corners{};
        std::vector<float> cornerSharpness{};
        std::vector<std::int32_t> holes{};
        std::string boundary { "none" };
    };

    /**
     * RenderMan's interpolateboundary rule: 0 for none, 2 for the edges alone, and any other for
     * the edges and the corners.
     **/
    static std::string boundaryOf(unsigned int rule);

    /**
     * Add one tag's integers and floats to a mesh's edges.
     *
     * @return false for a tag RI does not name
     **/
    static bool addTag(const std::string & name, const std::vector<unsigned int> & given, const std::vector<float> & sharpness, Edges * edges);

    /** The points of a request: its "P", or its "Pw" divided out. **/
    static scene::Value pointsOf(const v3d::render::offline::rib::ParameterList & parameters);

    /** Counts as the scene's integers. **/
    static scene::Value integersOf(const std::vector<unsigned int> & counts);

    /** Add a request's `"width"` and `"constantwidth"` to a shape, as the kind names them. **/
    static void widths(const v3d::render::offline::rib::ParameterList & parameters, Shape * shape);

    /** A quadric, whose parameters are its request's dimensions alone. **/
    static Shape quadric(const std::string & kind, const std::string & request, const v3d::render::offline::rib::ParameterList & parameters,
        const std::vector<std::pair<std::string, scene::Value>> & dimensions);
};

};  // namespace v3d::moya::ri::internal
