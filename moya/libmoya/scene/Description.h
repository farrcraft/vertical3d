/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/scene/Scene.h>

#include <string>

namespace v3d::moya::scene {

/**
 * A scene as text: every object the applied batches hold, with every parameter and primvar and
 * its value. Kinds are in name order, the objects of each kind in name order, and an object's
 * parameters in name order, so two scenes holding the same objects describe alike whatever order
 * they were made in.
 *
 * Each object is a line of its kind and its quoted name, then a line per parameter, indented
 * four spaces. A parameter's line gives its name, its type, its element count in brackets when it
 * is an array, and a primvar's interpolation class unless it is constant. Then it gives its value
 * after `=`, or each sample's time and value after `@`. A float is written to six significant
 * digits, and a negative zero as zero, so a description is the same under every compiler's
 * mathematics. A colour, point, vector or normal is three floats in parentheses, and a colour
 * names its space after it when it has one. A matrix is sixteen floats in square brackets,
 * column by column. A handle is its object's kind and quoted name, or `none` when the scene
 * holds no object it names.
 **/
std::string describe(const Scene & scene);

};  // namespace v3d::moya::scene
