/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/animation/Curve.h>
#include <api/brep/BRep.h>
#include <api/interchange/Scene.h>
#include <vertical3d/src/scene/Camera.h>
#include <vertical3d/src/scene/Curve.h>
#include <vertical3d/src/scene/Import.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Light.h>

#include <array>
#include <cstddef>
#include <optional>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/ext/quaternion_float.hpp>

namespace v3d::editor {

/**
 * Builds a scene's items, its meshes once each, as importScene() describes. An importer runs
 * once.
 **/
class Importer final {
 public:
    Importer(const v3d::interchange::Scene& scene, float rate);

    Import run();

 private:
    static constexpr std::array<const char*, 3> AXES{ "x", "y", "z" };

    /**
     * Half a turn about y, which takes +z to -z and keeps +y: the turn between the way an editor
     * camera or light points and the way a file's does.
     **/
    static glm::quat halfTurn();
    /**
     * A file's perspective angle runs from the bottom of the picture to the top, and the editor's
     * across the shorter side, which is the width when the picture is taller than it is wide.
     **/
    static Camera::Lens lensOf(const v3d::interchange::Camera& camera);
    static Light::Emission emissionOf(const v3d::interchange::Light& light);

    /**
     * The turn of a node's parent's item against the parent node's frame, which the node's own
     * placement and keys are turned back by.
     **/
    glm::quat parentCorrection(const v3d::interchange::Node& node) const;

    std::size_t addNode(const v3d::interchange::Node& node, std::size_t index, const std::vector<std::size_t>& itemOf);

    /**
     * A curve item holding a node's curves. Curves a file holds are ones its tool draws, so the
     * item renders, at the file's width when it gives one that is valid.
     **/
    static boost::shared_ptr<v3d::editor::Curve> curves(const v3d::interchange::Curves& read);

    static bool turned(const Item& item);

    std::size_t add(const boost::shared_ptr<Item>& item, std::optional<std::size_t> parent);

    /**
     * A mesh's geometry, built the first time a node asks for it, or null when it cannot be.
     **/
    boost::shared_ptr<v3d::brep::BRep> mesh(std::size_t index);

    void addChannel(const v3d::interchange::Channel& channel, std::size_t item);

    /**
     * One key, from its frame, its value and the file's tangents. A file's tangent is a change a
     * second, and a slope a change a frame. A rotation keyed by a cubic spline has automatic
     * slopes, because its tangents are a quaternion's.
     **/
    v3d::animation::Curve::Key key(const v3d::interchange::Channel& channel, v3d::animation::Curve::Key made, bool cubic) const;

    const v3d::interchange::Scene& scene_;
    float rate_;
    std::vector<boost::shared_ptr<v3d::brep::BRep>> meshes_;
    std::vector<bool> built_;
    std::vector<glm::quat> corrections_;
    Import import_;
};

};  // namespace v3d::editor
