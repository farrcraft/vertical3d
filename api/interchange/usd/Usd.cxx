/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Usd.h"

#include <api/curve/Curve.h>
#include <api/interchange/Placement.h>
#include <api/interchange/Scene.h>
#include <api/type/Transform.h>
#include <pxr/base/gf/camera.h>
#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/range1f.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/array.h>
#include <pxr/base/vt/types.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/common.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/primFlags.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usdGeom/basisCurves.h>
#include <pxr/usd/usdGeom/camera.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/metrics.h>
#include <pxr/usd/usdGeom/nurbsCurves.h>
#include <pxr/usd/usdGeom/scope.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/usdGeom/xformable.h>
#include <pxr/usd/usdLux/distantLight.h>
#include <pxr/usd/usdLux/lightAPI.h>
#include <pxr/usd/usdLux/shapingAPI.h>
#include <pxr/usd/usdLux/sphereLight.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::interchange {

namespace {

/**
 * A camera's apertures are in tenths of the stage's unit.
 **/
constexpr double APERTURE_UNIT = 0.1;

/**
 * A shaping cone of a quarter turn or more lights the whole half space in front of it. A sphere
 * light with no shaping lights that side the same way, so only a narrower cone is a spot.
 **/
constexpr float WIDEST_SPOT = 90.0F;

/**
 * The solid angle of a whole sphere, which is also its surface over the square of its radius.
 **/
constexpr float SPHERE = 4.0F * glm::pi<float>();

/**
 * A GfMatrix4d is row major and turns a row vector, so its rows are glm's columns, and the
 * elements copy across in place.
 **/
glm::mat4 toGlm(const pxr::GfMatrix4d& matrix) {
    glm::mat4 converted(1.0F);
    for (glm::length_t column = 0; column < 4; column++) {
        for (glm::length_t row = 0; row < 4; row++) {
            converted[column][row] = static_cast<float>(matrix[column][row]);
        }
    }
    return converted;
}

/**
 * An attribute's value at a time, or a fallback when it has none there.
 **/
template <typename T>
T valueOf(const pxr::UsdAttribute& attribute, pxr::UsdTimeCode time, const T& fallback) {
    T value = fallback;
    if (attribute && attribute.Get(&value, time)) {
        return value;
    }
    return fallback;
}

/**
 * The same values in every key, which leave a part of a placement unkeyed.
 **/
bool constant(const std::vector<glm::vec4>& values) {
    return std::all_of(values.begin(), values.end(), [&values](const glm::vec4& value) { return value == values.front(); });
}

/**
 * The order of a cubic curve, and how far apart a cubic Bezier curve's points on the curve are.
 **/
constexpr unsigned int CUBIC_ORDER = 4;
constexpr std::size_t BEZIER_STEP = 3;

/**
 * Each end point reflected through its neighbour and added beyond the end. A uniform B-spline
 * or Catmull-Rom curve over the longer list starts and ends on the given end points. USD's
 * pinned wrap asks for that.
 **/
std::vector<glm::vec3> pinned(const std::vector<glm::vec3>& points) {
    std::vector<glm::vec3> extended;
    extended.reserve(points.size() + 2);
    extended.push_back((2.0F * points.front()) - points[1]);
    extended.insert(extended.end(), points.begin(), points.end());
    extended.push_back((2.0F * points.back()) - points[points.size() - 2]);
    return extended;
}

/**
 * A point a Bezier curve passes through, with free handles that start on it.
 **/
v3d::curve::BezierPoint freePoint(const glm::vec3& position) {
    return { .position = position, .left = position, .right = position, .leftType = v3d::curve::Handle::Free,
        .rightType = v3d::curve::Handle::Free };
}

/**
 * A cubic Bezier curve from its basis curve points: a point on the curve, then its right handle
 * and the next point's left one, and so on. A periodic curve's last two handles lead back to its
 * first point.
 **/
std::optional<v3d::curve::Curve> bezierOf(const std::vector<glm::vec3>& points, bool periodic) {
    const std::size_t count = points.size();
    if (periodic ? (count < BEZIER_STEP || count % BEZIER_STEP != 0) : (count < CUBIC_ORDER || (count - 1) % BEZIER_STEP != 0)) {
        return std::nullopt;
    }
    v3d::curve::Bezier bezier;
    for (std::size_t at = 0; at < count; at += BEZIER_STEP) {
        v3d::curve::BezierPoint point = freePoint(points[at]);
        if (at > 0 || periodic) {
            point.left = points[(at + count - 1) % count];
        }
        if (at + 1 < count) {
            point.right = points[at + 1];
        }
        bezier.points.push_back(point);
    }
    return v3d::curve::Curve{ .shape = bezier, .closed = periodic };
}

/**
 * An open Catmull-Rom basis curve as the Bezier curve it is. It runs from its second point to
 * its second to last, and each segment's handles lie a sixth of the way from one of its
 * neighbours to the other.
 **/
std::optional<v3d::curve::Curve> catmullRomOf(const std::vector<glm::vec3>& points) {
    constexpr float SIXTH = 1.0F / 6.0F;
    if (points.size() < CUBIC_ORDER) {
        return std::nullopt;
    }
    v3d::curve::Bezier bezier;
    for (std::size_t at = 1; at + 1 < points.size(); at++) {
        v3d::curve::BezierPoint point = freePoint(points[at]);
        if (at > 1) {
            point.left = points[at] - ((points[at + 1] - points[at - 1]) * SIXTH);
        }
        if (at + 2 < points.size()) {
            point.right = points[at] + ((points[at + 1] - points[at - 1]) * SIXTH);
        }
        bezier.points.push_back(point);
    }
    return v3d::curve::Curve{ .shape = bezier };
}

/**
 * An open uniform cubic B-spline basis curve, as a NURBS curve over evenly spaced knots, which
 * is the same curve.
 **/
v3d::curve::Curve bsplineOf(const std::vector<glm::vec3>& points) {
    v3d::curve::Nurbs nurbs;
    nurbs.points = points;
    for (std::size_t knot = 0; knot < points.size() + CUBIC_ORDER; knot++) {
        nurbs.knots.push_back(static_cast<float>(knot));
    }
    return { .shape = nurbs };
}

/**
 * How a BasisCurves prim's points make its curves: linear or cubic, the cubic basis, and the
 * wrap.
 **/
struct Form {
    pxr::TfToken type;
    pxr::TfToken basis;
    pxr::TfToken wrap;
};

/**
 * One curve of a BasisCurves prim, or nothing when its form makes no curve of its points.
 **/
std::optional<v3d::curve::Curve> basisCurveOf(const std::vector<glm::vec3>& points, const Form& form) {
    const pxr::TfToken& type = form.type;
    const pxr::TfToken& basis = form.basis;
    const bool periodic = form.wrap == pxr::UsdGeomTokens->periodic;
    const bool pin = form.wrap == pxr::UsdGeomTokens->pinned;
    if (points.size() < 2) {
        return std::nullopt;
    }
    if (type == pxr::UsdGeomTokens->linear) {
        return v3d::curve::Curve{ .shape = v3d::curve::Poly{ points }, .closed = periodic };
    }
    if (basis == pxr::UsdGeomTokens->bezier) {
        return bezierOf(points, periodic);
    }
    if (basis == pxr::UsdGeomTokens->catmullRom) {
        if (periodic) {
            return v3d::curve::Curve{ .shape = v3d::curve::CatmullRom{ points }, .closed = true };
        }
        return catmullRomOf(pin ? pinned(points) : points);
    }
    if (periodic) {
        v3d::curve::Nurbs nurbs;
        nurbs.points = points;
        return v3d::curve::Curve{ .shape = nurbs, .closed = true };
    }
    const std::vector<glm::vec3> spline = pin ? pinned(points) : points;
    if (spline.size() < CUBIC_ORDER) {
        return std::nullopt;
    }
    return bsplineOf(spline);
}

std::vector<glm::vec3> toGlm(const pxr::VtArray<pxr::GfVec3f>& points, std::size_t first, std::size_t count) {
    std::vector<glm::vec3> converted;
    converted.reserve(count);
    for (std::size_t index = first; index < first + count; index++) {
        converted.emplace_back(points[index][0], points[index][1], points[index][2]);
    }
    return converted;
}

/**
 * Fills a scene from a stage.
 **/
class Reader final {
 public:
    explicit Reader(const pxr::UsdStageRefPtr& stage) :
        stage_(stage),
        rest_(stage->HasAuthoredTimeCodeRange() ? pxr::UsdTimeCode(stage->GetStartTimeCode()) : pxr::UsdTimeCode::Default()),
        perSecond_(stage->GetTimeCodesPerSecond()),
        metres_(pxr::UsdGeomGetStageMetersPerUnit(stage)),
        upright_(1.0F) {
        if (pxr::UsdGeomGetStageUpAxis(stage) == pxr::UsdGeomTokens->z) {
            // a quarter turn about x takes z up to y up, as glTF's exporters turn it
            upright_ = glm::rotate(upright_, -glm::half_pi<float>(), glm::vec3(1.0F, 0.0F, 0.0F));
        }
        upright_ = glm::scale(glm::mat4(1.0F), glm::vec3(static_cast<float>(metres_))) * upright_;
    }

