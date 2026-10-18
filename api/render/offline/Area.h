/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::render::offline {

/**
 * The geometry of an area light, in world space: the triangles and spheres a scene declared
 * after `AreaLightSource` in the same attribute block. The light shader runs at points sampled
 * on it.
 **/
class Area final {
 public:
    /** A sphere of the geometry, whole, whatever slab and sweep the scene cut it to. **/
    class Sphere final {
     public:
        /** Its own space into world space. **/
        glm::mat4x4 placement = glm::mat4x4(1.0F);
        float radius = 1.0F;
    };

    /**
     * @param handle the light's handle, which the seed of its samples is made from, so two
     *        lights at one shading point do not share their samples
     **/
    explicit Area(const std::string & handle);

    void add(const glm::vec3 & first, const glm::vec3 & second, const glm::vec3 & third);
    void add(const Sphere & sphere);

    bool empty() const;
    /** The area of all of it, in world units squared. **/
    float area() const;

    /**
     * A point on the geometry from two numbers in [0, 1), spread evenly by area across every
     * piece. A sphere under a scale that is not uniform is weighted by its mean scale.
     **/
    glm::vec3 sample(const glm::vec2 & square) const;

    /** How many points a shading point samples the light at. **/
    unsigned int samples() const;
    void samples(unsigned int count);

    /** A number made from the handle, which seeds the light's samples with the shading point. **/
    std::uint64_t key() const;

 private:
    struct Corners {
        glm::vec3 a;
        glm::vec3 b;
        glm::vec3 c;
    };

    std::vector<Corners> triangles_;
    std::vector<Sphere> spheres_;
    /** The running sum of the pieces' areas, triangles first and then spheres. **/
    std::vector<float> running_;
    unsigned int samples_ = 16;
    std::uint64_t key_ = 0;
};

};  // namespace v3d::render::offline
