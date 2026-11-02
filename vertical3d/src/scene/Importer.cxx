/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Importer.h"

#include <api/animation/Curve.h>
#include <api/brep/BRep.h>
#include <api/brep/Builder.h>
#include <api/curve/Curve.h>
#include <api/curve/Curves.h>
#include <api/interchange/Scene.h>
#include <api/type/Transform.h>
#include <vertical3d/src/scene/item/Camera.h>
#include <vertical3d/src/scene/item/Curve.h>
#include <vertical3d/src/scene/item/Group.h>
#include <vertical3d/src/scene/Import.h>
#include <vertical3d/src/scene/item/Item.h>
#include <vertical3d/src/scene/item/Light.h>
#include <vertical3d/src/scene/item/Mesh.h>
#include <vertical3d/src/scene/Properties.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor::scene {

/**
 **/
Importer::Importer(const interchange::Scene& scene, float rate) :
    scene_(scene), rate_(rate), meshes_(scene.meshes.size()), built_(scene.meshes.size(), false) {
}

/**
 **/
Import Importer::run() {
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

/**
 **/
glm::quat Importer::halfTurn() {
    return glm::angleAxis(glm::pi<float>(), glm::vec3(0.0F, 1.0F, 0.0F));
}

/**
 **/
item::Camera::Lens Importer::lensOf(const interchange::Camera& camera) {
    item::Camera::Lens lens;
    if (camera.projection == interchange::Camera::Projection::Orthographic) {
        lens.projection = item::Camera::Projection::Orthographic;
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

/**
 **/
item::Light::Emission Importer::emissionOf(const interchange::Light& light) {
    item::Light::Emission emission;
    if (light.type == interchange::Light::Type::Distant) {
        emission.type = item::Light::Type::Distant;
    } else if (light.type == interchange::Light::Type::Spot) {
        emission.type = item::Light::Type::Spot;
        emission.cone = light.outerCone;
        emission.delta = light.outerCone - light.innerCone;
    }
    emission.colour = light.colour;
    emission.intensity = light.intensity;
    return emission;
}

/**
 **/
glm::quat Importer::parentCorrection(const interchange::Node& node) const {
    return node.parent && *node.parent < corrections_.size() ? corrections_[*node.parent] : glm::identity<glm::quat>();
}

/**
 **/
std::size_t Importer::addNode(const interchange::Node& node, std::size_t index, const std::vector<std::size_t>& itemOf) {
    std::vector<boost::shared_ptr<item::Item>> made;
    if (node.mesh) {
        const boost::shared_ptr<v3d::brep::BRep> geometry = mesh(*node.mesh);
        if (geometry) {
            made.push_back(boost::make_shared<item::Mesh>(geometry));
        }
    }
    if (node.camera && *node.camera < scene_.cameras.size()) {
        const boost::shared_ptr<item::Camera> camera = boost::make_shared<item::Camera>();
        const item::Camera::Lens lens = lensOf(scene_.cameras[*node.camera]);
        if (lens.valid()) {
            camera->lens(lens);
        } else {
            import_.notes.push_back("camera " + scene_.cameras[*node.camera].name + " has a lens the editor cannot use, so it has a new camera's");
        }
        made.push_back(camera);
    }
    if (node.light && *node.light < scene_.lights.size()) {
        const boost::shared_ptr<item::Light> light = boost::make_shared<item::Light>();
        item::Light::Emission emission = emissionOf(scene_.lights[*node.light]);
        if (!emission.valid()) {
            import_.notes.push_back("light " + scene_.lights[*node.light].name + " has a value the editor cannot use, so it has a new light's");
            const item::Light::Type type = emission.type;
            emission = item::Light::Emission();
            emission.type = type;
        }
        light->emission(emission);
        made.push_back(light);
    }
    if (node.curves && *node.curves < scene_.curves.size()) {
        made.push_back(curves(scene_.curves[*node.curves]));
    }
    if (made.empty()) {
        made.push_back(boost::make_shared<item::Group>());
    }
    for (const boost::shared_ptr<item::Item>& item : made) {
        item->name(node.name);
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

/**
 **/
boost::shared_ptr<v3d::editor::scene::item::Curve> Importer::curves(const interchange::Curves& read) {
    const boost::shared_ptr<v3d::curve::Curves> geometry = boost::make_shared<v3d::curve::Curves>();
    for (const v3d::curve::Curve& each : read.curves) {
        geometry->add(each);
    }
    const boost::shared_ptr<v3d::editor::scene::item::Curve> item = boost::make_shared<v3d::editor::scene::item::Curve>(geometry);
    v3d::editor::scene::item::Curve::Style style{ .renders = true };
    if (std::isfinite(read.width) && read.width > 0.0F) {
        style.width = read.width;
    }
    item->style(style);
    return item;
}

/**
 **/
bool Importer::turned(const item::Item& item) {
    return item.kind() == item::Item::Kind::Camera || item.kind() == item::Item::Kind::Light;
}

/**
 **/
std::size_t Importer::add(const boost::shared_ptr<item::Item>& item, std::optional<std::size_t> parent) {
    import_.items.push_back(item);
    import_.parents.push_back(parent);
    import_.curves.emplace_back();
    return import_.items.size() - 1;
}

/**
 **/
boost::shared_ptr<v3d::brep::BRep> Importer::mesh(std::size_t index) {
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

/**
 **/
void Importer::addChannel(const interchange::Channel& channel, std::size_t item) {
    const glm::quat back = glm::inverse(parentCorrection(scene_.nodes[channel.node]));
    const glm::quat correction = corrections_[channel.node];
    const std::size_t count = std::min(channel.times.size(), channel.values.size());
    const bool cubic = channel.interpolation == interchange::Channel::Interpolation::CubicSpline &&
        channel.inTangents.size() >= count && channel.outTangents.size() >= count;

    // three curves, one an axis, keyed in step
    std::array<std::vector<v3d::animation::Curve::Key>, 3> keys;
    glm::vec3 near(0.0F);
    for (std::size_t index = 0; index < count; index++) {
        const float frame = channel.times[index] * rate_;
        if (!std::isfinite(frame) || (!keys[0].empty() && frame - keys[0].back().frame < v3d::animation::Curve::SAME_FRAME)) {
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
            v3d::animation::Curve::Key read;
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
            import_.curves[item][std::string(part) + AXES[axis]] = v3d::animation::Curve(keys[axis]);
        } catch (const std::invalid_argument&) {
            import_.notes.push_back("node " + scene_.nodes[channel.node].name + " has keys out of order, which are left out");
            return;
        }
    }
}

/**
 **/
v3d::animation::Curve::Key Importer::key(const interchange::Channel& channel, v3d::animation::Curve::Key made, bool cubic) const {
    made.automatic = false;
    if (channel.interpolation == interchange::Channel::Interpolation::CubicSpline) {
        made.interpolation = v3d::animation::Curve::Interpolation::Bezier;
        made.automatic = !cubic || channel.path == interchange::Channel::Path::Rotation;
        made.in /= rate_;
        made.out /= rate_;
        return made;
    }
    made.interpolation = channel.interpolation == interchange::Channel::Interpolation::Step ?
        v3d::animation::Curve::Interpolation::Step : v3d::animation::Curve::Interpolation::Linear;
    made.in = 0.0F;
    made.out = 0.0F;
    return made;
}

};  // namespace v3d::editor::scene
