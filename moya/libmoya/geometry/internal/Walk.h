/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/geometry/Triangulate.h>
#include <moya/libmoya/geometry/World.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace v3d::log {
class Logger;
};  // namespace v3d::log

namespace v3d::moya::geometry::internal {

/**
 * A walk down the scene's instances from the world, which places the triangles of every mesh a
 * chain of instances reaches. flatten() makes one and gives back what it placed.
 **/
class Walk final {
 public:
    Walk(const scene::Scene & scene, const boost::shared_ptr<log::Logger> & logger);

    /**
     * Places every instance in a group, or in the world for a handle that names none, inside a
     * transformation to world space, after the chain of instances that reached the group.
     **/
    void group(const scene::Handle & group, const glm::mat4 & toWorld, std::vector<std::uint32_t> & chain);

    /** What the walk placed, which it gives up. **/
    Flat take();

 private:
    /** A mesh's triangles, cut once however many chains place it. **/
    struct Mesh {
        std::vector<glm::vec3> points;
        std::vector<std::int32_t> indices;
        Triangles triangles;
    };

    /** The key of the world among the groups, which no group's index reaches. **/
    static constexpr std::uint32_t kWorld = 0xffffffffU;

    /**
     * The instances of each group by the group's index, in the order of their own indices. An
     * instance whose group is removed is in neither its group nor the world.
     **/
    static std::map<std::uint32_t, std::vector<scene::Handle>> byGroup(const scene::Scene & scene);

    /** Reports a message once, however many chains meet it. **/
    void report(const std::string & message);

    void place(const scene::Handle & instance, const glm::mat4 & parent, std::vector<std::uint32_t> & chain);
    void mesh(const scene::Handle & handle, const glm::mat4 & toWorld, const std::vector<std::uint32_t> & chain);
    const Mesh & cutMesh(const scene::Handle & handle);

    const scene::Scene & scene_;
    boost::shared_ptr<log::Logger> logger_;
    std::map<std::uint32_t, std::vector<scene::Handle>> groups_;
    std::map<std::uint32_t, Mesh> meshes_;
    std::set<std::string> reported_;
    Flat flat_;

    /** The groups the walk is inside, outermost first. **/
    std::vector<std::uint32_t> open_;
};

};  // namespace v3d::moya::geometry::internal
