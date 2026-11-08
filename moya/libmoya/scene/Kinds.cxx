/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/scene/Kinds.h>

#include <moya/libmoya/scene/Schema.h>
#include <moya/libmoya/scene/Value.h>

#include <limits>

#include <glm/mat4x4.hpp>

namespace v3d::moya::scene {

namespace {

/** RI's defaults, as CameraOptions states them. **/
constexpr int kWidth = 320;
constexpr int kHeight = 240;
constexpr float kFov = 90.0F;
constexpr float kHither = 1.0e-10F;
constexpr float kYon = 1.0e38F;

/** RI's PixelSamples of 2 by 2, and its Gaussian filter 2 pixels wide. **/
constexpr int kSamples = 4;
constexpr float kFilterWidth = 2.0F;

/** A film channel's floats, from 1 to 4, and the three of a colour. **/
constexpr int kColourComponents = 3;

Kind camera() {
    return Kind { .name = "camera",
        .parameters = {
            Declaration { .name = "transform", .type = Type::Matrix, .motion = true, .fallback = Value::matrix(glm::mat4(1.0F)) },
            Declaration { .name = "projection",
                .type = Type::String,
                .fallback = Value::string("orthographic"),
                .choices = { "orthographic", "perspective" } },
            Declaration { .name = "fov", .type = Type::Float, .fallback = Value::number(kFov) },
            Declaration { .name = "screen", .type = Type::Float, .count = Declaration::kAny, .fallback = Value::floats({}) },
            Declaration { .name = "near", .type = Type::Float, .fallback = Value::number(kHither) },
            Declaration { .name = "far", .type = Type::Float, .fallback = Value::number(kYon) },
            Declaration { .name = "fstop", .type = Type::Float, .fallback = Value::number(std::numeric_limits<float>::infinity()) },
            Declaration { .name = "focalLength", .type = Type::Float, .fallback = Value::number(0.0F) },
            Declaration { .name = "focalDistance", .type = Type::Float, .fallback = Value::number(0.0F) },
            Declaration { .name = "shutter", .type = Type::Float, .count = 2, .fallback = Value::floats({ 0.0F, 0.0F }) } } };
}

Kind options() {
    return Kind { .name = "options",
        .parameters = {
            Declaration { .name = "resolution", .type = Type::Integer, .count = 2, .fallback = Value::integers({ kWidth, kHeight }) },
            Declaration { .name = "pixelAspect", .type = Type::Float, .fallback = Value::number(1.0F) },
            Declaration { .name = "frameAspect", .type = Type::Float, .fallback = Value::number(0.0F) },
            Declaration { .name = "crop", .type = Type::Float, .count = 4, .fallback = Value::floats({ 0.0F, 1.0F, 0.0F, 1.0F }) },
            Declaration { .name = "samples", .type = Type::Integer, .fallback = Value::integer(kSamples) },
            Declaration { .name = "filter",
                .type = Type::String,
                .fallback = Value::string("gaussian"),
                .choices = { "box", "triangle", "catmull-rom", "gaussian", "sinc" } },
            Declaration { .name = "filterWidth", .type = Type::Float, .count = 2, .fallback = Value::floats({ kFilterWidth, kFilterWidth }) },
            Declaration { .name = "frame", .type = Type::Integer, .fallback = Value::integer(0) },
            Declaration { .name = "camera", .type = Type::Handle, .fallback = Value::handle({}), .target = "camera" } } };
}

Kind channel() {
    return Kind { .name = "channel",
        .parameters = {
            Declaration { .name = "source", .type = Type::String, .fallback = Value::string("") },
            Declaration { .name = "components", .type = Type::Integer, .fallback = Value::integer(kColourComponents) } } };
}

Kind output() {
    return Kind { .name = "output",
        .parameters = {
            Declaration { .name = "file", .type = Type::String, .fallback = Value::string("") },
            Declaration { .name = "format", .type = Type::String, .fallback = Value::string("exr"), .choices = { "exr", "png", "framebuffer" } },
            Declaration { .name = "channels", .type = Type::Handle, .count = Declaration::kAny, .fallback = Value::handles({}), .target = "channel" },
            Declaration { .name = "precision", .type = Type::String, .fallback = Value::string("half"), .choices = { "half", "float" } },
            Declaration { .name = "compression",
                .type = Type::String,
                .fallback = Value::string("zip"),
                .choices = { "none", "rle", "zips", "zip", "piz", "pxr24", "b44", "b44a", "dwaa", "dwab", "htj2k256", "htj2k32" } } } };
}

};  // namespace

Schema kinds() {
    Schema schema;
    schema.add(camera());
    schema.add(options());
    schema.add(channel());
    schema.add(output());
    return schema;
}

};  // namespace v3d::moya::scene
