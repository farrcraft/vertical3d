/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include "Deforming.h"

#include <api/log/Logger.h>
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
#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Kind.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Schema.h>
#include <moya/libmoya/scene/Value.h>

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include "Shape.h"
#include "Shapes.h"
#include "Warnings.h"

namespace v3d::moya::ri::internal {

namespace {

typedef v3d::render::offline::rib::ParameterList ParameterList;

};  // namespace

Deforming::Deforming(scene::Scene * scene, Warnings * warnings, const boost::shared_ptr<v3d::log::Logger> & logger, const std::string & basis) :
    scene_(scene), warnings_(warnings), logger_(logger), basis_(basis) {
}

void Deforming::clear() {
    prototype_ = scene::Handle {};
    samples_.clear();
}

void Deforming::begin(const scene::Handle & prototype, const Shape & shape) {
    prototype_ = prototype;
    samples_.clear();
    const scene::Kind & kind = scene_->schema().kind(prototype.kind);
    for (const auto & [name, value] : shape.parameters) {
        const auto declared = std::find_if(kind.parameters.begin(), kind.parameters.end(),
            [&name](const scene::Declaration & each) { return each.name == name; });
        if (declared != kind.parameters.end() && declared->motion) {
            samples_[name].push_back(value);
        }
    }
}

void Deforming::end(const std::vector<float> & times) {
    if (!prototype_.valid()) {
        return;
    }
    for (const auto & [name, values] : samples_) {
        const std::size_t count = std::min(values.size(), times.size());
        if (count < 2) {
            continue;
        }
        try {
            scene_->set(prototype_, name, std::vector<float>(times.begin(), times.begin() + static_cast<std::ptrdiff_t>(count)),
                std::vector<scene::Value>(values.begin(), values.begin() + static_cast<std::ptrdiff_t>(count)));
        } catch (const std::invalid_argument & error) {
            logger_->get()->warn("a deforming primitive's poses differ in their points or its times are not in order, so it keeps its first pose - {}",
                error.what());
        }
    }
    clear();
}

void Deforming::add(const Shape & shape) {
    if (!prototype_.valid() || shape.kind != scene_->schema().kind(prototype_.kind).name) {
        warnings_->once("deform " + shape.kind, "a pose of a different primitive inside a motion block is left out");
        return;
    }
    for (const auto & [name, value] : shape.parameters) {
        const auto found = samples_.find(name);
        if (found != samples_.end()) {
            found->second.push_back(value);
        }
    }
}

void Deforming::polygon(unsigned int vertices, const ParameterList & parameters) {
    add(Shapes::polygon(vertices, parameters));
}

void Deforming::pointsPolygons(const v3d::render::offline::rib::requests::PointsPolygons & request, const ParameterList & parameters) {
    add(Shapes::mesh(request, parameters));
}

void Deforming::sphere(const v3d::render::offline::rib::requests::Sphere & request, const ParameterList & parameters) {
    add(Shapes::sphere(request, parameters));
}

void Deforming::curves(const v3d::render::offline::rib::requests::Curves & request, const ParameterList & parameters) {
    add(Shapes::curves(request, parameters, basis_));
}

void Deforming::points(unsigned int count, const ParameterList & parameters) {
    (void)count;
    add(Shapes::points(parameters));
}

void Deforming::cone(const v3d::render::offline::rib::requests::Cone & request, const ParameterList & parameters) {
    add(Shapes::cone(request, parameters));
}

void Deforming::cylinder(const v3d::render::offline::rib::requests::Cylinder & request, const ParameterList & parameters) {
    add(Shapes::cylinder(request, parameters));
}

void Deforming::hyperboloid(const v3d::render::offline::rib::requests::Hyperboloid & request, const ParameterList & parameters) {
    add(Shapes::hyperboloid(request, parameters));
}

void Deforming::paraboloid(const v3d::render::offline::rib::requests::Paraboloid & request, const ParameterList & parameters) {
    add(Shapes::paraboloid(request, parameters));
}

void Deforming::disk(const v3d::render::offline::rib::requests::Disk & request, const ParameterList & parameters) {
    add(Shapes::disk(request, parameters));
}

void Deforming::torus(const v3d::render::offline::rib::requests::Torus & request, const ParameterList & parameters) {
    add(Shapes::torus(request, parameters));
}

void Deforming::subdivisionMesh(const v3d::render::offline::rib::requests::SubdivisionMesh & request, const ParameterList & parameters) {
    add(Shapes::subdivision(request, parameters, warnings_));
}

};  // namespace v3d::moya::ri::internal