    Imported read() {
        const pxr::UsdPrimRange range = pxr::UsdPrimRange::Stage(stage_, pxr::UsdTraverseInstanceProxies());
        for (auto prim = range.begin(); prim != range.end(); ++prim) {
            if (!prim->IsA<pxr::UsdGeomXformable>() && !prim->IsA<pxr::UsdGeomScope>()) {
                prim.PruneChildren();
                continue;
            }
            addPrim(*prim);
        }
        Imported imported;
        imported.scene = std::move(scene_);
        imported.notes = std::move(notes_);
        return imported;
    }

 private:
    void addPrim(const pxr::UsdPrim& prim) {
        Node node;
        node.name = prim.GetName().GetString();
        const auto parent = nodes_.find(prim.GetParent().GetPath());
        if (parent != nodes_.end()) {
            node.parent = parent->second;
        }

        const pxr::UsdGeomXformable xformable(prim);
        const bool keyed = xformable && xformable.TransformMightBeTimeVarying();
        bool resets = false;
        glm::mat4 local = xformable ? localAt(xformable, rest_, &resets) : glm::mat4(1.0F);
        if (resets) {
            notes_.push_back("prim " + node.name + " discards its parent's placement, and is placed beneath its parent anyway");
        }
        if (!node.parent && upright_ != glm::mat4(1.0F)) {
            if (keyed) {
                node.parent = stageNode();
            } else {
                local = upright_ * local;
            }
        }
        node.placement = placementOf(local);

        if (prim.IsA<pxr::UsdGeomMesh>()) {
            node.mesh = addMesh(pxr::UsdGeomMesh(prim), node.name);
        } else if (prim.IsA<pxr::UsdGeomCamera>()) {
            node.camera = addCamera(pxr::UsdGeomCamera(prim), node.name);
        } else if (prim.HasAPI<pxr::UsdLuxLightAPI>()) {
            node.light = addLight(prim, node.name);
        } else if (prim.IsA<pxr::UsdGeomBasisCurves>()) {
            node.curves = addBasisCurves(pxr::UsdGeomBasisCurves(prim), node.name);
        } else if (prim.IsA<pxr::UsdGeomNurbsCurves>()) {
            node.curves = addNurbsCurves(pxr::UsdGeomNurbsCurves(prim), node.name);
        }

        const std::size_t index = scene_.nodes.size();
        nodes_.emplace(prim.GetPath(), index);
        scene_.nodes.push_back(std::move(node));
        if (keyed) {
            addKeys(xformable, index);
        }
    }

