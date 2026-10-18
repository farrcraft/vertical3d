/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "RenderMan.h"
#include "Polygon.h"
#include "Renderer.h"

#include <api/render/offline/Sampling.h>
#include <api/render/offline/rib/Arguments.h>
#include <api/render/offline/rib/Parameters.h>

#include <stdarg.h>
#include <string.h>

#include <algorithm>
#include <cstddef>
#include <deque>
#include <string>
#include <vector>

#include "RenderContext.h"
#include "Vertex.h"

#include <glm/gtc/type_ptr.hpp>

namespace {

// the renderer the C interface drives. It is built on first use, inside the RI call that
// reaches it, so an exception from its constructor reaches the caller. An exception from a
// static initialiser reaches no handler at all.
v3d::moya::Renderer& renderer() {
    static v3d::moya::Renderer instance;
    return instance;
}

/**
 * Read a variadic parameter list into the two arrays the vector form takes.
 *
 * RI terminates the list with RI_NULL, because a va_list carries no count. A variadic entry
 * point that calls this has no body of its own. It hands the arrays to its vector form, so
 * the two forms of a request cannot drift apart.
 **/
void collect(va_list arguments, std::vector<RtToken>* tokens, std::vector<RtPointer>* parms) {
    for (;;) {
        RtToken token = va_arg(arguments, RtToken);
        if (token == RI_NULL) {
            return;
        }
        tokens->push_back(token);
        parms->push_back(va_arg(arguments, RtPointer));
    }
}

/**
 * The parameter list of a shader request, typed by what the context has declared.
 *
 * A shader's parameters are uniform, so one vertex is the count that bounds every value
 * array: a uniform value is one element whatever it is attached to.
 **/
void parameters(v3d::moya::RenderContext & rc, RtInt n, RtToken tokens[], RtPointer parms[],
    v3d::render::offline::rib::ParameterList* list) {
    std::vector<std::string> unresolved;
    v3d::render::offline::rib::arguments(rc.declarations(), n, tokens, parms, 1, list,
        &unresolved);
    for (const std::string & name : unresolved) {
        // a name with no declaration has no length either, so it is dropped rather than
        // read past, as the RIB reader does for a file
        rc.logger()->get()->warn("RI parameter '{}' was not declared and was skipped", name);
    }
}

/**
 * A light handle a C caller can hold on to.
 *
 * RtLightHandle is an untyped pointer, so the characters it points at must outlive the call
 * that made it. A caller may pass one back to RiIlluminate at any later point. The handles
 * are kept in a deque, whose elements do not move when another is added.
 **/
RtLightHandle keep(const std::string & value) {
    static std::deque<std::string> handles;
    handles.push_back(value);
    return const_cast<char*>(handles.back().c_str());
}

/** How many lights the C interface has made. Each light's handle is the count before it. **/
unsigned int made = 0;

};  // namespace

// a token that a request reads holds RI's string for it. The context writes a file only when
// the display type is "file", and RiDisplay passes a null type on as an empty string
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
RtToken RI_HIDDEN,
        RI_PAINT;
RtToken RI_CONSTANT,
        RI_SMOOTH;
RtToken RI_FLATNESS;
RtToken RI_FOV = const_cast<char*>("fov");
RtToken RI_AMBIENTLIGHT,
        RI_POINTLIGHT,
        RI_DISTANTLIGHT,
        RI_SPOTLIGHT;
RtToken RI_INTENSITY,
        RI_LIGHTCOLOR,
        RI_FROM,
        RI_TO,
        RI_CONEANGLE,
        RI_CONEDELTAANGLE,
        RI_BEAMDISTRIBUTION;
RtToken RI_MATTE,
        RI_METAL,
        RI_SHINYMETAL,
        RI_PLASTIC,
        RI_PAINTEDPLASTIC;
RtToken RI_KA,
        RI_KD,
        RI_KS,
        RI_ROUGHNESS,
        RI_KR,
        RI_TEXTURENAME,
        RI_SPECULARCOLOR;
RtToken RI_DEPTHCUE,
        RI_FOG,
        RI_BUMPY;
