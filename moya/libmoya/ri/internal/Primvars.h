/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/render/offline/rib/Declaration.h>
#include <api/render/offline/rib/Parameters.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Schema.h>
#include <moya/libmoya/scene/Value.h>

#include <optional>
#include <set>
#include <string>

namespace v3d::moya::ri::internal {

/**
 * A RIB parameter's values as a scene value of its declared type. An integer arrives as floats
 * and loses any fraction, and one beyond two billion either way drops the parameter. An hpoint
 * is divided by its w. A matrix's sixteen floats are read in RIB's order, as the reader reads a
 * transformation.
 *
 * @return nothing when the list holds no such parameter
 **/
std::optional<scene::Value> valueOf(const v3d::render::offline::rib::ParameterList & parameters, const std::string & name);

/** The scene's interpolation class for a RIB storage class. **/
scene::Interpolation interpolationOf(v3d::render::offline::rib::Declaration::Storage storage);

/**
 * Declare and set each parameter of a list as a primvar of an object, with its RIB storage
 * class, except the names given.
 *
 * @param constant declare every one as constant, as a shader node's inputs are, rather than by
 *        its storage class
 **/
void declare(scene::Scene * scene, const scene::Handle & object, const v3d::render::offline::rib::ParameterList & parameters,
    const std::set<std::string> & except, bool constant);

};  // namespace v3d::moya::ri::internal