    /**
     * @param resets set to whether the prim discards its parent's placement
     **/
    static glm::mat4 localAt(const pxr::UsdGeomXformable& xformable, pxr::UsdTimeCode time, bool* resets) {
        pxr::GfMatrix4d matrix(1.0);
        xformable.GetLocalTransformation(&matrix, resets, time);
        return toGlm(matrix);
    }

    /**
     * The group that turns a keyed prim at the top of a z up stage, made when one is first met.
     **/
    std::size_t stageNode() {
        if (!stageGroup_) {
            Node node;
            node.name = "stage";
            node.placement = placementOf(upright_);
            stageGroup_ = scene_.nodes.size();
            scene_.nodes.push_back(std::move(node));
        }
        return *stageGroup_;
    }

    std::size_t addMesh(const pxr::UsdGeomMesh& source, const std::string& name) {
        Mesh mesh;
        mesh.name = name;
        const pxr::VtArray<pxr::GfVec3f> points = valueOf(source.GetPointsAttr(), rest_, pxr::VtArray<pxr::GfVec3f>());
        const pxr::VtArray<int> counts = valueOf(source.GetFaceVertexCountsAttr(), rest_, pxr::VtArray<int>());
        const pxr::VtArray<int> indices = valueOf(source.GetFaceVertexIndicesAttr(), rest_, pxr::VtArray<int>());
        for (const pxr::GfVec3f& point : points) {
            mesh.points.emplace_back(point[0], point[1], point[2]);
        }
        // a negative count or index is out of range for the builder, which refuses it
        for (const int count : counts) {
            mesh.counts.push_back(static_cast<std::uint32_t>(std::max(count, 0)));
        }
        for (const int index : indices) {
            mesh.indices.push_back(index < 0 ? static_cast<std::uint32_t>(mesh.points.size()) : static_cast<std::uint32_t>(index));
        }
        const pxr::TfToken orientation = valueOf(source.GetOrientationAttr(), rest_, pxr::UsdGeomTokens->rightHanded);
        if (orientation == pxr::UsdGeomTokens->leftHanded) {
            reverseFaces(&mesh);
        }
        if (source.GetPointsAttr().ValueMightBeTimeVarying()) {
            notes_.push_back("mesh " + name + " has keyed points, and is read at rest");
        }
        scene_.meshes.push_back(std::move(mesh));
        return scene_.meshes.size() - 1;
    }