RtToken RI_MINDISTANCE,
        RI_MAXDISTANCE,
        RI_BACKGROUND,
        RI_DISTANCE,
        RI_AMPLITUDE;
RtToken RI_RASTER,
        RI_SCREEN,
        RI_CAMERA,
        RI_WORLD,
        RI_OBJECT;
RtToken RI_INSIDE,
        RI_OUTSIDE,
        RI_LH,
        RI_RH;
RtToken RI_P = const_cast<char*>("P");
RtToken RI_PZ = const_cast<char*>("Pz");
RtToken RI_PW = const_cast<char*>("Pw");
RtToken RI_N = const_cast<char*>("N");
RtToken RI_NP;
RtToken RI_CS = const_cast<char*>("Cs");
RtToken RI_OS = const_cast<char*>("Os");
RtToken RI_S = const_cast<char*>("s");
RtToken RI_T = const_cast<char*>("t");
RtToken RI_ST = const_cast<char*>("st");
RtToken RI_BILINEAR,
        RI_BICUBIC;
RtToken RI_LINEAR,
        RI_CUBIC;
RtToken RI_PRIMITIVE,
        RI_INTERSECTION,
        RI_UNION,
        RI_DIFFERENCE;
RtToken RI_PERIODIC,
        RI_NONPERIODIC,
        RI_CLAMP,
        RI_BLACK;
RtToken RI_IGNORE,
        RI_PRINT,
        RI_ABORT,
        RI_HANDLER;
RtToken RI_COMMENT,
        RI_STRUCTURE,
        RI_VERBATIM;
RtToken RI_IDENTIFIER,
        RI_NAME,
        RI_SHADINGGROUP;
RtToken RI_WIDTH,
        RI_CONSTANTWIDTH;

RtBasis RiBezierBasis,
        RiBSplineBasis,
        RiCatmullRomBasis,
        RiHermiteBasis,
        RiPowerBasis;

RtInt RiLastError;

// RI subroutines. The RenderMan Interface sets every signature below, including the order of
// parameters that share a type, so each that the swappable parameter check reports is marked.
RtFloat RiGaussianFilter(RtFloat x, RtFloat y, RtFloat xwidth, RtFloat ywidth) {
    return v3d::render::offline::filter(v3d::render::offline::Filter::Gaussian, glm::vec2(x, y), glm::vec2(xwidth, ywidth));
}

RtFloat RiBoxFilter(RtFloat x, RtFloat y, RtFloat xwidth, RtFloat ywidth) {
    return v3d::render::offline::filter(v3d::render::offline::Filter::Box, glm::vec2(x, y), glm::vec2(xwidth, ywidth));
}

RtFloat RiTriangleFilter(RtFloat x, RtFloat y, RtFloat xwidth, RtFloat ywidth) {
    return v3d::render::offline::filter(v3d::render::offline::Filter::Triangle, glm::vec2(x, y), glm::vec2(xwidth, ywidth));
}

RtFloat RiCatmullRomFilter(RtFloat x, RtFloat y, RtFloat xwidth, RtFloat ywidth) {
    return v3d::render::offline::filter(v3d::render::offline::Filter::CatmullRom, glm::vec2(x, y), glm::vec2(xwidth, ywidth));
}

