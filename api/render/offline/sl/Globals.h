/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/sl/Placed.h>
#include <api/render/offline/sl/runtime/Machine.h>
#include <api/render/offline/sl/runtime/Program.h>
#include <api/render/offline/sl/runtime/Value.h>

#include <vector>

#include <glm/vec3.hpp>

namespace v3d::render::offline::sl {

/**
 * What a hider knows about one shading point, which a surface shader reads as its globals.
 * Every hider fills in the same fields, so a shader sees the same thing under either.
 **/
class Point final {
 public:
    glm::vec3 position = glm::vec3(0.0f);   /**< P **/
    glm::vec3 normal = glm::vec3(0.0f);     /**< N **/
    glm::vec3 geometric = glm::vec3(0.0f);  /**< Ng **/
    glm::vec3 incident = glm::vec3(0.0f);   /**< I **/
    glm::vec3 colour = glm::vec3(1.0f);     /**< Cs **/
    glm::vec3 opacity = glm::vec3(1.0f);    /**< Os, and Oi until the shader sets it **/
    float s = 0.0f;
    float t = 0.0f;
    float u = 0.0f;
    float v = 0.0f;
    float du = 0.0f;                        /**< zero for a hider with no neighbouring point to difference **/
    float dv = 0.0f;
};

/**
 * Which register each global a renderer writes or reads is, in one program, looked up once
 * when its machine is prepared rather than by name at every point. A global the program does
 * not have is -1, and writing or reading it does nothing.
 **/
class Globals final {
 public:
    /**
     * Every global absent, for a machine not yet prepared.
     **/
    Globals() = default;

    /**
     **/
    explicit Globals(const runtime::Program & program);

    /**
     * Write a surface point into one lane of the batch.
     **/
    void surface(runtime::Machine* machine, unsigned int lane, const Point & point) const;

    /**
     * Write E, which is one value for the whole batch.
     **/
    void eye(runtime::Machine* machine, const glm::vec3 & position) const;

    /**
     * @return what the shader left in Ci at a lane, or otherwise when it has no Ci
     **/
    glm::vec3 colour(const runtime::Machine & machine, unsigned int lane, const glm::vec3 & otherwise) const;

    /**
     * @return what the shader left in Oi at a lane, or otherwise when it has no Oi
     **/
    glm::vec3 opacity(const runtime::Machine & machine, unsigned int lane, const glm::vec3 & otherwise) const;

    /**
     * Write a pixel into one lane, for an imager: its colour as Ci, its coverage as Oi and
     * alpha, and its centre in raster space as P, which is what an imager that varies across
     * the frame reads.
     **/
    void pixel(runtime::Machine* machine, unsigned int lane, const glm::vec3 & colour, float coverage,
        const glm::vec3 & position) const;

    /**
     * @return what an imager left in alpha at a lane, or otherwise when it has no alpha
     **/
    float alpha(const runtime::Machine & machine, unsigned int lane, float otherwise) const;

    /**
     * Run a light over a batch of surface points and read back what it shone on each.
     *
     * The light's parameters are written through its placement, and its P is the origin of
     * that space, which is what lands a light placed by a transform where the scene put it.
     * The renderer answers "shader" space with the light's placement while this runs.
     *
     * @param machine prepared for the light's program, whose globals these are
     * @param surface the points being lit, in the renderer's current space
     * @return whether the light ran
     **/
    bool shine(const Placed & light, runtime::Machine* machine, unsigned int batch, const runtime::Value & surface,
        runtime::Value* direction, runtime::Value* colour, std::vector<char>* reached, bool* ambient) const;

 private:
    int position_ = -1;
    int normal_ = -1;
    int geometric_ = -1;
    int incident_ = -1;
    int eye_ = -1;
    int surfaceColour_ = -1;
    int surfaceOpacity_ = -1;
    int colour_ = -1;
    int opacity_ = -1;
    int alpha_ = -1;
    int s_ = -1;
    int t_ = -1;
    int u_ = -1;
    int v_ = -1;
    int du_ = -1;
    int dv_ = -1;
    int lit_ = -1;          /**< Ps, the surface point a light shader is lighting **/
    int towards_ = -1;      /**< L **/
    int lightColour_ = -1;  /**< Cl **/
};

};  // namespace v3d::render::offline::sl
