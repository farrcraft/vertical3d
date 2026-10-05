/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/sl/Placed.h>

#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/vec3.hpp>

namespace v3d::render::offline::trace {

/**
 * The lights a primitive is shaded by, each with the space it was instanced in. Shared, since
 * every primitive made between two changes of the lights has the same set.
 **/
typedef boost::shared_ptr<const std::vector<v3d::render::offline::sl::Placed> > Lights;

/**
 * What every primitive is shaded with: the colour, opacity, surface shader and lights that
 * were current when the scene made it, and the motion that carries it.
 **/
class Primitive {
 public:
    /** The colour that was current, which is SL's Cs. **/
    const glm::vec3 & colour() const;

    /**
     * The surface shader a scene named, and the space it named it in.
     *
     * Empty for a primitive built in code without one, which is then its own flat colour:
     * a scene that said nothing about shading is drawn the way it was before there was a
     * language to say it in.
     **/
    const v3d::render::offline::sl::Placed & surface() const;
    void surface(const v3d::render::offline::sl::Placed & shader);

    /** The opacity that was current, which is SL's Os. **/
    const glm::vec3 & opacity() const;
    void opacity(const glm::vec3 & value);

    /**
     * The lights that were on when the scene made it, per ADR-0077. Null for a primitive
     * given none, which is shaded by the scene's own list.
     **/
    const Lights & lights() const;
    void lights(const Lights & lit);

    /**
     * Which of the scene's motions carries the primitive, or negative for none. It is
     * stored where the motion's open end put it.
     **/
    int motion() const;

 protected:
    explicit Primitive(const glm::vec3 & colour);

 private:
    friend class Scene;
    int motion_ = -1;
    v3d::render::offline::sl::Placed surface_;
    Lights lights_;
    glm::vec3 opacity_ = glm::vec3(1.0f);
    glm::vec3 colour_;
};

};  // namespace v3d::render::offline::trace
