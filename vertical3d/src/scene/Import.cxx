/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Import.h"

#include <api/animation/Curve.h>
#include <api/brep/BRep.h>
#include <api/brep/Builder.h>
#include <api/interchange/Scene.h>
#include <api/type/Transform.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Camera.h"
#include "Group.h"
#include "Item.h"
#include "Light.h"
#include "Mesh.h"
#include "Properties.h"
#include "Scene.h"

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor {

namespace {

using v3d::animation::Curve;
namespace interchange = v3d::interchange;

/**
 * Half a turn about y, which takes +z to -z and keeps +y: the turn between the way an editor
 * camera or light points and the way a file's does.
 **/
glm::quat halfTurn() {
    return glm::angleAxis(glm::pi<float>(), glm::vec3(0.0F, 1.0F, 0.0F));
}

const std::array<const char*, 3> AXES{ "x", "y", "z" };

/**
 * A file's perspective angle runs from the bottom of the picture to the top, and the editor's
 * across the shorter side, which is the width when the picture is taller than it is wide.
 **/
Camera::Lens lensOf(const interchange::Camera& camera) {
    Camera::Lens lens;
    if (camera.projection == interchange::Camera::Projection::Orthographic) {
        lens.projection = Camera::Projection::Orthographic;
        lens.extent = camera.magnification.y;
    } else {
        float fov = camera.yfov;
        if (camera.aspect > 0.0F && camera.aspect < 1.0F) {
            fov = 2.0F * std::atan(std::tan(fov / 2.0F) * camera.aspect);
        }
        lens.fov = glm::degrees(fov);
    }
    lens.clipping.x = camera.clipping.x;
    // a camera with no far plane is given the far plane of a new one
    if (camera.clipping.y > 0.0F) {
        lens.clipping.y = camera.clipping.y;
    }
    return lens;
}

Light::Emission emissionOf(const interchange::Light& light) {
    Light::Emission emission;
    if (light.type == interchange::Light::Type::Distant) {
        emission.type = Light::Type::Distant;
    } else if (light.type == interchange::Light::Type::Spot) {
        emission.type = Light::Type::Spot;
        emission.cone = light.outerCone;
        emission.delta = light.outerCone - light.innerCone;
    }
    emission.colour = light.colour;
    emission.intensity = light.intensity;
    return emission;
}

/**
 * Builds a scene's items, its meshes once each.
 **/
class Importer final {
 public:
    Importer(const interchange::Scene& scene, float rate) :
        scene_(scene), rate_(rate), meshes_(scene.meshes.size()), built_(scene.meshes.size(), false) {
    }

    Import run() {
        // the item each node became, and the turn its item has against the node's own frame
        std::vector<std::size_t> itemOf(scene_.nodes.size(), 0);
        corrections_.assign(scene_.nodes.size(), glm::identity<glm::quat>());
        for (std::size_t index = 0; index < scene_.nodes.size(); index++) {
            itemOf[index] = addNode(scene_.nodes[index], index, itemOf);
        }
        for (const interchange::Channel& channel : scene_.channels) {
            if (channel.node < scene_.nodes.size()) {
                addChannel(channel, itemOf[channel.node]);
            }
        }
        return std::move(import_);
    }

 private:
    /**
     * The turn of a node's parent's item against the parent node's frame, which the node's own
     * placement and keys are turned back by.
     **/
    glm::quat parentCorrection(const interchange::Node& node) const {
        return node.parent && *node.parent < corrections_.size() ? corrections_[*node.parent] : glm::identity<glm::quat>();
    }

    std::size_t addNode(const interchange::Node& node, std::size_t index, const std::vector<std::size_t>& itemOf) {
        std::vector<boost::shared_ptr<Item>> made;
        if (node.mesh) {
            const boost::shared_ptr<v3d::brep::BRep> geometry = mesh(*node.mesh);
            if (geometry) {
                made.push_back(boost::make_shared<Mesh>(geometry));
            }
        }
        if (node.camera && *node.camera < scene_.cameras.size()) {
            const boost::shared_ptr<Camera> camera = boost::make_shared<Camera>();
            const Camera::Lens lens = lensOf(scene_.cameras[*node.camera]);
            if (lens.valid()) {
                camera->lens(lens);
            } else {
                import_.notes.push_back("camera " + scene_.cameras[*node.camera].name + " has a lens the editor cannot use, so it has a new camera's");
            }
            made.push_back(camera);
        }
        if (node.light && *node.light < scene_.lights.size()) {
            const boost::shared_ptr<Light> light = boost::make_shared<Light>();
            Light::Emission emission = emissionOf(scene_.lights[*node.light]);
            if (!emission.valid()) {
                import_.notes.push_back("light " + scene_.lights[*node.light].name + " has a value the editor cannot use, so it has a new light's");
                const Light::Type type = emission.type;
                emission = Light::Emission();
                emission.type = type;
            }
            light->emission(emission);
            made.push_back(light);
        }
        if (made.empty()) {
            made.push_back(boost::make_shared<Group>());
        }

        corrections_[index] = turned(*made.front()) ? halfTurn() : glm::identity<glm::quat>();
        const glm::quat back = glm::inverse(parentCorrection(node));
        v3d::type::Transform placement = node.placement;
        placement.position = back * placement.position;
        placement.rotation = back * placement.rotation * corrections_[index];
        made.front()->placement(placement);
        std::optional<std::size_t> parent;
        if (node.parent && *node.parent < itemOf.size()) {
            parent = itemOf[*node.parent];
        }
        const std::size_t first = add(made.front(), parent);

        // the others sit in the first one's frame, turned as their own kind needs
        for (std::size_t other = 1; other < made.size(); other++) {
            v3d::type::Transform beside;
            beside.rotation = glm::inverse(corrections_[index]) * (turned(*made[other]) ? halfTurn() : glm::identity<glm::quat>());
            made[other]->placement(beside);
            add(made[other], first);
        }
        return first;
    }

    static bool turned(const Item& item) {
        return item.kind() == Item::Kind::Camera || item.kind() == Item::Kind::Light;
    }

    std::size_t add(const boost::shared_ptr<Item>& item, std::optional<std::size_t> parent) {
        import_.items.push_back(item);
        import_.parents.push_back(parent);
        import_.curves.emplace_back();
        return import_.items.size() - 1;
    }

    /**
     * A mesh's geometry, built the first time a node asks for it, or null when it cannot be.
     **/
    boost::shared_ptr<v3d::brep::BRep> mesh(std::size_t index) {
        if (index >= meshes_.size()) {
            return {};
        }
        if (!built_[index]) {
            built_[index] = true;
            const interchange::Mesh& source = scene_.meshes[index];
            v3d::brep::BuildOptions options;
            options.winding = v3d::brep::Winding::CounterClockwise;
            options.joinTriangles = source.triangulated;
            const v3d::brep::Built built = v3d::brep::build({ source.points, source.counts, source.indices }, options);
            if (!built.mesh) {
                import_.notes.push_back("mesh " + source.name + " could not be built, because " + built.problem +
                    ", so its node is a group");
            } else if (built.skipped > 0) {
                import_.notes.push_back("mesh " + source.name + " has " + std::to_string(built.skipped) +
                    " faces with no area, which are left out");
            }
            meshes_[index] = built.mesh;
        }
        return meshes_[index];
    }

    void addChannel(const interchange::Channel& channel, std::size_t item) {
        const glm::quat back = glm::inverse(parentCorrection(scene_.nodes[channel.node]));
        const glm::quat correction = corrections_[channel.node];
        const std::size_t count = std::min(channel.times.size(), channel.values.size());
        const bool cubic = channel.interpolation == interchange::Channel::Interpolation::CubicSpline &&
            channel.inTangents.size() >= count && channel.outTangents.size() >= count;

        // three curves, one an axis, keyed in step
        std::array<std::vector<Curve::Key>, 3> keys;
        glm::vec3 near(0.0F);
        for (std::size_t index = 0; index < count; index++) {
            const float frame = channel.times[index] * rate_;
            if (!std::isfinite(frame) || (!keys[0].empty() && frame - keys[0].back().frame < Curve::SAME_FRAME)) {
                continue;
            }
            const glm::vec4& raw = channel.values[index];
            glm::vec3 value(raw);
            glm::vec3 in(0.0F);
            glm::vec3 out(0.0F);
            if (channel.path == interchange::Channel::Path::Rotation) {
                const glm::quat rotation = back * glm::quat(raw.w, raw.x, raw.y, raw.z) * correction;
                value = eulerNear(glm::normalize(rotation), near);
                near = value;
            } else if (channel.path == interchange::Channel::Path::Translation) {
                value = back * value;
                if (cubic) {
                    in = back * glm::vec3(channel.inTangents[index]);
                    out = back * glm::vec3(channel.outTangents[index]);
                }
            } else if (cubic) {
                in = glm::vec3(channel.inTangents[index]);
                out = glm::vec3(channel.outTangents[index]);
            }
            for (std::size_t axis = 0; axis < 3; axis++) {
                const auto at = static_cast<glm::length_t>(axis);
                Curve::Key read;
                read.frame = frame;
                read.value = value[at];
                read.in = in[at];
                read.out = out[at];
                keys[axis].push_back(key(channel, read, cubic));
            }
        }

        const char* part = "translation.";
        if (channel.path == interchange::Channel::Path::Rotation) {
            part = "rotation.";
        } else if (channel.path == interchange::Channel::Path::Scale) {
            part = "scale.";
        }
        for (std::size_t axis = 0; axis < 3; axis++) {
            if (keys[axis].empty()) {
                continue;
            }
            try {
                import_.curves[item][std::string(part) + AXES[axis]] = Curve(keys[axis]);
            } catch (const std::invalid_argument&) {
                import_.notes.push_back("node " + scene_.nodes[channel.node].name + " has keys out of order, which are left out");
                return;
            }
        }
    }

    /**
     * One key, from its frame, its value and the file's tangents. A file's tangent is a change a
     * second, and a slope a change a frame. A rotation keyed by a cubic spline has automatic
     * slopes, because its tangents are a quaternion's.
     **/
    Curve::Key key(const interchange::Channel& channel, Curve::Key made, bool cubic) const {
        made.automatic = false;
        if (channel.interpolation == interchange::Channel::Interpolation::CubicSpline) {
            made.interpolation = Curve::Interpolation::Bezier;
            made.automatic = !cubic || channel.path == interchange::Channel::Path::Rotation;
            made.in /= rate_;
            made.out /= rate_;
            return made;
        }
        made.interpolation = channel.interpolation == interchange::Channel::Interpolation::Step ?
            Curve::Interpolation::Step : Curve::Interpolation::Linear;
        made.in = 0.0F;
        made.out = 0.0F;
        return made;
    }

    const interchange::Scene& scene_;
    float rate_;
    std::vector<boost::shared_ptr<v3d::brep::BRep>> meshes_;
    std::vector<bool> built_;
    std::vector<glm::quat> corrections_;
    Import import_;
};

};  // namespace

Import importScene(const interchange::Scene& scene, float rate) {
    return Importer(scene, rate).run();
}

};  // namespace v3d::editor