RtFloat RiSincFilter(RtFloat x, RtFloat y, RtFloat xwidth, RtFloat ywidth) {
    return v3d::render::offline::filter(v3d::render::offline::Filter::Sinc, glm::vec2(x, y), glm::vec2(xwidth, ywidth));
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

// RiGetContext and RiContext have no RIB equivalents
RtContextHandle RiGetContext(void) {
    // the handle is the active context. RiContext ignores it and does not switch contexts
    return &renderer().activeRenderContext();
}

RtVoid RiContext(RtContextHandle /* handle */) {
}

RtToken RiDeclare(char * name, char * declaration) {
    if (name == nullptr || declaration == nullptr) {
        return 0;
    }
    renderer().activeRenderContext().declarations().declare({.name = name, .type = declaration});
    // RI returns the token the name now stands for. In this renderer the token is the name
    return name;
}


// RiBegin pushes a new context and makes it active. RI_NULL is the empty name
RtVoid RiBegin(RtToken name) {
    std::string str;
    if (name != RI_NULL) {
        str = name;
    }
    renderer().createRenderContext(str);
}


// RiEnd pops the active context, and the one beneath it becomes active. A request that
// arrives with no context active creates one
RtVoid RiEnd(void) {
    renderer().destroyActiveRenderContext();
}

RtVoid RiFrameBegin(RtInt frame) {
}

RtVoid RiFrameEnd(void) {
}

// the transformation in force becomes the world to camera transformation, and the
// framebuffer is allocated
RtVoid RiWorldBegin(void) {
    renderer().activeRenderContext().prepareWorld();
}

// renders the frame, and writes it when the display type is "file"
RtVoid RiWorldEnd(void) {
    renderer().activeRenderContext().render();
}

RtVoid RiFormat(RtInt xres, RtInt yres, RtFloat aspect) {
    // the context takes a side of zero or less as its default, and refuses one that is too large
    renderer().activeRenderContext().imageResolution(glm::ivec2(xres, yres), aspect);
}

RtVoid RiFrameAspectRatio(RtFloat aspect) {
    renderer().activeRenderContext().frameAspectRatio(aspect);
}

RtVoid RiScreenWindow(RtFloat left, RtFloat right, RtFloat bot, RtFloat top) {
    renderer().activeRenderContext().screenWindow({.left = left, .right = right, .bottom = bot, .top = top});
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiCropWindow(RtFloat xmin, RtFloat xmax, RtFloat ymin, RtFloat ymax) {
}

// "perspective" reads an optional "fov", in degrees, and takes 90 without one. RI_NULL
// selects orthographic
RtVoid RiProjection(RtToken name, ...) {
    va_list ap;
    va_start(ap, name);
    RtToken param = va_arg(ap, RtToken);
    std::string token;
    float fov = v3d::moya::RenderContext::defaultFov;
    if (name == RI_NULL) {
        renderer().activeRenderContext().projection("");
    } else if (strncmp(name, RI_PERSPECTIVE, strlen(RI_PERSPECTIVE)) == 0) {
        // perspective takes an optional fov parameter
        if (param != RI_NULL) {
            token = va_arg(ap, RtToken);
            if (token == RI_FOV) {
                fov = static_cast<float>(va_arg(ap, double));
            }
        }
        renderer().activeRenderContext().projection(name, fov);
    } else {
        renderer().activeRenderContext().projection(name);
    }
    va_end(ap);
}

RtVoid RiProjectionV(RtToken name, RtInt n, RtToken tokens[], RtPointer parms[]) {
}

RtVoid RiClipping(RtFloat hither, RtFloat yon) {
    renderer().activeRenderContext().clipping({.hither = hither, .yon = yon});
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiClippingPlane(RtFloat x, RtFloat y, RtFloat z, RtFloat nx, RtFloat ny, RtFloat nz) {
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiDepthOfField(RtFloat fstop, RtFloat focallength, RtFloat focaldistance) {
    v3d::render::offline::Sampling & sampling = renderer().activeRenderContext().sampling();
    sampling.fstop = fstop;
    sampling.focalLength = focallength;
    sampling.focalDistance = focaldistance;
}

RtVoid RiShutter(RtFloat min, RtFloat max) {
    renderer().activeRenderContext().sampling().shutter = glm::vec2(min, max);
}

RtVoid RiPixelVariance(RtFloat variation) {
    renderer().activeRenderContext().sampling().variance = variation;
}

RtVoid RiPixelSamples(RtFloat xsamples, RtFloat ysamples) {
    renderer().activeRenderContext().sampling().samples = glm::uvec2(
        v3d::render::offline::sampleCount(xsamples), v3d::render::offline::sampleCount(ysamples));
}

RtVoid RiPixelFilter(RtFilterFunc filterfunc, RtFloat xwidth, RtFloat ywidth) {
    // the C interface names a filter by its function, and only the five RI declares are
    // known; any other leaves the filter the context had
    v3d::render::offline::Filter filter = v3d::render::offline::Filter::Gaussian;
    if (filterfunc == RiBoxFilter) {
        filter = v3d::render::offline::Filter::Box;
    } else if (filterfunc == RiTriangleFilter) {
        filter = v3d::render::offline::Filter::Triangle;
    } else if (filterfunc == RiCatmullRomFilter) {
        filter = v3d::render::offline::Filter::CatmullRom;
    } else if (filterfunc == RiGaussianFilter) {
        filter = v3d::render::offline::Filter::Gaussian;
    } else if (filterfunc == RiSincFilter) {
        filter = v3d::render::offline::Filter::Sinc;
    } else {
        return;
    }
    // a width the RIB reader would refuse is refused here too, and the context keeps its filter
    if (!v3d::render::offline::filterWidth(glm::vec2(xwidth, ywidth))) {
        return;
    }
    v3d::render::offline::Sampling & sampling = renderer().activeRenderContext().sampling();
    sampling.filter = filter;
    sampling.width = glm::vec2(xwidth, ywidth);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiExposure(RtFloat gain, RtFloat gamma) {
}

RtVoid RiImager(RtToken name, ...) {
    va_list arguments;
    va_start(arguments, name);
    std::vector<RtToken> tokens;
    std::vector<RtPointer> parms;
    collect(arguments, &tokens, &parms);
    va_end(arguments);
    RiImagerV(name, static_cast<RtInt>(tokens.size()), tokens.data(), parms.data());
}

RtVoid RiImagerV(RtToken name, RtInt n, RtToken tokens[], RtPointer parms[]) {
    if (name == nullptr) {
        return;
    }
    v3d::moya::RenderContext & rc = renderer().activeRenderContext();
    v3d::render::offline::rib::ParameterList list;
    parameters(rc, n, tokens, parms, &list);
    rc.imager(name, list);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiQuantize(RtToken type, RtInt one, RtInt min, RtInt max, RtFloat ampl) {
}

// the parameter list is not read. A null name, type or mode reaches the context as an
// empty string
RtVoid RiDisplay(char *name, RtToken type, RtToken mode, ...) {
    renderer().activeRenderContext().display({
        .name = name != nullptr ? name : "",
        .type = type != nullptr ? type : "",
        .mode = mode != nullptr ? mode : ""});
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiDisplayV(char *name, RtToken type, RtToken mode, RtInt n, RtToken tokens[], RtPointer parms[]) {
}

RtVoid RiHider(RtToken type, ...) {
    renderer().activeRenderContext().hider(type != nullptr ? type : "");
}

RtVoid RiHiderV(RtToken type, RtInt n, RtToken tokens[], RtPointer parms[]) {
    // no hider takes a parameter
    (void)n;
    (void)tokens;
    (void)parms;
    renderer().activeRenderContext().hider(type != nullptr ? type : "");
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiColorSamples(RtInt n, RtFloat nRGB[], RtFloat RGBn[]) {
}

RtVoid RiRelativeDetail(RtFloat relativedetail) {
}

RtVoid RiOption(RtToken name, ...) {
}

RtVoid RiOptionV(RtToken name, RtInt n, RtToken tokens[], RtPointer parms[]) {
}

RtVoid RiAttributeBegin(void) {
    renderer().activeRenderContext().attributeBegin();
}

RtVoid RiAttributeEnd(void) {
    renderer().activeRenderContext().attributeEnd();
}

RtVoid RiColor(RtColor color) {
    renderer().activeRenderContext().color(glm::vec3(color[0], color[1], color[2]));
}

RtVoid RiOpacity(RtColor color) {
    renderer().activeRenderContext().opacity(glm::vec3(color[0], color[1], color[2]));
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
    collect(arguments, &tokens, &parms);
    va_end(arguments);
    return RiLightSourceV(name, static_cast<RtInt>(tokens.size()), tokens.data(), parms.data());
}

RtLightHandle RiLightSourceV(RtToken name, RtInt n, RtToken tokens[], RtPointer parms[]) {
    if (name == nullptr) {
        return 0;
    }
    v3d::moya::RenderContext & rc = renderer().activeRenderContext();
    /*
        A C caller has no handle to give, so the interface returns one. The render context
        keys lights on a string, because RIB 3.03 writes a handle as a number and later
        versions write a string. This handle is the count of lights made so far, the same
        sequence a 3.03 file writes.
    */
    const std::string handle = std::to_string(made++);
    v3d::render::offline::rib::ParameterList list;
    parameters(rc, n, tokens, parms, &list);
    rc.lightSource({.shader = name, .handle = handle}, list);
    return keep(handle);
}

RtLightHandle RiAreaLightSource(RtToken name, ...) {
    va_list arguments;
    va_start(arguments, name);
    std::vector<RtToken> tokens;
    std::vector<RtPointer> parms;
    collect(arguments, &tokens, &parms);
    va_end(arguments);
    return RiAreaLightSourceV(name, static_cast<RtInt>(tokens.size()), tokens.data(), parms.data());
}

RtLightHandle RiAreaLightSourceV(RtToken name, RtInt n, RtToken tokens[], RtPointer parms[]) {
    // an area light is a light whose shape matters, and moya does not sample its shape. It
    // reaches the context as an ordinary light so that a scene using one is still lit
    return RiLightSourceV(name, n, tokens, parms);
}

RtVoid RiIlluminate(RtLightHandle light, RtBoolean onoff) {
    if (light == nullptr) {
        return;
    }
    renderer().activeRenderContext().illuminate(static_cast<const char*>(light), onoff != RI_FALSE);
}

// a shader the library cannot load leaves no surface current, and a primitive with no
// surface is shaded with "constant"
RtVoid RiSurface(RtToken name, ...) {
    va_list arguments;
    va_start(arguments, name);
    std::vector<RtToken> tokens;
    std::vector<RtPointer> parms;
    collect(arguments, &tokens, &parms);
    va_end(arguments);
    RiSurfaceV(name, static_cast<RtInt>(tokens.size()), tokens.data(), parms.data());
}

RtVoid RiSurfaceV(RtToken name, RtInt n, RtToken tokens[], RtPointer parms[]) {
    if (name == nullptr) {
        return;
    }
    v3d::moya::RenderContext & rc = renderer().activeRenderContext();
    v3d::render::offline::rib::ParameterList list;
    parameters(rc, n, tokens, parms, &list);
    rc.surface(name, list);
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
    renderer().activeRenderContext().shadingRate(size);
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
    renderer().activeRenderContext().setIdentityTransform();
}

namespace {

/*
    An RtMatrix is sixteen floats in RI's row major order under a row vector convention;
    glm stores column major under a column vector one, so reading them in order is the
    change of convention and a transpose would undo it.
*/
glm::mat4x4 matrix(RtMatrix transform) {
    return glm::make_mat4(&transform[0][0]);
}

};  // namespace

RtVoid RiTransform(RtMatrix transform) {
    renderer().activeRenderContext().setTransform(matrix(transform));
}

RtVoid RiConcatTransform(RtMatrix transform) {
    renderer().activeRenderContext().concatTransform(matrix(transform));
}

RtVoid RiPerspective(RtFloat fov) {
}

RtVoid RiTranslate(RtFloat dx, RtFloat dy, RtFloat dz) {
    renderer().activeRenderContext().translate(dx, dy, dz);
}

// angle is in degrees
RtVoid RiRotate(RtFloat angle, RtFloat dx, RtFloat dy, RtFloat dz) {
    renderer().activeRenderContext().rotate(angle, glm::vec3(dx, dy, dz));
}

RtVoid RiScale(RtFloat sx, RtFloat sy, RtFloat sz) {
    renderer().activeRenderContext().scale(sx, sy, sz);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiSkew(RtFloat angle, RtFloat dx1, RtFloat dy1, RtFloat dz1, RtFloat dx2, RtFloat dy2, RtFloat dz2) {
}

RtVoid RiDeformation(RtToken name, ...) {
}

RtVoid RiDeformationV(RtToken name, RtInt n, RtToken tokens[], RtPointer parms[]) {
}

RtVoid RiDisplacement(RtToken name, ...) {
}

RtVoid RiDisplacementV(RtToken name, RtInt n, RtToken tokens[], RtPointer parms[]) {
}

RtVoid RiCoordinateSystem(RtToken space) {
    renderer().activeRenderContext().saveCoordinateSystem(space);
}

RtVoid RiCoordSysTransform(RtToken space) {
    renderer().activeRenderContext().setCoordinateSystem(space);
}

RtPoint * RiTransformPoints(RtToken /* fromspace */, RtToken /* tospace */, RtInt /* n */, RtPoint /* points */[]) {
    return 0;
}

RtVoid RiTransformBegin(void) {
    renderer().activeRenderContext().pushTransform();
}

RtVoid RiTransformEnd(void) {
    renderer().activeRenderContext().popTransform();
}

RtVoid RiAttribute(RtToken name, ...) {
}

RtVoid RiAttributeV(RtToken name, RtInt n, RtToken tokens[], RtPointer parms[]) {
}

// reads "P" only. Every other parameter is skipped, and a polygon is not checked for being
// planar, convex or degenerate
RtVoid RiPolygon(RtInt nverts, ...) {
    va_list ap;

    va_start(ap, nverts);

    RtToken name = va_arg(ap, RtToken);
    std::string token;
    const boost::shared_ptr<v3d::moya::Polygon> poly(new v3d::moya::Polygon);
    while (name != RI_NULL) {
        token = name;
        RtPointer value = va_arg(ap, RtPointer);
        if (token == RI_P) {
            // value contains RtPoint points[nverts]
            v3d::moya::Vertex v;
            const RtPoint *points = static_cast<RtPoint*>(value);
            for (int i = 0; i < nverts; i++) {
                v.point(glm::vec3(points[i][0], points[i][1], points[i][2]));
                poly->addVertex(v);
            }
        } else if (token == RI_PZ || token == RI_PW || token == RI_N || token == RI_CS ||
                   token == RI_OS || token == RI_S || token == RI_T || token == RI_ST) {
            // a standard parameter this renderer does not read: depth and homogeneous
            // positions, normals, colours, opacities and texture coordinates
        }
        // an unknown token is skipped the same way

        name = va_arg(ap, RtToken);
    }
    va_end(ap);

    renderer().activeRenderContext().addPolygon(poly);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiPolygonV(RtInt nverts, RtInt n, RtToken tokens[], RtPointer parms[]) {
}

RtVoid RiGeneralPolygon(RtInt nloops, RtInt nverts[], ...) {
}

RtVoid RiGeneralPolygonV(RtInt nloops, RtInt nverts[], RtInt n, RtToken tokens[], RtPointer parms[]) {
}

RtVoid RiPointsPolygons(RtInt npolys, RtInt nverts[], RtInt verts[], ...) {
}

RtVoid RiPointsPolygonsV(RtInt npolys, RtInt nverts[], RtInt verts[], RtInt n, RtToken tokens[], RtPointer parms[]) {
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiPointsGeneralPolygons(RtInt npolys, RtInt nloops[], RtInt nverts[], RtInt verts[], ...) {
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiPointsGeneralPolygonsV(RtInt npolys, RtInt nloops[], RtInt nverts[], RtInt verts[], RtInt n, RtToken tokens[], RtPointer parms[]) {
}

RtVoid RiBasis(RtBasis ubasis, RtInt ustep, RtBasis vbasis, RtInt vstep) {
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

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiSphere(RtFloat radius, RtFloat zmin, RtFloat zmax, RtFloat tmax, ...) {
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
RtVoid RiSphereV(RtFloat radius, RtFloat zmin, RtFloat zmax, RtFloat tmax, RtInt n, RtToken tokens[], RtPointer parms[]) {
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
}

RtVoid RiCurvesV(RtToken type, RtInt nvcurves, RtInt nvertices[], RtToken wrap, RtInt n, RtToken tokens[], RtPointer parms[]) {
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
    // n times follow, each promoted to a double on its way through the ellipsis
    va_list arguments;
    va_start(arguments, n);
    std::vector<float> times;
    times.reserve(static_cast<std::size_t>(std::max(n, 0)));
    for (RtInt i = 0; i < n; i++) {
        times.push_back(static_cast<float>(va_arg(arguments, double)));
    }
    va_end(arguments);
    renderer().activeRenderContext().motionBegin(times);
}

RtVoid RiMotionBeginV(RtInt n, RtInt n2, RtToken tokens[], RtPointer parms[]) {
}

RtVoid RiMotionEnd(void) {
    renderer().activeRenderContext().motionEnd();
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
