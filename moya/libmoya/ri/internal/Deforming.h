/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/log/Logger.h>
#include <api/render/offline/rib/Handler.h>
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
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>

#include <map>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include "Shape.h"
#include "Warnings.h"

namespace v3d::moya::ri::internal {

/**
 * A deforming primitive: one repeated inside a motion block. The first pose makes a prototype,
 * and begin() is given it. This handler takes each pose after the first, and keeps the values of
 * the parameters its kind declares for motion. end() sets them as samples of the prototype, one
 * at each of the block's times.
 **/
class Deforming final : public v3d::render::offline::rib::Handler {
 public:
    /**
     * The scene and the warnings outlive the handler.
     *
     * @param basis the basis the current attribute state gives a cubic curve, which outlives the
     *        handler
     **/
    Deforming(scene::Scene * scene, Warnings * warnings, const boost::shared_ptr<v3d::log::Logger> & logger, const std::string & basis);

    /** Forget any primitive, at the start of a motion block. **/
    void clear();

    /** Take the prototype a motion block's first pose made, and that pose's shape. **/
    void begin(const scene::Handle & prototype, const Shape & shape);

    /**
     * Set the poses as samples of the prototype, at the block's times, and forget the primitive.
     * Poses that differ in their points, or times that are not in order, are logged, and the
     * prototype keeps its first pose.
     **/
    void end(const std::vector<float> & times);

    void polygon(unsigned int vertices, const v3d::render::offline::rib::ParameterList & parameters) override;
    void pointsPolygons(const v3d::render::offline::rib::requests::PointsPolygons & request, const v3d::render::offline::rib::ParameterList & parameters) override;
    void sphere(const v3d::render::offline::rib::requests::Sphere & request, const v3d::render::offline::rib::ParameterList & parameters) override;
    void curves(const v3d::render::offline::rib::requests::Curves & request, const v3d::render::offline::rib::ParameterList & parameters) override;
    void points(unsigned int count, const v3d::render::offline::rib::ParameterList & parameters) override;
    void cone(const v3d::render::offline::rib::requests::Cone & request, const v3d::render::offline::rib::ParameterList & parameters) override;
    void cylinder(const v3d::render::offline::rib::requests::Cylinder & request, const v3d::render::offline::rib::ParameterList & parameters) override;
    void hyperboloid(const v3d::render::offline::rib::requests::Hyperboloid & request, const v3d::render::offline::rib::ParameterList & parameters) override;
    void paraboloid(const v3d::render::offline::rib::requests::Paraboloid & request, const v3d::render::offline::rib::ParameterList & parameters) override;
    void disk(const v3d::render::offline::rib::requests::Disk & request, const v3d::render::offline::rib::ParameterList & parameters) override;
    void torus(const v3d::render::offline::rib::requests::Torus & request, const v3d::render::offline::rib::ParameterList & parameters) override;
    void subdivisionMesh(const v3d::render::offline::rib::requests::SubdivisionMesh & request, const v3d::render::offline::rib::ParameterList & parameters) override;

 private:
    /** Keep a later pose's values. A pose of another kind of primitive is logged and left out. **/
    void add(const Shape & shape);

    scene::Scene * scene_;
    Warnings * warnings_;
    boost::shared_ptr<v3d::log::Logger> logger_;
    const std::string & basis_;

    /** The prototype the first pose made, and the values of each pose. **/
    scene::Handle prototype_;
    std::map<std::string, std::vector<scene::Value>> samples_;
};

};  // namespace v3d::moya::ri::internal
