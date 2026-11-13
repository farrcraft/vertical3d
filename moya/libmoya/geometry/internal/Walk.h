/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/geometry/Geometry.h>
#include <moya/libmoya/geometry/Group.h>
#include <moya/libmoya/geometry/internal/Piece.h>
#include <moya/libmoya/geometry/internal/Primvars.h>
#include <moya/libmoya/geometry/internal/Programs.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/geometry/subdivision/Surface.h>
#include <moya/libmoya/geometry/subdivision/Tessellation.h>
#include <moya/libmoya/kernel/camera/Camera.h>
#include <moya/libmoya/kernel/geometry/Sampled.h>
#include <moya/libmoya/kernel/surface/Material.h>
#include <moya/libmoya/material/Compile.h>
#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>

#include <cstddef>
#include <cstdint>
#include <functional>
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

namespace v3d::moya::geometry {
class Cache;
};  // namespace v3d::moya::geometry

namespace v3d::moya::geometry::internal {

/**
 * A walk down the scene's instances from the world, which gathers every prototype a chain of
 * instances reaches. gather() makes one and gives back what it gathered.
 **/
class Walk final {
 public:
    /** A walk that places what a cache holds and keeps what it makes there, if it has one. **/
    Walk(const scene::Scene & scene, const boost::shared_ptr<log::Logger> & logger, Cache * cache);

    /** Gathers what the world's instances reach, and gives it up. **/
    Prototypes take();

 private:
    /** How many of each of the prototypes' values were held at a point of the walk. **/
    struct Mark {
        std::size_t points { 0 };
        std::size_t widths { 0 };
        std::size_t quadrics { 0 };
        std::size_t controls { 0 };
        std::size_t ends { 0 };
        std::size_t times { 0 };
        std::size_t faces { 0 };
        std::size_t primvars { 0 };
        std::size_t primvarValues { 0 };
        std::size_t reports { 0 };
    };

    /** A subdivision surface waiting to be cut, and its index among the geometries. **/
    struct Uncut {
        scene::Handle handle;
        std::uint32_t index;
        subdivision::Surface surface;
    };

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

    /**
     * Places each light with a shape through an instance in the world, after every other, as
     * gather() says.
     **/
    void lights();

    /**
     * The index among the geometries of a light's shape, in the light's own space, made once or
     * placed from the cache.
     **/
    std::uint32_t lightShape(const scene::Handle & handle);

    /** The index of a group among the groups, gathered once. **/
    std::uint32_t group(const scene::Handle & handle);

    /** The index of a material among the prototypes', gathered once from its surface node. **/
    std::uint32_t material(const scene::Handle & handle);


    /**
     * The index of a geometry among the geometries, gathered once or placed from the cache, or
     * nothing when it is left out.
     **/
    std::optional<std::uint32_t> geometry(const scene::Handle & handle, const std::string & kind);

    /** How many of each value the prototypes hold now. **/
    Mark marked() const;

    /** A geometry moved from where one mark holds its values to where another does. **/
    static Geometry shifted(Geometry geometry, const Mark & from, const Mark & to);

    /** The values a geometry was made of since a mark, apart from every other geometry's. **/
    Piece pieceOf(std::uint32_t index, const Mark & mark) const;

    /** Appends a piece's values to the prototypes, and gives its geometry where they now lie. **/
    Geometry place(const Piece & piece);

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

    /** Where a subdivision surface is placed, and the material its chain names there. **/
    struct Placement {
        glm::mat4 transform { 1.0F };
        std::uint32_t material { kernel::surface::kNoMaterial };
    };

    /**
     * Each subdivision surface's placement nearest a camera, by its index among the geometries:
     * the one that brings its points' box, grown by its displacement bound, nearest the camera.
     **/
    std::map<std::uint32_t, Placement> nearest(const kernel::camera::Camera & camera) const;

    /** The displacement of a material among the prototypes', when it displaces, or none. **/
    const material::Compiled * displacementOf(std::uint32_t material);

    /**
     * Moves the points a subdivision surface was cut into, at every sample, by a displacement run
     * at each place, as Displace.h says. Each sample moves along its own limit normals, and its
     * tangent at the first sample laid flat against them. A displacement longer than the
     * surface's bound is held to it, and reported.
     *
     * @param mark where the surface's values begin among the prototypes'
     **/
    void displace(const Uncut & uncut, const subdivision::Tessellation & tessellation, const material::Compiled & displacement, const glm::mat4 & placement,
        const Mark & mark);

    /**
     * The index of curves among the geometries, cut once, or nothing when they cannot be cut into
     * segments.
     **/
    std::optional<std::uint32_t> curves(const scene::Handle & handle);

    /**
     * Appends the primvars shading reads of a geometry's object, `N` and `Cs`, each spread over
     * the geometry's primitives by a function of its interpolation, its values and its floats per
     * value. A primvar the function refuses, or one of another width than three, is reported and
     * left out.
     **/
    void addPrimvars(const scene::Handle & handle, Geometry & geometry,
        const std::function<Spread(scene::Interpolation, const std::vector<float> &, std::size_t)> & spread);

    /** Appends one primvar of a geometry, already spread. **/
    void addPrimvar(Geometry & geometry, std::uint32_t name, std::uint32_t components, const Spread & spread, const std::optional<std::string> & space);

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
    std::map<std::uint32_t, std::uint32_t> materialIndex_;

    /** The scene's handle of each material among the prototypes', by its index there. **/
    std::vector<scene::Handle> materialHandles_;
    std::map<std::uint32_t, std::set<std::uint32_t>> held_;
    std::set<std::pair<std::uint32_t, std::uint32_t>> deepened_;
    std::set<std::string> reported_;

    /** Each gathered instance's handle, by its index among the instances. **/
    std::vector<scene::Handle> handles_;

    std::vector<Uncut> uncut_;

    /** What the walk places from and keeps, or none. **/
    Cache * cache_;

    /** The scene's index of every geometry the walk placed. **/
    std::set<std::uint32_t> placed_;

    Prototypes prototypes_;

    /** Every MaterialX material, compiled before the walk, and the primvars they read. **/
    Programs programs_;
};

};  // namespace v3d::moya::geometry::internal
