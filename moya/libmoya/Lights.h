/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/Area.h>
#include <api/render/offline/sl/Instance.h>
#include <api/render/offline/sl/Placed.h>
#include <api/render/offline/trace/Primitive.h>

#include <cstddef>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/mat4x4.hpp>

namespace v3d::moya {

/**
 * The lights a scene created. A light lasts for the frame. Which of them are on is an
 * attribute, so it is asked for with the handles that are on.
 **/
class Lights final {
 public:
    /** A light, kept under the handle a later Illuminate names it by. **/
    class Light final {
     public:
        std::string handle;
        v3d::render::offline::sl::InstancePtr shader;
        /** Where the scene instanced it, in camera space. **/
        glm::mat4x4 placement = glm::mat4x4(1.0F);
        /** The geometry an area light shines from, or null for any other light. **/
        boost::shared_ptr<v3d::render::offline::Area> area;
    };

    void add(const Light & light);
    std::size_t size() const;
    /**
     * Make the light added last an area light, which shines from the geometry declared after
     * it. The area takes a number of samples from each shading point.
     **/
    const boost::shared_ptr<v3d::render::offline::Area> & area(unsigned int samples);

    /** The lights the handles switch on, in the order the scene created them, in camera space. **/
    std::vector<v3d::render::offline::sl::Placed> shining(const std::vector<std::string> & lit) const;
    /**
     * The same lights in world space, as one set the traced scene's primitives share. The set
     * is built again only when the lights that are on, or the number of lights, change.
     *
     * @param on the lights that are on, in camera space, as shining() gives them
     * @param toWorld camera to world
     **/
    const v3d::render::offline::trace::Lights & traced(const std::vector<v3d::render::offline::sl::Placed> & on,
        const std::vector<std::string> & lit, const glm::mat4x4 & toWorld);

 private:
    std::vector<Light> lights_;
    v3d::render::offline::trace::Lights traced_;
    std::vector<std::string> tracedFor_;
    std::size_t tracedCount_ = 0;
};

};  // namespace v3d::moya
