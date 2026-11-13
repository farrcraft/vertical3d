/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/scene/Kinds.h>

#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/scene/Kind.h>
#include <moya/libmoya/scene/Schema.h>
#include <moya/libmoya/scene/Value.h>

#include <cstdint>
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

/**
 * The integrator's depth limits, as Cycles sets them: twelve bounces in all and of each kind but
 * four diffuse and four glossy, and Russian roulette from the third.
 **/
constexpr int kMaxDepth = 12;
constexpr int kDiffuseDepth = 4;
constexpr int kGlossyDepth = 4;
constexpr int kRouletteDepth = 3;

/** The most kilobytes of texture tiles a render holds, as RI's texturememory gives it: 1 GiB. **/
constexpr int kTextureMemory = 1048576;

/** A film channel's floats, from 1 to 4, and the three of a colour. **/
constexpr int kColourComponents = 3;

/** A full turn, in degrees, as RI gives thetamax and phimax. **/
constexpr float kTurn = 360.0F;

/** A torus's minor radius until a client sets one. RI gives no default. **/
constexpr float kMinorRadius = 0.25F;

/**
 * UsdLux's defaults: a light's radius, a distant light's angle in degrees, and a cone that shapes
 * nothing.
 **/
constexpr float kLightRadius = 0.5F;
constexpr float kSunAngle = 0.53F;
constexpr float kOpenCone = 90.0F;

/** UsdLux's colour temperature until a client sets one, in kelvin. **/
constexpr float kWhiteTemperature = 6500.0F;

/** The samples a pixel takes before adaptive sampling first checks its noise: one pass. **/
constexpr std::int32_t kMinSamples = 16;

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
            Declaration { .name = "noiseThreshold", .type = Type::Float, .fallback = Value::number(0.0F) },
            Declaration { .name = "minSamples", .type = Type::Integer, .fallback = Value::integer(kMinSamples) },
            Declaration { .name = "filter",
                .type = Type::String,
                .fallback = Value::string("gaussian"),
                .choices = { "box", "triangle", "catmull-rom", "gaussian", "sinc" } },
            Declaration { .name = "filterWidth", .type = Type::Float, .count = 2, .fallback = Value::floats({ kFilterWidth, kFilterWidth }) },
            Declaration { .name = "frame", .type = Type::Integer, .fallback = Value::integer(0) },
            Declaration { .name = "camera", .type = Type::Handle, .fallback = Value::handle({}), .targets = { "camera" } },
            Declaration { .name = "edgeLength", .type = Type::Float, .fallback = Value::number(1.0F) },
            Declaration { .name = "dicingCamera", .type = Type::Handle, .fallback = Value::handle({}), .targets = { "camera" } },
            Declaration { .name = "maxDepth", .type = Type::Integer, .fallback = Value::integer(kMaxDepth) },
            Declaration { .name = "diffuseDepth", .type = Type::Integer, .fallback = Value::integer(kDiffuseDepth) },
            Declaration { .name = "glossyDepth", .type = Type::Integer, .fallback = Value::integer(kGlossyDepth) },
            Declaration { .name = "specularDepth", .type = Type::Integer, .fallback = Value::integer(kMaxDepth) },
            Declaration { .name = "transmissionDepth", .type = Type::Integer, .fallback = Value::integer(kMaxDepth) },
            Declaration { .name = "rouletteDepth", .type = Type::Integer, .fallback = Value::integer(kRouletteDepth) },
            Declaration { .name = "lightSampling", .type = Type::String, .fallback = Value::string("mis"), .choices = { "mis", "light", "bsdf" } },
            Declaration { .name = "lightPicking", .type = Type::String, .fallback = Value::string("tree"), .choices = { "tree", "power" } },
            Declaration { .name = "textureSearchPath", .type = Type::String, .fallback = Value::string("") },
            Declaration { .name = "textureMemory", .type = Type::Integer, .fallback = Value::integer(kTextureMemory) },
            Declaration { .name = "textureDirectory", .type = Type::String, .fallback = Value::string("") } } };
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
                .choices = { "none", "edges", "edges-and-corners" } },
            Declaration { .name = "displacementBound", .type = Type::Float, .fallback = Value::number(0.0F) } },
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
            Declaration { .name = "constantWidth", .type = Type::Float, .fallback = Value::number(1.0F) },
            Declaration { .name = "profile", .type = Type::String, .fallback = Value::string("round"), .choices = { "flat", "round" } } },
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
            Declaration { .name = "material", .type = Type::Handle, .fallback = Value::handle({}), .targets = { "material" } },
            Declaration { .name = "camera", .type = Type::Integer, .fallback = Value::integer(1) },
            Declaration { .name = "shadow", .type = Type::Integer, .fallback = Value::integer(1) },
            Declaration { .name = "indirect", .type = Type::Integer, .fallback = Value::integer(1) } } };
}

