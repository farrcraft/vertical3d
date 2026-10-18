/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>

#include <glm/glm.hpp>

namespace v3d::moya {
class Vertex {
 public:
    Vertex();
    ~Vertex();

    enum Bits : std::uint8_t {
        HAS_COLOR = (1 << 1),
        HAS_NORMAL = (1 << 2),
        HAS_TEX_COORD = (1 << 3)
    };

    glm::vec3 point(void) const;
    void point(const glm::vec3 & v);

    /**
     * The colour. Before a grid is shaded it is the geometry's colour, SL's Cs, and the surface
     * shader replaces it with Ci. It is set on every vertex of a grid rather than per
     * micropolygon, since a micropolygon shares its corners with its neighbours.
     */
    glm::vec3 color(void) const;
    void color(const glm::vec3 & colour);
    /**
     * Whether a colour was written. A vertex with no "Cs" of its own takes the colour that
     * was current when its primitive was added, and one with its own keeps it.
     */
    bool hasColor(void) const;

    /**
     * The shading normal, SL's N. A scene that gives a varying "N" per vertex has it here,
     * and the surface comes out smooth. A vertex without one takes the geometric normal, and
     * the surface is faceted.
     */
    glm::vec3 normal(void) const;
    void normal(const glm::vec3 & n);
    /**
     * Whether a shading normal was written. A vertex the scene gave none takes the
     * primitive's geometric normal in RenderContext::addPolygon(), as it takes the colour.
     */
    bool hasNormal(void) const;

    /**
     * The geometric normal, SL's Ng: the normal of the plane the primitive lies in, one value
     * across the primitive rather than a value a vertex brings. It is carried apart from N
     * because a shader reads Ng on its own, and a traced ray starts off the plane it names.
     */
    glm::vec3 geometricNormal(void) const;
    void geometricNormal(const glm::vec3 & n);

    /**
     * The texture coordinates, SL's s and t, from a scene's varying "st". A grid whose
     * corners were given none takes its own parameters instead.
     */
    glm::vec2 st(void) const;
    void st(const glm::vec2 & value);
    bool hasTexCoord(void) const;

 private:
    // a grid is filled in by writing over default constructed vertices, so every member
    // starts at zero and a vertex not yet written reads as zero
    glm::vec3 point_ = glm::vec3(0.0f);
    glm::vec3 color_ = glm::vec3(0.0f);
    glm::vec3 normal_ = glm::vec3(0.0f);
    glm::vec3 geometric_ = glm::vec3(0.0f);
    glm::vec2 st_ = glm::vec2(0.0f);
    unsigned int bits_ = 0;
};
};  // namespace v3d::moya
