/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "RenderMan.h"
#include "RIBHandler.h"
#include "Renderer.h"

#include <api/render/offline/film/Sampling.h>
#include <api/render/offline/rib/Requests.h>
#include <moya/libmoya/ri/internal/Converter.h>

#include <stdarg.h>

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

/*
    Each entry point turns its arguments into the request rib::Handler takes, and passes it to
    the active handler. The handler applies RI's rules, the same ones it applies to a file. An
    entry point converts its arguments with Converter and does nothing more.
*/

using v3d::moya::ri::internal::Converter;

namespace {

// the handlers the C interface drives. The stack is built on first use, inside the RI call
// that reaches it, so an exception from its constructor reaches the caller. An exception from
// a static initialiser reaches no handler at all.
v3d::moya::ri::Renderer& renderer() {
    static v3d::moya::ri::Renderer instance;  // process-wide: the RenderMan C interface drives one implicit renderer
    return instance;
}

v3d::moya::ri::RIBHandler& handler() {
    return renderer().active();
}

/** The conversions of an entry point's arguments, reading the active handler. **/
Converter converter() {
    return Converter(renderer());
}

/**
 * A light handle a C caller can hold on to.
 *
 * RtLightHandle is an untyped pointer, so the characters it points at must outlive the call
 * that made it. A caller may pass one back to RiIlluminate at any later point. The renderer
 * keeps the handles for as long as it runs.
 **/
RtLightHandle keep(const std::string & value) {
    return const_cast<char*>(renderer().keep(value).c_str());
}

/** A light, as an ordinary light or as an area light. **/
RtLightHandle light(RtToken name, RtInt n, RtToken tokens[], RtPointer parms[], bool area) {
    /*
        A C caller has no handle to give, so the interface returns one. The handler keys
        lights on a string, because RIB 3.03 writes a handle as a number and later versions
        write a string. This handle is the count of lights made so far, the same sequence a
        3.03 file writes. A light whose shader cannot be made still has a handle, which turns
        nothing on or off.
    */
    const std::string handle = std::to_string(renderer().nextLight());
    const v3d::render::offline::rib::LightSource request = {.shader = Converter::text(name), .handle = handle};
    if (area) {
        handler().areaLightSource(request, converter().parameters(n, tokens, parms));
    } else {
        handler().lightSource(request, converter().parameters(n, tokens, parms));
    }
    return keep(handle);
}

};  // namespace

// every token holds RI's string for it. The context writes a file only when the display type
// is "file", and RiDisplay passes a null type on as an empty string
RtToken RI_FRAMEBUFFER = const_cast<char*>("framebuffer"),
        RI_FILE = const_cast<char*>("file");
RtToken RI_RGB = const_cast<char*>("rgb"),
        RI_RGBA = const_cast<char*>("rgba"),
        RI_RGBZ = const_cast<char*>("rgbz"),
        RI_RGBAZ = const_cast<char*>("rgbaz"),
        RI_A = const_cast<char*>("a"),
        RI_Z = const_cast<char*>("z"),
        RI_AZ = const_cast<char*>("az");
RtToken RI_PERSPECTIVE = const_cast<char*>("perspective");
RtToken RI_ORTHOGRAPHIC = const_cast<char*>("orthographic");
RtToken RI_HIDDEN = const_cast<char*>("hidden"),
        RI_PAINT = const_cast<char*>("paint");
RtToken RI_CONSTANT = const_cast<char*>("constant"),
        RI_SMOOTH = const_cast<char*>("smooth");
RtToken RI_FLATNESS = const_cast<char*>("flatness");
RtToken RI_FOV = const_cast<char*>("fov");
RtToken RI_AMBIENTLIGHT = const_cast<char*>("ambientlight"),
        RI_POINTLIGHT = const_cast<char*>("pointlight"),
        RI_DISTANTLIGHT = const_cast<char*>("distantlight"),
        RI_SPOTLIGHT = const_cast<char*>("spotlight");
RtToken RI_INTENSITY = const_cast<char*>("intensity"),
        RI_LIGHTCOLOR = const_cast<char*>("lightcolor"),
        RI_FROM = const_cast<char*>("from"),
        RI_TO = const_cast<char*>("to"),
        RI_CONEANGLE = const_cast<char*>("coneangle"),
        RI_CONEDELTAANGLE = const_cast<char*>("conedeltaangle"),
        RI_BEAMDISTRIBUTION = const_cast<char*>("beamdistribution");
RtToken RI_MATTE = const_cast<char*>("matte"),
        RI_METAL = const_cast<char*>("metal"),
        RI_SHINYMETAL = const_cast<char*>("shinymetal"),
        RI_PLASTIC = const_cast<char*>("plastic"),
        RI_PAINTEDPLASTIC = const_cast<char*>("paintedplastic");
