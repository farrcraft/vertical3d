/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/curve/Curve.h>
#include <api/interchange/Scene.h>
#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/array.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/common.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usdGeom/basisCurves.h>
#include <pxr/usd/usdGeom/camera.h>
#include <pxr/usd/usdGeom/curves.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/nurbsCurves.h>
#include <pxr/usd/usdGeom/xformable.h>
#include <pxr/usd/usdLux/sphereLight.h>

#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/gtc/constants.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::interchange::usd::internal {

/**
 * Fills a scene from a stage, as usd::read() describes. A reader reads one stage once.
 **/
class Reader final {
 public:
    explicit Reader(const pxr::UsdStageRefPtr& stage);

    Imported read();

 private:
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
     * A camera's apertures are in tenths of the stage's unit.
     **/
    static constexpr double APERTURE_UNIT = 0.1;
    /**
     * A shaping cone of a quarter turn or more lights the whole half space in front of it. A
     * sphere light with no shaping lights that side the same way, so only a narrower cone is a
     * spot.
     **/
    static constexpr float WIDEST_SPOT = 90.0F;
    /**
     * The solid angle of a whole sphere, which is also its surface over the square of its radius.
     **/
    static constexpr float SPHERE = 4.0F * glm::pi<float>();
    /**
     * The order of a cubic curve, and how far apart a cubic Bezier curve's points on the curve
     * are.
     **/
    static constexpr unsigned int CUBIC_ORDER = 4;
    static constexpr std::size_t BEZIER_STEP = 3;

    /**
     * A GfMatrix4d is row major and turns a row vector, so its rows are glm's columns, and the
     * elements copy across in place.
     **/
    static glm::mat4 toGlm(const pxr::GfMatrix4d& matrix);
    /**
     * A run of count points from first.
     **/
    static std::vector<glm::vec3> toGlm(const pxr::VtArray<pxr::GfVec3f>& points, std::size_t first, std::size_t count);
    /**
     * An attribute's value at a time, or a fallback when it has none there.
     **/
    template <typename T>
    static T valueOf(const pxr::UsdAttribute& attribute, pxr::UsdTimeCode time, const T& fallback);
    /**
     * The same values in every key, which leave a part of a placement unkeyed.
     **/
    static bool constant(const std::vector<glm::vec4>& values);
    /**
     * Each end point reflected through its neighbour and added beyond the end. A uniform B-spline
     * or Catmull-Rom curve over the longer list starts and ends on the given end points. USD's
     * pinned wrap asks for that.
     **/
    static std::vector<glm::vec3> pinned(const std::vector<glm::vec3>& points);
    /**
     * A point a Bezier curve passes through, with free handles that start on it.
     **/
    static v3d::curve::BezierPoint freePoint(const glm::vec3& position);
    /**
     * A cubic Bezier curve from its basis curve points: a point on the curve, then its right
     * handle and the next point's left one, and so on. A periodic curve's last two handles lead
     * back to its first point.
     **/
    static std::optional<v3d::curve::Curve> bezierOf(const std::vector<glm::vec3>& points, bool periodic);
    /**
     * An open Catmull-Rom basis curve as the Bezier curve it is. It runs from its second point
     * to its second to last, and each segment's handles lie a sixth of the way from one of its
     * neighbours to the other.
     **/
    static std::optional<v3d::curve::Curve> catmullRomOf(const std::vector<glm::vec3>& points);
    /**
     * An open uniform cubic B-spline basis curve, as a NURBS curve over evenly spaced knots,
     * which is the same curve.
     **/
    static v3d::curve::Curve bsplineOf(const std::vector<glm::vec3>& points);
    /**
     * One curve of a BasisCurves prim, or nothing when its form makes no curve of its points.
     **/
    static std::optional<v3d::curve::Curve> basisCurveOf(const std::vector<glm::vec3>& points, const Form& form);
    /**
     * @param resets set to whether the prim discards its parent's placement
     **/
    static glm::mat4 localAt(const pxr::UsdGeomXformable& xformable, pxr::UsdTimeCode time, bool* resets);
    /**
     * A left handed mesh runs clockwise seen from outside, so each face is turned round.
     **/
    static void reverseFaces(Mesh* mesh);

    /**
     * Read where a prim is shown into its node. An invisible prim is neither seen nor rendered,
     * and a guide or a proxy is seen and not rendered.
     **/
    void shownOf(const pxr::UsdPrim& prim, Node* node) const;

    void addPrim(const pxr::UsdPrim& prim);
    /**
     * The group that turns a keyed prim at the top of a z up stage, made when one is first met.
     **/
    std::size_t stageNode();
    std::size_t addMesh(const pxr::UsdGeomMesh& source, const std::string& name);
    /**
     * The curves of a BasisCurves prim. A cubic basis other than Bezier, B-spline and Catmull-Rom
     * is noted and its prim made a group. A curve its type, basis and wrap make nothing of is
     * left out, and noted.
     **/
    std::optional<std::size_t> addBasisCurves(const pxr::UsdGeomBasisCurves& source, const std::string& name);
    /**
     * The curves of a NurbsCurves prim. Each takes its order, its run of the knots and, when the
     * prim has them, its points' weights.
     **/
    std::optional<std::size_t> addNurbsCurves(const pxr::UsdGeomNurbsCurves& source, const std::string& name);
    /**
     * The curves of a Curves prim, each made from its run of the points by make, and their
     * width, the mean of the prim's widths. A curve make gives nothing for, or one
     * curve::problem() finds fault with, is left out.
     *
     * @return the curves' index in the scene, or nothing when the prim gave no curve
     **/
    template <typename Make>
    std::optional<std::size_t> addCurves(const pxr::UsdGeomCurves& source, const std::string& name, Make make);
    std::size_t addCamera(const pxr::UsdGeomCamera& source, const std::string& name);
    std::optional<std::size_t> addLight(const pxr::UsdPrim& prim, const std::string& name);
    /**
     * A sphere's power is its radiance times pi times its surface, or times pi alone when it is
     * normalized or has no size. Spread over a sphere of directions, that is watts per steradian.
     **/
    float sphereIntensity(const pxr::UsdLuxSphereLight& sphere, float radiance, bool normalized) const;
    void shape(const pxr::UsdPrim& prim, Light* light) const;
    /**
     * A keyed placement, read at every time the stage samples it. A rotation keeps to the same
     * side as the key before, so linear keys turn the short way.
     **/
    void addKeys(const pxr::UsdGeomXformable& xformable, std::size_t node);

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

};  // namespace v3d::interchange::usd::internal