/**
 * A node of a material's network: the shader it runs, the language the shader is written in, and
 * the inputs its client declares on it, each as a primvar of constant interpolation.
 **/
Kind shader() {
    return Kind { .name = "shader",
        .parameters = {
            Declaration { .name = "shader", .type = Type::String, .fallback = Value::string("") },
            Declaration { .name = "language", .type = Type::String, .fallback = Value::string("materialx"), .choices = { "materialx", "osl", "ri" } } },
        .primvars = true };
}

/** One node's output feeding another node's input. **/
Kind connection() {
    return Kind { .name = "connection",
        .parameters = {
            Declaration { .name = "from", .type = Type::Handle, .fallback = Value::handle({}), .targets = { "shader" } },
            Declaration { .name = "output", .type = Type::String, .fallback = Value::string("out") },
            Declaration { .name = "to", .type = Type::Handle, .fallback = Value::handle({}), .targets = { "shader" } },
            Declaration { .name = "input", .type = Type::String, .fallback = Value::string("") } } };
}

/**
 * A network's terminals: the node and output that give its surface, and those that give its
 * displacement.
 **/
Kind material() {
    return Kind { .name = "material",
        .parameters = {
            Declaration { .name = "surface", .type = Type::Handle, .fallback = Value::handle({}), .targets = { "shader" } },
            Declaration { .name = "surfaceOutput", .type = Type::String, .fallback = Value::string("out") },
            Declaration { .name = "displacement", .type = Type::Handle, .fallback = Value::handle({}), .targets = { "shader" } },
            Declaration { .name = "displacementOutput", .type = Type::String, .fallback = Value::string("out") } } };
}

/**
 * What every light holds: its placement, its colour and strength, its colour temperature in
 * kelvin while enableColorTemperature is 1, and what it lights. As in UsdLux's light linking, it
 * lights every instance while includeRoot is 1, and otherwise only the instances it names, which
 * may be none. As in UsdLux's shadow linking, every instance casts its shadows while
 * shadowIncludeRoot is 1, and otherwise only the instances shadowInstances names.
 **/
std::vector<Declaration> light() {
    return {
        Declaration { .name = "transform", .type = Type::Matrix, .motion = true, .fallback = Value::matrix(glm::mat4(1.0F)) },
        Declaration { .name = "colour", .type = Type::Colour, .fallback = Value::colour(glm::vec3(1.0F)) },
        Declaration { .name = "intensity", .type = Type::Float, .fallback = Value::number(1.0F) },
        Declaration { .name = "exposure", .type = Type::Float, .fallback = Value::number(0.0F) },
        Declaration { .name = "normalize", .type = Type::Integer, .fallback = Value::integer(0) },
        Declaration { .name = "enableColorTemperature", .type = Type::Integer, .fallback = Value::integer(0) },
        Declaration { .name = "colorTemperature", .type = Type::Float, .fallback = Value::number(kWhiteTemperature) },
        Declaration { .name = "includeRoot", .type = Type::Integer, .fallback = Value::integer(1) },
        Declaration { .name = "instances", .type = Type::Handle, .count = Declaration::kAny, .fallback = Value::handles({}), .targets = { "instance" } },
        Declaration { .name = "shadowIncludeRoot", .type = Type::Integer, .fallback = Value::integer(1) },
        Declaration {
            .name = "shadowInstances", .type = Type::Handle, .count = Declaration::kAny, .fallback = Value::handles({}), .targets = { "instance" } } };
}