RtToken RI_KA = const_cast<char*>("Ka"),
        RI_KD = const_cast<char*>("Kd"),
        RI_KS = const_cast<char*>("Ks"),
        RI_ROUGHNESS = const_cast<char*>("roughness"),
        RI_KR = const_cast<char*>("Kr"),
        RI_TEXTURENAME = const_cast<char*>("texturename"),
        RI_SPECULARCOLOR = const_cast<char*>("specularcolor");
RtToken RI_DEPTHCUE = const_cast<char*>("depthcue"),
        RI_FOG = const_cast<char*>("fog"),
        RI_BUMPY = const_cast<char*>("bumpy");
RtToken RI_MINDISTANCE = const_cast<char*>("mindistance"),
        RI_MAXDISTANCE = const_cast<char*>("maxdistance"),
        RI_BACKGROUND = const_cast<char*>("background"),
        RI_DISTANCE = const_cast<char*>("distance"),
        RI_AMPLITUDE = const_cast<char*>("amplitude");
RtToken RI_RASTER = const_cast<char*>("raster"),
        RI_SCREEN = const_cast<char*>("screen"),
        RI_CAMERA = const_cast<char*>("camera"),
        RI_WORLD = const_cast<char*>("world"),
        RI_OBJECT = const_cast<char*>("object");
RtToken RI_INSIDE = const_cast<char*>("inside"),
        RI_OUTSIDE = const_cast<char*>("outside"),
        RI_LH = const_cast<char*>("lh"),
        RI_RH = const_cast<char*>("rh");
RtToken RI_P = const_cast<char*>("P");
RtToken RI_PZ = const_cast<char*>("Pz");
RtToken RI_PW = const_cast<char*>("Pw");
RtToken RI_N = const_cast<char*>("N");
RtToken RI_NP = const_cast<char*>("Np");
RtToken RI_CS = const_cast<char*>("Cs");
RtToken RI_OS = const_cast<char*>("Os");
RtToken RI_S = const_cast<char*>("s");
RtToken RI_T = const_cast<char*>("t");
RtToken RI_ST = const_cast<char*>("st");
RtToken RI_BILINEAR = const_cast<char*>("bilinear"),
        RI_BICUBIC = const_cast<char*>("bicubic");
RtToken RI_LINEAR = const_cast<char*>("linear"),
        RI_CUBIC = const_cast<char*>("cubic");
RtToken RI_PRIMITIVE = const_cast<char*>("primitive"),
        RI_INTERSECTION = const_cast<char*>("intersection"),
        RI_UNION = const_cast<char*>("union"),
        RI_DIFFERENCE = const_cast<char*>("difference");
RtToken RI_PERIODIC = const_cast<char*>("periodic"),
        RI_NONPERIODIC = const_cast<char*>("nonperiodic"),
        RI_CLAMP = const_cast<char*>("clamp"),
        RI_BLACK = const_cast<char*>("black");
RtToken RI_IGNORE = const_cast<char*>("ignore"),
        RI_PRINT = const_cast<char*>("print"),
        RI_ABORT = const_cast<char*>("abort"),
        RI_HANDLER = const_cast<char*>("handler");
RtToken RI_COMMENT = const_cast<char*>("comment"),
        RI_STRUCTURE = const_cast<char*>("structure"),
        RI_VERBATIM = const_cast<char*>("verbatim");
RtToken RI_IDENTIFIER = const_cast<char*>("identifier"),
        RI_NAME = const_cast<char*>("name"),
        RI_SHADINGGROUP = const_cast<char*>("shadinggroup");
RtToken RI_WIDTH = const_cast<char*>("width"),
        RI_CONSTANTWIDTH = const_cast<char*>("constantwidth");

// each basis is RI's matrix for it, rows first. The entries are the values RI's specification
// gives, so they are not named. A B-spline entry is a sixth, written as the float it rounds to
RtBasis RiBezierBasis = { { -1.0F, 3.0F, -3.0F, 1.0F }, { 3.0F, -6.0F, 3.0F, 0.0F },  // NOLINT(readability-magic-numbers)
    { -3.0F, 3.0F, 0.0F, 0.0F }, { 1.0F, 0.0F, 0.0F, 0.0F } };  // NOLINT(readability-magic-numbers)
RtBasis RiBSplineBasis = { { -0.16666667F, 0.5F, -0.5F, 0.16666667F }, { 0.5F, -1.0F, 0.5F, 0.0F }, { -0.5F, 0.0F, 0.5F, 0.0F },  // NOLINT(readability-magic-numbers)
    { 0.16666667F, 0.6666667F, 0.16666667F, 0.0F } };  // NOLINT(readability-magic-numbers)
