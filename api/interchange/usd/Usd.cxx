/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Usd.h"

#include <api/interchange/Placement.h>
#include <api/interchange/Scene.h>
#include <api/type/Transform.h>
#include <pxr/base/gf/camera.h>
#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/range1f.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/array.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/common.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/primFlags.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usdGeom/camera.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/metrics.h>
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
 * A shaping cone of a quarter turn or more lights the whole half space in front of it, which is
 * what a sphere light with no shaping does on that side, so only a narrower one is a spot.
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