/**
 * A light with an area, which a cone and an IES profile may shape, as UsdLux's shaping does. The
 * profile is the one iesFile names, with UsdLux's ies:angleScale as iesAngleScale and its
 * ies:normalize as iesNormalize. Camera rays see it while camera is 1.
 **/
std::vector<Declaration> areaLight(const std::vector<Declaration> & own) {
    std::vector<Declaration> parameters = light();
    parameters.push_back(Declaration { .name = "camera", .type = Type::Integer, .fallback = Value::integer(0) });
    parameters.push_back(Declaration { .name = "coneAngle", .type = Type::Float, .fallback = Value::number(kOpenCone) });
    parameters.push_back(Declaration { .name = "coneSoftness", .type = Type::Float, .fallback = Value::number(0.0F) });
    parameters.push_back(Declaration { .name = "iesFile", .type = Type::String, .fallback = Value::string("") });
    parameters.push_back(Declaration { .name = "iesAngleScale", .type = Type::Float, .fallback = Value::number(0.0F) });
    parameters.push_back(Declaration { .name = "iesNormalize", .type = Type::Integer, .fallback = Value::integer(0) });
    parameters.insert(parameters.end(), own.begin(), own.end());
    return parameters;
}

/** A light at infinity, which no cone shapes. **/
std::vector<Declaration> farLight(const std::vector<Declaration> & own) {
    std::vector<Declaration> parameters = light();
    parameters.insert(parameters.end(), own.begin(), own.end());
    return parameters;
}

Declaration size(const char * name, float fallback) {
    return Declaration { .name = name, .type = Type::Float, .fallback = Value::number(fallback) };
}

Declaration texture() {
    return Declaration { .name = "texture", .type = Type::String, .fallback = Value::string("") };
}

/** What a mesh light may take its shape from: any geometry, but not a group. **/
std::vector<std::string> shapes() {
    std::vector<std::string> kinds = prototypes();
    std::erase(kinds, "group");
    return kinds;
}

/** UsdLux's lights, with its parameters and defaults. **/
std::vector<Kind> lights() {
    return {
        Kind { .name = "rectLight", .parameters = areaLight({ size("width", 1.0F), size("height", 1.0F), texture() }) },
        Kind { .name = "diskLight", .parameters = areaLight({ size("radius", kLightRadius) }) },
        Kind { .name = "sphereLight",
            .parameters = areaLight({ size("radius", kLightRadius),
                Declaration { .name = "treatAsPoint", .type = Type::Integer, .fallback = Value::integer(0) } }) },
        Kind { .name = "cylinderLight", .parameters = areaLight({ size("radius", kLightRadius), size("length", 1.0F) }) },
        Kind { .name = "meshLight",
            .parameters = areaLight({ Declaration { .name = "geometry", .type = Type::Handle, .fallback = Value::handle({}), .targets = shapes() } }) },
        Kind { .name = "distantLight", .parameters = farLight({ size("angle", kSunAngle) }) },
        Kind { .name = "domeLight",
            .parameters = farLight({ texture(),
                Declaration { .name = "format",
                    .type = Type::String,
                    .fallback = Value::string("automatic"),
                    .choices = { "automatic", "latlong", "mirroredBall", "angular", "cubeMapVerticalCross" } } }) } };
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
    schema.add(shader());
    schema.add(connection());
    schema.add(material());
    schema.add(instance());
    for (const Kind & each : lights()) {
        schema.add(each);
    }
    return schema;
}

};  // namespace v3d::moya::scene