RtBasis RiCatmullRomBasis = { { -0.5F, 1.5F, -1.5F, 0.5F }, { 1.0F, -2.5F, 2.0F, -0.5F },  // NOLINT(readability-magic-numbers)
    { -0.5F, 0.0F, 0.5F, 0.0F }, { 0.0F, 1.0F, 0.0F, 0.0F } };
RtBasis RiHermiteBasis = { { 2.0F, 1.0F, -2.0F, 1.0F }, { -3.0F, -2.0F, 3.0F, -1.0F },  // NOLINT(readability-magic-numbers)
    { 0.0F, 1.0F, 0.0F, 0.0F }, { 1.0F, 0.0F, 0.0F, 0.0F } };
RtBasis RiPowerBasis = { { 1.0F, 0.0F, 0.0F, 0.0F }, { 0.0F, 1.0F, 0.0F, 0.0F }, { 0.0F, 0.0F, 1.0F, 0.0F }, { 0.0F, 0.0F, 0.0F, 1.0F } };

RtInt RiLastError;

// RI subroutines. The RenderMan Interface sets every signature below, including the order of
// parameters that share a type, so each that the swappable parameter check reports is marked.
RtFloat RiGaussianFilter(RtFloat x, RtFloat y, RtFloat xwidth, RtFloat ywidth) {
    return v3d::render::offline::film::filter(v3d::render::offline::film::Filter::Gaussian, glm::vec2(x, y), glm::vec2(xwidth, ywidth));
}

RtFloat RiBoxFilter(RtFloat x, RtFloat y, RtFloat xwidth, RtFloat ywidth) {
    return v3d::render::offline::film::filter(v3d::render::offline::film::Filter::Box, glm::vec2(x, y), glm::vec2(xwidth, ywidth));
}

RtFloat RiTriangleFilter(RtFloat x, RtFloat y, RtFloat xwidth, RtFloat ywidth) {
    return v3d::render::offline::film::filter(v3d::render::offline::film::Filter::Triangle, glm::vec2(x, y), glm::vec2(xwidth, ywidth));
}

RtFloat RiCatmullRomFilter(RtFloat x, RtFloat y, RtFloat xwidth, RtFloat ywidth) {
    return v3d::render::offline::film::filter(v3d::render::offline::film::Filter::CatmullRom, glm::vec2(x, y), glm::vec2(xwidth, ywidth));
}