    /**
     * The curves of a BasisCurves prim. A cubic basis other than Bezier, B-spline and Catmull-Rom
     * is noted and its prim made a group. A curve its type, basis and wrap make nothing of is
     * left out, and noted.
     **/
    std::optional<std::size_t> addBasisCurves(const pxr::UsdGeomBasisCurves& source, const std::string& name) {
        const Form form{ .type = valueOf(source.GetTypeAttr(), rest_, pxr::UsdGeomTokens->cubic),
            .basis = valueOf(source.GetBasisAttr(), rest_, pxr::UsdGeomTokens->bezier),
            .wrap = valueOf(source.GetWrapAttr(), rest_, pxr::UsdGeomTokens->nonperiodic) };
        if (form.type == pxr::UsdGeomTokens->cubic && form.basis != pxr::UsdGeomTokens->bezier &&
            form.basis != pxr::UsdGeomTokens->bspline && form.basis != pxr::UsdGeomTokens->catmullRom) {
            notes_.push_back("curves " + name + " have a " + form.basis.GetString() + " basis, which the scene cannot hold");
            return std::nullopt;
        }
        return addCurves(source, name, [&form](const std::vector<glm::vec3>& points, std::size_t /*curve*/) {
            return basisCurveOf(points, form);
        });
    }

    /**
     * The curves of a NurbsCurves prim. Each takes its order, its run of the knots and, when the
     * prim has them, its points' weights.
     **/
    std::optional<std::size_t> addNurbsCurves(const pxr::UsdGeomNurbsCurves& source, const std::string& name) {
        const pxr::VtArray<int> orders = valueOf(source.GetOrderAttr(), rest_, pxr::VtArray<int>());
        const pxr::VtArray<double> knots = valueOf(source.GetKnotsAttr(), rest_, pxr::VtArray<double>());
        const pxr::VtArray<double> weights = valueOf(source.GetPointWeightsAttr(), rest_, pxr::VtArray<double>());
        std::size_t knot = 0;
        std::size_t weight = 0;
        return addCurves(source, name, [&](const std::vector<glm::vec3>& points, std::size_t curve) -> std::optional<v3d::curve::Curve> {
            if (curve >= orders.size() || orders[curve] < 2) {
                return std::nullopt;
            }
            v3d::curve::Nurbs nurbs;
            nurbs.order = static_cast<unsigned int>(orders[curve]);
            nurbs.points = points;
            const std::size_t needed = points.size() + nurbs.order;
            if (knot + needed > knots.size()) {
                return std::nullopt;
            }
            for (std::size_t each = knot; each < knot + needed; each++) {
                nurbs.knots.push_back(static_cast<float>(knots[each]));
            }
            knot += needed;
            if (!weights.empty()) {
                if (weight + points.size() > weights.size()) {
                    return std::nullopt;
                }
                for (std::size_t each = weight; each < weight + points.size(); each++) {
                    nurbs.weights.push_back(static_cast<float>(weights[each]));
                }
                weight += points.size();
            }
            return v3d::curve::Curve{ .shape = nurbs };
        });
    }

