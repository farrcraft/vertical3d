/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/Texture.h>
#include <api/render/offline/sl/Placed.h>
#include <api/render/offline/sl/runtime/Machine.h>
#include <api/render/offline/sl/runtime/Renderer.h>

#include <string>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Hit.h"
#include "Scene.h"

namespace v3d::render::offline::trace {

class Tracer;

/**
 * Runs a surface shader over a batch of hits, and implements the renderer callbacks the shader
 * makes while it runs.
 *
 * A batch takes no separate path. It runs the same program and the same instructions that moya
 * runs over a grid. **Every hit of a batch shares the uniform state of the run**: its surface
 * shader and the shader's placement, its object space and its lights. The Tracer groups hits
 * that way before it makes one.
 *
 * **A hit is one point, or three when the shader differentiates.** A surface shader or one of
 * its lights that reads a derivative shades each hit with its two neighbours, a step along u
 * and along v, as a stencil of three. Any other shades each hit as a single point, and its
 * derivatives are zero.
 *
 * **Only a hit casts rays.** transmission() traces one shadow ray for each hit, from the hit
 * itself, and its neighbours take its answer. trace() traces one ray for each hit, and the
 * batch's rays are traced and shaded together, one depth down.
 *
 * The Tracer makes one on the stack for each batch it shades. A hit reached by the shader's
 * trace() is shaded by its own HitShader, so this one's state is unchanged when trace()
 * returns.
 *
 * **A hit's current space is world space**, because the scene is held in world space. moya's
 * grids shade in camera space. Each renderer answers current, shader and object space itself.
 * Every other space comes from the scene's table, Scene::fromWorld(), from world space.
 **/
class HitShader final : public v3d::render::offline::sl::runtime::Renderer {
 public:
    /**
     * @param hits what to shade, which the Tracer grouped to share one run. Each is kept by
     *        pointer, and must outlive the HitShader
     * @param times when each hit's sample was taken
     * @param poses where the moving primitives are for each hit, at its time. Kept by
     *        pointer, as the hits are
     **/
    HitShader(Tracer* tracer, const std::vector<const Hit*> & hits, const std::vector<float> & times,
        const std::vector<const Scene::Poses*> & poses);
    /** One hit, at the time the tracer was set to. **/
    HitShader(Tracer* tracer, const Hit & hit);

    HitShader(const HitShader &) = delete;
    HitShader & operator=(const HitShader &) = delete;

    /**
     * Shade every hit, and write each one's Ci and Oi. When the primitives have no shader or
     * the shader fails, each hit is its primitive's colour times its opacity, and its opacity.
     **/
    void shade(std::vector<glm::vec3>* colours, std::vector<glm::vec3>* opacities);
    /** shade() for a batch of one: the hit's Ci, with its Oi written through opacity. **/
    glm::vec3 shade(glm::vec3* opacity);

    /** How many points each hit is shaded as: 1, or STENCIL for a shader that differentiates. **/
    unsigned int lanes() const;

    // the runtime::Renderer callbacks
    bool space(const std::string & name, glm::mat4x4* matrix) override;
    unsigned int lights() override;
    bool light(unsigned int index, const v3d::render::offline::sl::runtime::Value & surface,
        const v3d::render::offline::sl::runtime::Cone & gathering,
        v3d::render::offline::sl::runtime::Value* direction,
        v3d::render::offline::sl::runtime::Value* colour,
        std::vector<char>* reached, bool* ambient) override;
    bool transmission(const v3d::render::offline::sl::runtime::Value & from,
        const v3d::render::offline::sl::runtime::Value & to,
        v3d::render::offline::sl::runtime::Value* fraction) override;
    bool trace(const v3d::render::offline::sl::runtime::Value & origin,
        const v3d::render::offline::sl::runtime::Value & direction,
        v3d::render::offline::sl::runtime::Value* colour) override;
    const v3d::render::offline::Texture* texture(const std::string & name) override;
    /**
     * A light reads the surface being shaded, and a surface's illuminance loop reads the light
     * it is running for. A traced primitive was displaced when it was diced, so a hit has no
     * displacement shader to read.
     **/
    bool message(const std::string & shader, const std::string & name,
        v3d::render::offline::sl::runtime::Value* value) override;
    /**
     * A stencil of each hit and its two neighbours, each group with its own steps, or lone
     * points when the shader does not differentiate.
     **/
    Shape shape() const override;

 private:
    /** The lights on the batch: its primitives', or the scene's. **/
    const std::vector<v3d::render::offline::sl::Placed> & shining() const;

    /**
     * Whether the surface shader or any light shining on the batch reads a derivative.
     **/
    bool differentiates() const;

    /**
     * A hit's neighbours along u and v, and the steps to them. A step is the parameter change
     * that moves the surface one pixel's width, so a derivative is taken over what one pixel
     * sees. A neighbour the primitive cannot place leaves its step at zero, and is the hit
     * itself.
     **/
    glm::vec2 neighbours(const Hit & hit, const Scene::Poses & poses, Hit* alongU, Hit* alongV) const;

    /** The machines whose output parameters a message reads, or null. **/
    const v3d::render::offline::sl::runtime::Machine* surfaceMachine_ = nullptr;
    const v3d::render::offline::sl::runtime::Machine* lightMachine_ = nullptr;

    Tracer* tracer_;
    std::vector<const Hit*> hits_;
    std::vector<float> times_;
    /** Where the moving primitives are for each hit, at its time. **/
    std::vector<const Scene::Poses*> poses_;
    /** 1, or STENCIL once shade() has found that the shader differentiates. **/
    unsigned int lanes_ = 1;
    /**
     * The placement of the shader being run, which maps its own space into world space: the
     * surface's, and a light's while that light runs.
     **/
    glm::mat4x4 placement_;
    /** Each hit's steps to its neighbours, for shape(). **/
    std::vector<glm::vec2> steps_;
};

};  // namespace v3d::render::offline::trace