RtFloat RiSincFilter(RtFloat x, RtFloat y, RtFloat xwidth, RtFloat ywidth) {
    return v3d::render::offline::film::filter(v3d::render::offline::film::Filter::Sinc, glm::vec2(x, y), glm::vec2(xwidth, ywidth));
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiErrorIgnore(RtInt code, RtInt severity, char *msg) {
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiErrorPrint(RtInt code, RtInt severity, char *msg) {
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiErrorAbort(RtInt code, RtInt severity, char *msg) {
}

RtVoid RiProcDelayedReadArchive(RtPointer data, RtFloat detail) {
}

RtVoid RiProcRunProgram(RtPointer data, RtFloat detail) {
}

RtVoid RiProcDynamicLoad(RtPointer data, RtFloat detail) {
}

// RiGetContext and RiContext have no RIB equivalents. The handle is the active handler, which
// stays where it is until RiEnd ends it
RtContextHandle RiGetContext(void) {
    return &renderer().active();
}

// a handle to a context that has ended, or was never begun, is ignored
RtVoid RiContext(RtContextHandle handle) {
    renderer().activate(static_cast<const v3d::moya::ri::RIBHandler*>(handle));
}

// RI returns the token the name now stands for. In this renderer the token is the name
RtToken RiDeclare(char * name, char * declaration) {
    handler().declare({.name = Converter::text(name), .type = Converter::text(declaration)});
    return name;
}

// RiBegin pushes a new context and makes it active
RtVoid RiBegin(RtToken name) {
    renderer().begin(Converter::text(name));
}

// RiEnd pops the active context, and the one beneath it becomes active. With no context
// begun it does nothing. A request that arrives with no context active creates one
RtVoid RiEnd(void) {
    renderer().end();
}

RtVoid RiFrameBegin(RtInt frame) {
    handler().frameBegin(frame);
}

RtVoid RiFrameEnd(void) {
    handler().frameEnd();
}

RtVoid RiWorldBegin(void) {
    handler().worldBegin();
}

RtVoid RiWorldEnd(void) {
    handler().worldEnd();
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiFormat(RtInt xres, RtInt yres, RtFloat aspect) {
    if (const std::optional<v3d::render::offline::rib::Format> request = Converter::format(glm::ivec2(xres, yres), aspect)) {
        handler().format(*request);
    }
}

RtVoid RiFrameAspectRatio(RtFloat aspect) {
    handler().frameAspectRatio(aspect);
}

RtVoid RiScreenWindow(RtFloat left, RtFloat right, RtFloat bot, RtFloat top) {
    handler().screenWindow({.left = left, .right = right, .bottom = bot, .top = top});
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiCropWindow(RtFloat xmin, RtFloat xmax, RtFloat ymin, RtFloat ymax) {
    handler().cropWindow({.xmin = xmin, .xmax = xmax, .ymin = ymin, .ymax = ymax});
}

RtVoid RiProjection(RtToken name, ...) {
    va_list arguments;
    va_start(arguments, name);
    std::vector<RtToken> tokens;
    std::vector<RtPointer> parms;
    Converter::collect(arguments, &tokens, &parms);
    va_end(arguments);
    RiProjectionV(name, static_cast<RtInt>(tokens.size()), tokens.data(), parms.data());
}

RtVoid RiProjectionV(RtToken name, RtInt n, RtToken tokens[], RtPointer parms[]) {
    handler().projection(Converter::text(name), converter().parameters(n, tokens, parms));
}

RtVoid RiClipping(RtFloat hither, RtFloat yon) {
    handler().clipping({.hither = hither, .yon = yon});
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiClippingPlane(RtFloat x, RtFloat y, RtFloat z, RtFloat nx, RtFloat ny, RtFloat nz) {
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiDepthOfField(RtFloat fstop, RtFloat focallength, RtFloat focaldistance) {
    handler().depthOfField({.fstop = fstop, .focalLength = focallength, .focalDistance = focaldistance});
}

RtVoid RiShutter(RtFloat min, RtFloat max) {
    handler().shutter({.open = min, .close = max});
}

RtVoid RiPixelVariance(RtFloat variation) {
    handler().pixelVariance(variation);
}

RtVoid RiPixelSamples(RtFloat xsamples, RtFloat ysamples) {
    handler().pixelSamples(glm::uvec2(
        v3d::render::offline::film::sampleCount(xsamples), v3d::render::offline::film::sampleCount(ysamples)));
}

RtVoid RiPixelFilter(RtFilterFunc filterfunc, RtFloat xwidth, RtFloat ywidth) {
    if (const std::optional<v3d::render::offline::film::Filter> filter = Converter::filter(filterfunc, xwidth, ywidth)) {
        handler().pixelFilter(*filter, xwidth, ywidth);
    }
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiExposure(RtFloat gain, RtFloat gamma) {
}

RtVoid RiImager(RtToken name, ...) {
    va_list arguments;
    va_start(arguments, name);
    std::vector<RtToken> tokens;
    std::vector<RtPointer> parms;
    Converter::collect(arguments, &tokens, &parms);
    va_end(arguments);
    RiImagerV(name, static_cast<RtInt>(tokens.size()), tokens.data(), parms.data());
}

RtVoid RiImagerV(RtToken name, RtInt n, RtToken tokens[], RtPointer parms[]) {
    handler().imager(Converter::text(name), converter().parameters(n, tokens, parms));
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiQuantize(RtToken type, RtInt one, RtInt min, RtInt max, RtFloat ampl) {
}

RtVoid RiDisplay(char *name, RtToken type, RtToken mode, ...) {
    va_list arguments;
    va_start(arguments, mode);
    std::vector<RtToken> tokens;
    std::vector<RtPointer> parms;
    Converter::collect(arguments, &tokens, &parms);
    va_end(arguments);
    RiDisplayV(name, type, mode, static_cast<RtInt>(tokens.size()), tokens.data(), parms.data());
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiDisplayV(char *name, RtToken type, RtToken mode, RtInt n, RtToken tokens[], RtPointer parms[]) {
    handler().display({.name = Converter::text(name), .type = Converter::text(type), .mode = Converter::text(mode)}, converter().parameters(n, tokens, parms));
}

RtVoid RiHider(RtToken type, ...) {
    va_list arguments;
    va_start(arguments, type);
    std::vector<RtToken> tokens;
    std::vector<RtPointer> parms;
    Converter::collect(arguments, &tokens, &parms);
    va_end(arguments);
    RiHiderV(type, static_cast<RtInt>(tokens.size()), tokens.data(), parms.data());
}

RtVoid RiHiderV(RtToken type, RtInt n, RtToken tokens[], RtPointer parms[]) {
    handler().hider(Converter::text(type), converter().parameters(n, tokens, parms));
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiColorSamples(RtInt n, RtFloat nRGB[], RtFloat RGBn[]) {
}

RtVoid RiRelativeDetail(RtFloat relativedetail) {
}

RtVoid RiOption(RtToken name, ...) {
    va_list arguments;
    va_start(arguments, name);
    std::vector<RtToken> tokens;
    std::vector<RtPointer> parms;
    Converter::collect(arguments, &tokens, &parms);
    va_end(arguments);
    RiOptionV(name, static_cast<RtInt>(tokens.size()), tokens.data(), parms.data());
}

RtVoid RiOptionV(RtToken name, RtInt n, RtToken tokens[], RtPointer parms[]) {
    handler().option(Converter::text(name), converter().parameters(n, tokens, parms));
}

RtVoid RiAttributeBegin(void) {
    handler().attributeBegin();
}

RtVoid RiAttributeEnd(void) {
    handler().attributeEnd();
}

RtVoid RiColor(RtColor color) {
    handler().color(glm::vec3(color[0], color[1], color[2]));
}

RtVoid RiOpacity(RtColor color) {
    handler().opacity(glm::vec3(color[0], color[1], color[2]));
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiTextureCoordinates(RtFloat s1, RtFloat t1, RtFloat s2, RtFloat t2, RtFloat s3, RtFloat t3, RtFloat s4, RtFloat t4) {
}

// creates a light, switches it on, and returns the handle RiIlluminate takes
RtLightHandle RiLightSource(RtToken name, ...) {
    va_list arguments;
    va_start(arguments, name);
    std::vector<RtToken> tokens;
    std::vector<RtPointer> parms;
    Converter::collect(arguments, &tokens, &parms);
    va_end(arguments);
    return RiLightSourceV(name, static_cast<RtInt>(tokens.size()), tokens.data(), parms.data());
}

RtLightHandle RiLightSourceV(RtToken name, RtInt n, RtToken tokens[], RtPointer parms[]) {
    return light(name, n, tokens, parms, false);
}

RtLightHandle RiAreaLightSource(RtToken name, ...) {
    va_list arguments;
    va_start(arguments, name);
    std::vector<RtToken> tokens;
    std::vector<RtPointer> parms;
    Converter::collect(arguments, &tokens, &parms);
    va_end(arguments);
    return RiAreaLightSourceV(name, static_cast<RtInt>(tokens.size()), tokens.data(), parms.data());
}

RtLightHandle RiAreaLightSourceV(RtToken name, RtInt n, RtToken tokens[], RtPointer parms[]) {
    return light(name, n, tokens, parms, true);
}

RtVoid RiIlluminate(RtLightHandle light, RtBoolean onoff) {
    handler().illuminate(Converter::text(static_cast<const char*>(light)), onoff != RI_FALSE);
}

RtVoid RiSurface(RtToken name, ...) {
    va_list arguments;
    va_start(arguments, name);
    std::vector<RtToken> tokens;
    std::vector<RtPointer> parms;
    Converter::collect(arguments, &tokens, &parms);
    va_end(arguments);
    RiSurfaceV(name, static_cast<RtInt>(tokens.size()), tokens.data(), parms.data());
}

RtVoid RiSurfaceV(RtToken name, RtInt n, RtToken tokens[], RtPointer parms[]) {
    handler().surface(Converter::text(name), converter().parameters(n, tokens, parms));
}

RtVoid RiAtmosphere(RtToken name, ...) {
}

RtVoid RiAtmosphereV(RtToken name, RtInt n, RtToken tokens[], RtPointer parms[]) {
}

RtVoid RiInterior(RtToken name, ...) {
}

RtVoid RiInteriorV(RtToken name, RtInt n, RtToken tokens[], RtPointer parms[]) {
}

RtVoid RiExterior(RtToken name, ...) {
}

RtVoid RiExteriorV(RtToken name, RtInt n, RtToken tokens[], RtPointer parms[]) {
}

RtVoid RiShadingRate(RtFloat size) {
    handler().shadingRate(size);
}

RtVoid RiShadingInterpolation(RtToken type) {
}

RtVoid RiMatte(RtBoolean onoff) {
}

RtVoid RiBound(RtBound bound) {
}

RtVoid RiDetail(RtBound bound) {
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiDetailRange(RtFloat minvis, RtFloat lowtran, RtFloat uptran, RtFloat maxvis) {
}

RtVoid RiGeometricApproximation(RtToken type, RtFloat value) {
}

RtVoid RiOrientation(RtToken orientation) {
}

RtVoid RiReverseOrientation(void) {
}

RtVoid RiSides(RtInt sides) {
}

RtVoid RiIdentity(void) {
    handler().identity();
}

RtVoid RiTransform(RtMatrix transform) {
    handler().transform(Converter::matrix(transform));
}

RtVoid RiConcatTransform(RtMatrix transform) {
    handler().concatTransform(Converter::matrix(transform));
}

RtVoid RiPerspective(RtFloat fov) {
}

RtVoid RiTranslate(RtFloat dx, RtFloat dy, RtFloat dz) {
    handler().translate(dx, dy, dz);
}

// angle is in degrees
RtVoid RiRotate(RtFloat angle, RtFloat dx, RtFloat dy, RtFloat dz) {
    handler().rotate(angle, glm::vec3(dx, dy, dz));
}

RtVoid RiScale(RtFloat sx, RtFloat sy, RtFloat sz) {
    handler().scale(sx, sy, sz);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiSkew(RtFloat angle, RtFloat dx1, RtFloat dy1, RtFloat dz1, RtFloat dx2, RtFloat dy2, RtFloat dz2) {
}

RtVoid RiDeformation(RtToken name, ...) {
}

RtVoid RiDeformationV(RtToken name, RtInt n, RtToken tokens[], RtPointer parms[]) {
}

RtVoid RiDisplacement(RtToken name, ...) {
    va_list arguments;
    va_start(arguments, name);
    std::vector<RtToken> tokens;
    std::vector<RtPointer> parms;
    Converter::collect(arguments, &tokens, &parms);
    va_end(arguments);
    RiDisplacementV(name, static_cast<RtInt>(tokens.size()), tokens.data(), parms.data());
}

RtVoid RiDisplacementV(RtToken name, RtInt n, RtToken tokens[], RtPointer parms[]) {
    handler().displacement(Converter::text(name), converter().parameters(n, tokens, parms));
}

RtVoid RiCoordinateSystem(RtToken space) {
    handler().coordinateSystem(Converter::text(space));
}

RtVoid RiCoordSysTransform(RtToken space) {
    handler().coordSysTransform(Converter::text(space));
}

RtPoint * RiTransformPoints(RtToken /* fromspace */, RtToken /* tospace */, RtInt /* n */, RtPoint /* points */[]) {
    return 0;
}

RtVoid RiTransformBegin(void) {
    handler().transformBegin();
}

RtVoid RiTransformEnd(void) {
    handler().transformEnd();
}

RtVoid RiAttribute(RtToken name, ...) {
    va_list arguments;
    va_start(arguments, name);
    std::vector<RtToken> tokens;
    std::vector<RtPointer> parms;
    Converter::collect(arguments, &tokens, &parms);
    va_end(arguments);
    RiAttributeV(name, static_cast<RtInt>(tokens.size()), tokens.data(), parms.data());
}

RtVoid RiAttributeV(RtToken name, RtInt n, RtToken tokens[], RtPointer parms[]) {
    handler().attribute(Converter::text(name), converter().parameters(n, tokens, parms));
}

RtVoid RiPolygon(RtInt nverts, ...) {
    va_list arguments;
    va_start(arguments, nverts);
    std::vector<RtToken> tokens;
    std::vector<RtPointer> parms;
    Converter::collect(arguments, &tokens, &parms);
    va_end(arguments);
    RiPolygonV(nverts, static_cast<RtInt>(tokens.size()), tokens.data(), parms.data());
}

// a vertex or varying value is one element per corner. A count below zero is no corners
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiPolygonV(RtInt nverts, RtInt n, RtToken tokens[], RtPointer parms[]) {
    const unsigned int vertices = static_cast<unsigned int>(std::max(nverts, 0));
    handler().polygon(vertices, converter().parameters(n, tokens, parms, vertices));
}

RtVoid RiGeneralPolygon(RtInt nloops, RtInt nverts[], ...) {
}

RtVoid RiGeneralPolygonV(RtInt nloops, RtInt nverts[], RtInt n, RtToken tokens[], RtPointer parms[]) {
}

RtVoid RiPointsPolygons(RtInt npolys, RtInt nverts[], RtInt verts[], ...) {
    va_list arguments;
    va_start(arguments, verts);
    std::vector<RtToken> tokens;
    std::vector<RtPointer> parms;
    Converter::collect(arguments, &tokens, &parms);
    va_end(arguments);
    RiPointsPolygonsV(npolys, nverts, verts, static_cast<RtInt>(tokens.size()), tokens.data(), parms.data());
}

RtVoid RiPointsPolygonsV(RtInt npolys, RtInt nverts[], RtInt verts[], RtInt n, RtToken tokens[], RtPointer parms[]) {
    unsigned int points = 0;
    if (const std::optional<v3d::render::offline::rib::PointsPolygons> request = Converter::pointsPolygons(npolys, nverts, verts, &points)) {
        handler().pointsPolygons(*request, converter().parameters(n, tokens, parms, points));
    }
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiPointsGeneralPolygons(RtInt npolys, RtInt nloops[], RtInt nverts[], RtInt verts[], ...) {
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiPointsGeneralPolygonsV(RtInt npolys, RtInt nloops[], RtInt nverts[], RtInt verts[], RtInt n, RtToken tokens[], RtPointer parms[]) {
}

RtVoid RiBasis(RtBasis ubasis, RtInt ustep, RtBasis vbasis, RtInt vstep) {
    if (const std::optional<v3d::render::offline::rib::Basis> request = converter().basis(ubasis, ustep, vbasis, vstep)) {
        handler().basis(*request);
    }
}

RtVoid RiPatch(RtToken type, ...) {
}

RtVoid RiPatchV(RtToken type, RtInt n, RtToken tokens[], RtPointer parms[]) {
}

RtVoid RiPatchMesh(RtToken type, RtInt nu, RtToken uwrap, RtInt nv, RtToken vwrap, ...) {
}

RtVoid RiPatchMeshV(RtToken type, RtInt nu, RtToken uwrap, RtInt nv, RtToken vwrap, RtInt n, RtToken tokens[], RtPointer parms[]) {
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiNuPatch(RtInt nu, RtInt uorder, RtFloat uknot[], RtFloat umin, RtFloat umax, RtInt nv, RtInt vorder, RtFloat vknot[], RtFloat vmin, RtFloat vmax, ...) {
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiNuPatchV(RtInt nu, RtInt uorder, RtFloat uknot[], RtFloat umin, RtFloat umax, RtInt nv, RtInt vorder, RtFloat vknot[], RtFloat vmin, RtFloat vmax,
    RtInt n, RtToken tokens[], RtPointer parms[]) {
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiTrimCurve(RtInt nloops, RtInt ncurves[], RtInt order[], RtFloat knot[], RtFloat min[], RtFloat max[], RtInt n[], RtFloat u[], RtFloat v[], RtFloat w[]) {
}

// RI's last named argument is a float, which a variadic list cannot be started from, so this
// form reads no parameter. moya reads none of a sphere's
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiSphere(RtFloat radius, RtFloat zmin, RtFloat zmax, RtFloat tmax, ...) {
    RiSphereV(radius, zmin, zmax, tmax, 0, nullptr, nullptr);
}

// a quadric has a varying value at each of the four corners of its parameter space
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiSphereV(RtFloat radius, RtFloat zmin, RtFloat zmax, RtFloat tmax, RtInt n, RtToken tokens[], RtPointer parms[]) {
    handler().sphere({.radius = radius, .zmin = zmin, .zmax = zmax, .thetamax = tmax}, converter().parameters(n, tokens, parms, 4));
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiCone(RtFloat height, RtFloat radius, RtFloat tmax, ...) {
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiConeV(RtFloat height, RtFloat radius, RtFloat tmax, RtInt n, RtToken tokens[], RtPointer parms[]) {
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiCylinder(RtFloat radius, RtFloat zmin, RtFloat zmax, RtFloat tmax, ...) {
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiCylinderV(RtFloat radius, RtFloat zmin, RtFloat zmax, RtFloat tmax, RtInt n, RtToken tokens[], RtPointer parms[]) {
}

RtVoid RiHyperboloid(RtPoint point1, RtPoint point2, RtFloat tmax, ...) {
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiHyperboloidV(RtPoint point1, RtPoint point2, RtFloat tmax, RtInt n, RtToken tokens[], RtPointer parms[]) {
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiParaboloid(RtFloat rmax, RtFloat zmin, RtFloat zmax, RtFloat tmax, ...) {
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiParaboloidV(RtFloat rmax, RtFloat zmin, RtFloat zmax, RtFloat tmax, RtInt n, RtToken tokens[], RtPointer parms[]) {
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiDisk(RtFloat height, RtFloat radius, RtFloat tmax, ...) {
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiDiskV(RtFloat height, RtFloat radius, RtFloat tmax, RtInt n, RtToken tokens[], RtPointer parms[]) {
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiTorus(RtFloat majrad, RtFloat minrad, RtFloat phimin, RtFloat phimax, RtFloat tmax, ...) {
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiTorusV(RtFloat majrad, RtFloat minrad, RtFloat phimin, RtFloat phimax, RtFloat tmax, RtInt n, RtToken tokens[], RtPointer parms[]) {
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiBlobby(RtInt nleaf, RtInt ncode, RtInt code[], RtInt nflt, RtFloat flt[], RtInt nstr, RtToken str[], ...) {
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiBlobbyV(RtInt nleaf, RtInt ncode, RtInt code[], RtInt nflt, RtFloat flt[], RtInt nstr, RtToken str[], RtInt n, RtToken tokens[], RtPointer parms[]) {
}

RtVoid RiCurves(RtToken type, RtInt nvcurves, RtInt nvertices[], RtToken wrap, ...) {
    va_list arguments;
    va_start(arguments, wrap);
    std::vector<RtToken> tokens;
    std::vector<RtPointer> parms;
    Converter::collect(arguments, &tokens, &parms);
    va_end(arguments);
    RiCurvesV(type, nvcurves, nvertices, wrap, static_cast<RtInt>(tokens.size()), tokens.data(), parms.data());
}

RtVoid RiCurvesV(RtToken type, RtInt nvcurves, RtInt nvertices[], RtToken wrap, RtInt n, RtToken tokens[], RtPointer parms[]) {
    unsigned int total = 0;
    if (const std::optional<v3d::render::offline::rib::Curves> request = converter().curves(type, nvcurves, nvertices, wrap, &total)) {
        handler().curves(*request, converter().curveParameters(n, tokens, parms, total));
    }
}

RtVoid RiPoints(RtInt nverts, ...) {
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiPointsV(RtInt nverts, RtInt n, RtToken tokens[], RtPointer parms[]) {
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiSubdivisionMesh(RtToken mask, RtInt nf, RtInt nverts[], RtInt verts[], RtInt ntags, RtToken tags[], RtInt numargs[], RtInt intargs[],
    RtFloat floatargs[], ...) {
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiSubdivisionMeshV(RtToken mask, RtInt nf, RtInt nverts[], RtInt verts[], RtInt ntags, RtToken tags[], RtInt numargs[], RtInt intargs[],
    RtFloat floatargs[], RtInt n, RtToken tokens[], RtPointer parms[]) {
}

RtVoid RiProcedural(RtPointer data, RtBound bound, RtVoid (*subdivfunc)(RtPointer, RtFloat), RtVoid (*freefunc)(RtPointer)) {
}

RtVoid RiGeometry(RtToken type, ...) {
}

RtVoid RiGeometryV(RtToken type, RtInt n, RtToken tokens[], RtPointer parms[]) {
}

RtVoid RiSolidBegin(RtToken operation) {
}

RtVoid RiSolidEnd(void) {
}

RtObjectHandle RiObjectBegin(void) {
    return 0;
}

RtVoid RiObjectEnd(void) {
}

RtVoid RiObjectInstance(RtObjectHandle handle) {
}

RtVoid RiMotionBegin(RtInt n, ...) {
    // n times follow
    va_list arguments;
    va_start(arguments, n);
    const std::vector<float> times = Converter::times(arguments, n);
    va_end(arguments);
    handler().motionBegin(times);
}

RtVoid RiMotionBeginV(RtInt n, RtInt n2, RtToken tokens[], RtPointer parms[]) {
}

RtVoid RiMotionEnd(void) {
    handler().motionEnd();
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiMakeTexture(char *pic, char *tex, RtToken swrap, RtToken twrap, RtFilterFunc filterfunc, RtFloat swidth, RtFloat twidth, ...) {
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiMakeTextureV(char *pic, char *tex, RtToken swrap, RtToken twrap, RtFilterFunc filterfunc, RtFloat swidth, RtFloat twidth, RtInt n, RtToken tokens[], RtPointer parms[]) {
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiMakeBump(char *pic, char *tex, RtToken swrap, RtToken twrap, RtFilterFunc filterfunc, RtFloat swidth, RtFloat twidth, ...) {
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiMakeBumpV(char *pic, char *tex, RtToken swrap, RtToken twrap, RtFilterFunc filterfunc, RtFloat swidth, RtFloat twidth, RtInt n, RtToken tokens[], RtPointer parms[]) {
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiMakeLatLongEnvironment(char *pic, char *tex, RtFilterFunc filterfunc, RtFloat swidth, RtFloat twidth, ...) {
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiMakeLatLongEnvironmentV(char *pic, char *tex, RtFilterFunc filterfunc, RtFloat swidth, RtFloat twidth, RtInt n, RtToken tokens[], RtPointer parms[]) {
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiMakeCubeFaceEnvironment(char *px, char *nx, char *py, char *ny, char *pz, char *nz, char *tex, RtFloat fov, RtFilterFunc filterfunc, RtFloat swidth, RtFloat ywidth, ...) {
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiMakeCubeFaceEnvironmentV(char *px, char *nx, char *py, char *ny, char *pz, char *nz, char *tex, RtFloat fov, RtFilterFunc filterfunc, RtFloat swidth, RtFloat ywidth,
    RtInt n, RtToken tokens[], RtPointer parms[]) {
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiMakeShadow(char *pic, char *tex, ...) {
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiMakeShadowV(char *pic, char *tex, RtInt n, RtToken tokens[], RtPointer parms[]) {
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiArchiveRecord(RtToken type, char *format, ...) {
}

RtVoid RiReadArchive(RtToken name, RtArchiveCallback callback, ...) {
}

RtVoid RiReadArchiveV(RtToken name, RtArchiveCallback callback, RtInt n, RtToken tokens[], RtPointer parms[]) {
}

RtVoid RiErrorHandler(RtErrorHandler handler) {
}
