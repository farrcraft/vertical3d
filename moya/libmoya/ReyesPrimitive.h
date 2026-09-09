/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/type/AABBox.h>

#include "MicroPolygonGrid.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <boost/shared_ptr.hpp>

namespace v3d::moya {
class RenderContext;

class ReyesPrimitive {
 public:
        ReyesPrimitive();
        virtual ~ReyesPrimitive();

        virtual bool diceable(void) const;
        virtual v3d::type::AABBox bound(void) const;
        /**
         * Break the primitive into smaller ones and submit each back to the first pass,
         * which is what decides the bucket and the diceability of each piece. The caller
         * discards this primitive afterwards either way, so a primitive that cannot be
         * usefully split submits nothing and is dropped.
         */
        virtual void split(RenderContext & rc);
        /**
         * Turn the primitive into a micropolygon grid, one call per grid.
         *
         * A primitive may need more than one, so the caller loops - each call that
         * produces a grid replaces what the reference holds and answers true, and the
         * call after the last one answers false. A primitive that answered true without
         * end would never leave that loop.
         */
        virtual bool dice(boost::shared_ptr<MicroPolygonGrid> & grid, RenderContext & rc);
        virtual void diceable(bool status);

        /**
         * The graphics state this primitive was submitted under: the object to eye
         * transformation the first pass measured it with, the colour that was current, and
         * the geometric normal of the plane it lies in, all in the space its vertices are.
         *
         * A primitive keeps them because splitting resubmits its pieces through that pass
         * during the second one, when none of it is current any more - a scene that places
         * and colours two objects would otherwise measure a split piece of the first
         * against the state of the last. The pieces need the colour and the normal for a
         * second reason: a split builds its vertices from intersection points, so they
         * carry neither. A piece therefore takes the whole primitive's plane, which is the
         * plane it lies in too.
         */
        bool placed(void) const;
        void place(const glm::mat4x4 & toEye, const glm::vec3 & color, const glm::vec3 & normal);
        const glm::mat4x4 & placement(void) const;
        const glm::vec3 & color(void) const;
        const glm::vec3 & normal(void) const;

 private:
        glm::mat4x4 placement_ = glm::mat4x4(1.0f);
        glm::vec3 color_ = glm::vec3(1.0f);
        glm::vec3 normal_ = glm::vec3(0.0f);
        bool placed_ = false;
        /*
            False until the first pass has measured the primitive against the grid size.
            An unmeasured primitive is therefore split rather than diced, which routes it
            through that measurement instead of assuming it small enough to skip it.
        */
        bool diceable_ = false;
};
};  // namespace v3d::moya