    /**
     * The curves of a Curves prim, each made from its run of the points by make, and their
     * width, the mean of the prim's widths. A curve make gives nothing for, or one
     * curve::problem() finds fault with, is left out.
     *
     * @return the curves' index in the scene, or nothing when the prim gave no curve
     **/
    template <typename Make>
    std::optional<std::size_t> addCurves(const pxr::UsdGeomCurves& source, const std::string& name, Make make) {
        const pxr::VtArray<pxr::GfVec3f> points = valueOf(source.GetPointsAttr(), rest_, pxr::VtArray<pxr::GfVec3f>());
        const pxr::VtArray<int> counts = valueOf(source.GetCurveVertexCountsAttr(), rest_, pxr::VtArray<int>());
        const pxr::VtArray<float> widths = valueOf(source.GetWidthsAttr(), rest_, pxr::VtArray<float>());
        Curves curves{ .name = name };
        std::size_t first = 0;
        std::size_t skipped = 0;
        for (std::size_t curve = 0; curve < counts.size(); curve++) {
            const int count = counts[curve];
            if (count < 0 || first + static_cast<std::size_t>(count) > points.size()) {
                skipped += counts.size() - curve;
                break;
            }
            const std::optional<v3d::curve::Curve> made = make(toGlm(points, first, static_cast<std::size_t>(count)), curve);
            first += static_cast<std::size_t>(count);
            if (made && v3d::curve::problem(*made).empty()) {
                curves.curves.push_back(*made);
            } else {
                skipped++;
            }
        }
        if (skipped > 0) {
            notes_.push_back(std::to_string(skipped) + " of the curves of " + name + " do not make a curve, and are left out");
        }
        if (source.GetPointsAttr().ValueMightBeTimeVarying()) {
            notes_.push_back("curves " + name + " have keyed points, and are read at rest");
        }
        if (curves.curves.empty()) {
            return std::nullopt;
        }
        if (!widths.empty()) {
            float total = 0.0F;
            for (const float width : widths) {
                total += width;
            }
            curves.width = total / static_cast<float>(widths.size());
        }
        scene_.curves.push_back(std::move(curves));
        return scene_.curves.size() - 1;
    }

    /**
     * A left handed mesh runs clockwise seen from outside, so each face is turned round.
     **/
    static void reverseFaces(Mesh* mesh) {
        std::size_t first = 0;
        for (const std::uint32_t count : mesh->counts) {
            if (first + count > mesh->indices.size()) {
                return;
            }
            const auto start = mesh->indices.begin() + static_cast<std::ptrdiff_t>(first);
            std::reverse(start, start + static_cast<std::ptrdiff_t>(count));
            first += count;
        }
    }

    std::size_t addCamera(const pxr::UsdGeomCamera& source, const std::string& name) {
        const pxr::GfCamera lens = source.GetCamera(rest_);
        Camera camera;
        camera.name = name;
        if (lens.GetProjection() == pxr::GfCamera::Orthographic) {
            camera.projection = Camera::Projection::Orthographic;
            camera.magnification = glm::vec2(static_cast<float>(lens.GetHorizontalAperture() * APERTURE_UNIT * metres_ / 2.0),
                static_cast<float>(lens.GetVerticalAperture() * APERTURE_UNIT * metres_ / 2.0));
        } else {
            camera.yfov = glm::radians(lens.GetFieldOfView(pxr::GfCamera::FOVVertical));
            camera.aspect = lens.GetAspectRatio();
        }
        const pxr::GfRange1f clipping = lens.GetClippingRange();
        camera.clipping = glm::vec2(static_cast<float>(clipping.GetMin() * metres_), static_cast<float>(clipping.GetMax() * metres_));
        scene_.cameras.push_back(std::move(camera));
        return scene_.cameras.size() - 1;
    }

