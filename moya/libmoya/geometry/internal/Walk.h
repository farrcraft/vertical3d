/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/geometry/Group.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/geometry/Surface.h>
#include <moya/libmoya/geometry/Tessellation.h>
#include <moya/libmoya/kernel/camera/Camera.h>
#include <moya/libmoya/kernel/geometry/Sampled.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <glm/mat4x4.hpp>

namespace v3d::log {
class Logger;
};  // namespace v3d::log

namespace v3d::moya::geometry::internal {

/**
 * A walk down the scene's instances from the world, which gathers every prototype a chain of
 * instances reaches. gather() makes one and gives back what it gathered.
 **/
class Walk final {
 public:
    Walk(const scene::Scene & scene, const boost::shared_ptr<log::Logger> & logger);

    /** Gathers what the world's instances reach, and gives it up. **/
    Prototypes take();

 private:
    /** The key of the world among the groups, which no group's index reaches. **/
    static constexpr std::uint32_t kWorld = 0xffffffffU;

    /**
     * The instances of each group by the group's index, in the order of their own indices. An
     * instance whose group is removed is in neither its group nor the world.
     **/
    static std::map<std::uint32_t, std::vector<scene::Handle>> byGroup(const scene::Scene & scene);

    /** Reports a message once, however many instances meet it. **/
    void report(const std::string & message);

    /** The instances a group holds, or the world for a handle that names none. **/
    Group members(const scene::Handle & group);

    /** Gathers an instance a group holds, and gives its index, or nothing when it is left out. **/
    std::optional<std::uint32_t> instance(const scene::Handle & handle, const scene::Handle & holder);

    /** The index of a group among the groups, gathered once. **/
    std::uint32_t group(const scene::Handle & handle);

    /**
     * The index of a mesh among the geometries, cut once, or nothing when its faces cannot be
     * cut from its points and indices.
     **/
    std::optional<std::uint32_t> mesh(const scene::Handle & handle);

    /** The index of a point cloud among the geometries, gathered once. **/
    std::uint32_t cloud(const scene::Handle & handle);

    /** The index of a quadric among the geometries, gathered once. **/
    std::uint32_t quadric(const scene::Handle & handle);

    /**
     * The index of a subdivision surface among the geometries, or nothing when OpenSubdiv cannot
     * refine its topology. It is cut into triangles once every instance is gathered.
     **/
    std::optional<std::uint32_t> subdivision(const scene::Handle & handle);

    /** Cuts every subdivision surface the instances reach, each where it is nearest the camera. **/
    void cutSurfaces();

    /**
     * Each subdivision surface's placement nearest a camera, by its index among the geometries:
     * the one that brings its points' box, grown by its displacement bound, nearest the camera.
     **/
    std::map<std::uint32_t, glm::mat4> nearest(const kernel::camera::Camera & camera) const;

    /**
     * Each place's height above a subdivision surface, from its displacement primvar and held
     * within its bound, or none for a surface that is not displaced.
     **/
    std::vector<float> heights(const scene::Handle & handle, const Surface & surface, const Tessellation & tessellation);

    /**
     * The index of curves among the geometries, cut once, or nothing when they cannot be cut into
     * segments.
     **/
    std::optional<std::uint32_t> curves(const scene::Handle & handle);

    /**
     * Where values are held from first, count for each of a list of samples' times, which the
     * prototypes keep when there is more than one.
     **/
    kernel::geometry::Sampled sampled(std::size_t first, std::size_t count, const std::vector<float> & times);

    /** Every group a group holds, directly or through another group. **/
    const std::set<std::uint32_t> & held(std::uint32_t group);

    /** Reports each instance of a group that a chain of a depth would place too deep. **/
    void deepen(const Group & group, std::uint32_t depth);

    const scene::Scene & scene_;
    boost::shared_ptr<log::Logger> logger_;
    std::map<std::uint32_t, std::vector<scene::Handle>> groups_;
    std::map<std::uint32_t, std::uint32_t> groupIndex_;
    std::map<std::uint32_t, std::uint32_t> geometryIndex_;
    std::map<std::uint32_t, std::set<std::uint32_t>> held_;
    std::set<std::pair<std::uint32_t, std::uint32_t>> deepened_;
    std::set<std::string> reported_;

    /** Each gathered instance's handle, by its index among the instances. **/
    std::vector<scene::Handle> handles_;

    /** A subdivision surface waiting to be cut, and its index among the geometries. **/
    struct Uncut {
        scene::Handle handle;
        std::uint32_t index;
        Surface surface;
    };

    std::vector<Uncut> uncut_;

    Prototypes prototypes_;
};

};  // namespace v3d::moya::geometry::internal
