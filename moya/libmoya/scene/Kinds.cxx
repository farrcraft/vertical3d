/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/scene/Kinds.h>

#include <moya/libmoya/scene/Schema.h>
#include <moya/libmoya/scene/Value.h>

#include <limits>
#include <string>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

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

/** A full turn, in degrees, as RI gives thetamax and phimax. **/
constexpr float kTurn = 360.0F;

/** A torus's minor radius until a client sets one. RI gives no default. **/
constexpr float kMinorRadius = 0.25F;

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
            Declaration { .name = "camera", .type = Type::Handle, .fallback = Value::handle({}), .targets = { "camera" } } } };
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
            Declaration { .name = "channels", .type = Type::Handle, .count = Declaration::kAny, .fallback = Value::handles({}), .targets = { "channel" } },
            Declaration { .name = "precision", .type = Type::String, .fallback = Value::string("half"), .choices = { "half", "float" } },
            Declaration { .name = "compression",
                .type = Type::String,
                .fallback = Value::string("zip"),
                .choices = { "none", "rle", "zips", "zip", "piz", "pxr24", "b44", "b44a", "dwaa", "dwab", "htj2k256", "htj2k32" } } } };
}

/** Points over the shutter, and the faces or curves they make. **/
Declaration points() {
    return Declaration { .name = "P", .type = Type::Point, .count = Declaration::kAny, .motion = true, .fallback = Value::points({}) };
}

Declaration integers(const char * name) {
    return Declaration { .name = name, .type = Type::Integer, .count = Declaration::kAny, .fallback = Value::integers({}) };
}

Declaration floats(const char * name) {
    return Declaration { .name = name, .type = Type::Float, .count = Declaration::kAny, .fallback = Value::floats({}) };
}

/** A float that may move over the shutter, as a quadric's dimensions may. **/
Declaration moving(const char * name, float fallback) {
    return Declaration { .name = name, .type = Type::Float, .motion = true, .fallback = Value::number(fallback) };
}

Kind mesh() {
    return Kind { .name = "mesh", .parameters = { points(), integers("vertices"), integers("indices") }, .primvars = true };
}

Kind subdivision() {
    return Kind { .name = "subdivision",
        .parameters = {
            points(),
            integers("vertices"),
            integers("indices"),
            Declaration { .name = "scheme",
                .type = Type::String,
                .fallback = Value::string("catmull-clark"),
                .choices = { "catmull-clark", "loop", "bilinear" } },
            integers("creases"),
            integers("creaseLengths"),
            floats("creaseSharpness"),
            integers("corners"),
            floats("cornerSharpness"),
            integers("holes"),
            Declaration { .name = "boundary",
                .type = Type::String,
                .fallback = Value::string("none"),
                .choices = { "none", "edges", "edges-and-corners" } } },
        .primvars = true };
}

Kind curves() {
    return Kind { .name = "curves",
        .parameters = {
            points(),
            integers("vertices"),
            Declaration { .name = "type", .type = Type::String, .fallback = Value::string("linear"), .choices = { "linear", "cubic" } },
            Declaration { .name = "basis",
                .type = Type::String,
                .fallback = Value::string("bezier"),
                .choices = { "bezier", "b-spline", "catmull-rom", "hermite", "power" } },
            Declaration { .name = "wrap", .type = Type::String, .fallback = Value::string("nonperiodic"), .choices = { "nonperiodic", "periodic" } },
            Declaration { .name = "width",
                .type = Type::Float,
                .count = Declaration::kAny,
                .motion = true,
                .fallback = Value::floats({}),
                .interpolation = Interpolation::Varying },
            Declaration { .name = "constantWidth", .type = Type::Float, .fallback = Value::number(1.0F) },
            Declaration { .name = "profile", .type = Type::String, .fallback = Value::string("flat"), .choices = { "flat", "round" } } },
        .primvars = true };
}

Kind pointCloud() {
    return Kind { .name = "points",
        .parameters = {
            points(),
            Declaration { .name = "width",
                .type = Type::Float,
                .count = Declaration::kAny,
                .motion = true,
                .fallback = Value::floats({}),
                .interpolation = Interpolation::Varying },
            Declaration { .name = "constantWidth", .type = Type::Float, .fallback = Value::number(1.0F) } },
        .primvars = true };
}

/** The seven RI quadrics. Each sweeps its profile about z up to thetamax degrees. **/
std::vector<Kind> quadrics() {
    const Declaration sweep = moving("thetamax", kTurn);
    return {
        Kind { .name = "sphere", .parameters = { moving("radius", 1.0F), moving("zmin", -1.0F), moving("zmax", 1.0F), sweep }, .primvars = true },
        Kind { .name = "cone", .parameters = { moving("height", 1.0F), moving("radius", 1.0F), sweep }, .primvars = true },
        Kind { .name = "cylinder", .parameters = { moving("radius", 1.0F), moving("zmin", -1.0F), moving("zmax", 1.0F), sweep }, .primvars = true },
        Kind { .name = "hyperboloid",
            .parameters = {
                Declaration { .name = "point1", .type = Type::Point, .motion = true, .fallback = Value::point(glm::vec3(1.0F, 0.0F, -1.0F)) },
                Declaration { .name = "point2", .type = Type::Point, .motion = true, .fallback = Value::point(glm::vec3(1.0F, 0.0F, 1.0F)) },
                sweep },
            .primvars = true },
        Kind { .name = "paraboloid", .parameters = { moving("rmax", 1.0F), moving("zmin", 0.0F), moving("zmax", 1.0F), sweep }, .primvars = true },
        Kind { .name = "disk", .parameters = { moving("height", 0.0F), moving("radius", 1.0F), sweep }, .primvars = true },
        Kind { .name = "torus",
            .parameters = { moving("majorRadius", 1.0F), moving("minorRadius", kMinorRadius), moving("phimin", 0.0F), moving("phimax", kTurn), sweep },
            .primvars = true } };
}

/** A prototype made of instances. Each instance names the group it belongs to. **/
Kind group() {
    return Kind { .name = "group" };
}

/** What an instance may place. **/
const std::vector<std::string> & prototypes() {
    static const std::vector<std::string> kinds = { "mesh", "subdivision", "curves", "points", "sphere", "cone", "cylinder", "hyperboloid",
        "paraboloid", "disk", "torus", "group" };
    return kinds;
}

Kind instance() {
    return Kind { .name = "instance",
        .parameters = {
            Declaration { .name = "prototype", .type = Type::Handle, .fallback = Value::handle({}), .targets = prototypes() },
            Declaration { .name = "group", .type = Type::Handle, .fallback = Value::handle({}), .targets = { "group" } },
            Declaration { .name = "transform", .type = Type::Matrix, .motion = true, .fallback = Value::matrix(glm::mat4(1.0F)) },
            Declaration { .name = "camera", .type = Type::Integer, .fallback = Value::integer(1) },
            Declaration { .name = "shadow", .type = Type::Integer, .fallback = Value::integer(1) },
            Declaration { .name = "indirect", .type = Type::Integer, .fallback = Value::integer(1) } } };
}

};  // namespace

Schema kinds() {
    Schema schema;
    schema.add(camera());
    schema.add(options());
    schema.add(channel());
    schema.add(output());
    schema.add(mesh());
    schema.add(subdivision());
    schema.add(curves());
    schema.add(pointCloud());
    for (const Kind & quadric : quadrics()) {
        schema.add(quadric);
    }
    schema.add(group());
    schema.add(instance());
    return schema;
}

};  // namespace v3d::moya::scene