    std::optional<std::size_t> addLight(const pxr::UsdPrim& prim, const std::string& name) {
        const pxr::UsdLuxLightAPI source(prim);
        const float exposure = valueOf(source.GetExposureAttr(), rest_, 0.0F);
        const float power = valueOf(source.GetIntensityAttr(), rest_, 1.0F) * std::exp2(exposure);
        const pxr::GfVec3f colour = valueOf(source.GetColorAttr(), rest_, pxr::GfVec3f(1.0F));
        Light light;
        light.name = name;
        light.colour = glm::vec3(colour[0], colour[1], colour[2]);
        if (prim.IsA<pxr::UsdLuxDistantLight>()) {
            light.type = Light::Type::Distant;
            light.intensity = power;
        } else if (prim.IsA<pxr::UsdLuxSphereLight>()) {
            light.intensity = sphereIntensity(pxr::UsdLuxSphereLight(prim), power, valueOf(source.GetNormalizeAttr(), rest_, false));
            shape(prim, &light);
        } else {
            notes_.push_back("light " + name + " is a " + prim.GetTypeName().GetString() + ", which the scene cannot hold");
            return std::nullopt;
        }
        scene_.lights.push_back(std::move(light));
        return scene_.lights.size() - 1;
    }

    /**
     * A sphere's power is its radiance times pi times its surface, or times pi alone when it is
     * normalized or has no size. Spread over a sphere of directions, that is watts per steradian.
     **/
    float sphereIntensity(const pxr::UsdLuxSphereLight& sphere, float radiance, bool normalized) const {
        const auto radius = static_cast<float>(valueOf(sphere.GetRadiusAttr(), rest_, 0.0F) * metres_);
        const float area = normalized || radius <= 0.0F ? 1.0F : SPHERE * radius * radius;
        const float watts = radiance * glm::pi<float>() * area;
        return watts / SPHERE;
    }

    void shape(const pxr::UsdPrim& prim, Light* light) const {
        if (!prim.HasAPI<pxr::UsdLuxShapingAPI>()) {
            return;
        }
        const pxr::UsdLuxShapingAPI shaping(prim);
        const float angle = valueOf(shaping.GetShapingConeAngleAttr(), rest_, WIDEST_SPOT);
        if (angle >= WIDEST_SPOT) {
            return;
        }
        const float softness = std::clamp(valueOf(shaping.GetShapingConeSoftnessAttr(), rest_, 0.0F), 0.0F, 1.0F);
        light->type = Light::Type::Spot;
        light->outerCone = glm::radians(angle);
        light->innerCone = light->outerCone * (1.0F - softness);
    }

    /**
     * A keyed placement, read at every time the stage samples it. A rotation keeps to the same
     * side as the key before, so linear keys turn the short way.
     **/
    void addKeys(const pxr::UsdGeomXformable& xformable, std::size_t node) {
        std::vector<double> times;
        xformable.GetTimeSamples(&times);
        Channel translation;
        Channel rotation;
        Channel scale;
        translation.path = Channel::Path::Translation;
        rotation.path = Channel::Path::Rotation;
        scale.path = Channel::Path::Scale;
        for (const double time : times) {
            bool resets = false;
            const v3d::type::Transform placement = placementOf(localAt(xformable, pxr::UsdTimeCode(time), &resets));
            const auto seconds = static_cast<float>(time / perSecond_);
            glm::quat turn = placement.rotation;
            if (!rotation.values.empty()) {
                const glm::vec4& before = rotation.values.back();
                if (glm::dot(glm::quat(before.w, before.x, before.y, before.z), turn) < 0.0F) {
                    turn = -turn;
                }
            }
            for (Channel* channel : { &translation, &rotation, &scale }) {
                channel->times.push_back(seconds);
            }
            translation.values.emplace_back(placement.position, 0.0F);
            rotation.values.emplace_back(turn.x, turn.y, turn.z, turn.w);
            scale.values.emplace_back(placement.scale, 0.0F);
        }
        for (Channel* channel : { &translation, &rotation, &scale }) {
            if (channel->values.size() > 1 && !constant(channel->values)) {
                channel->node = node;
                scene_.channels.push_back(std::move(*channel));
            }
        }
    }

    pxr::UsdStageRefPtr stage_;
    pxr::UsdTimeCode rest_;
    double perSecond_;
    double metres_;
    glm::mat4 upright_;
    Scene scene_;
    std::vector<std::string> notes_;
    std::unordered_map<pxr::SdfPath, std::size_t, pxr::SdfPath::Hash> nodes_;
    std::optional<std::size_t> stageGroup_;
};

};  // namespace

Imported readUsd(const std::string& path) {
    Imported imported;
    const pxr::UsdStageRefPtr stage = pxr::UsdStage::Open(path);
    if (!stage) {
        imported.problem = "could not open " + path + " as USD";
        return imported;
    }
    return Reader(stage).read();
}

};  // namespace v3d::interchange
