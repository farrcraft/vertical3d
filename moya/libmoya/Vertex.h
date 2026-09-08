/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <glm/glm.hpp>

namespace v3d::moya {
class Vertex {
 public:
    Vertex();
    ~Vertex();

    enum Bits {
        HAS_COLOR = (1 << 1),
        HAS_NORMAL = (1 << 2),
        HAS_TEX_COORD = (1 << 3)
    };

    glm::vec3 point(void) const;
    void point(const glm::vec3 & v);

    /**
     * The shaded colour. Set on every vertex of a grid rather than per micropolygon,
     * since a micropolygon shares its corners with its neighbours.
     */
    glm::vec3 color(void) const;
    void color(const glm::vec3 & c);
    /**
     * Whether a colour was written. A primitive carrying no "Cs" takes the colour that
     * was current when it was added, and one that carries its own keeps it.
     */
    bool hasColor(void) const;

    /**
     * The shading normal, SL's N. A scene that gives a varying "N" per vertex has it here
     * and a surface comes out smooth; one that does not takes the geometric normal, and
     * the surface is faceted.
     */
    glm::vec3 normal(void) const;
    void normal(const glm::vec3 & n);
    /**
     * Whether the scene gave one. A vertex left without it takes the primitive's
     * geometric normal in RenderContext::addPolygon(), the way it takes the colour.
     */
    bool hasNormal(void) const;

    /**
     * The geometric normal, SL's Ng: the plane the primitive lies in, one value across it
     * rather than a value a vertex brings. Both are carried because faceforward() and
     * calculatenormal() are defined in terms of the pair.
     */
    glm::vec3 geometricNormal(void) const;
    void geometricNormal(const glm::vec3 & n);

 private:
    // a grid is filled in by writing over default constructed vertices, so a vertex that
    // has not been written to yet has to read as one rather than as whatever was there
    glm::vec3 point_ = glm::vec3(0.0f);
    glm::vec3 color_ = glm::vec3(0.0f);
    glm::vec3 normal_ = glm::vec3(0.0f);
    glm::vec3 geometric_ = glm::vec3(0.0f);
    unsigned int bits_ = 0;
};
};  // namespace v3d::moya
