/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/MovingTransform.h>
#include <api/type/geometry/AABBox.h>
#include <moya/libmoya/Shading.h>

#include "MicroPolygonGrid.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <boost/shared_ptr.hpp>

namespace v3d::moya {
class RenderContext;
};  // namespace v3d::moya

namespace v3d::moya::reyes {

class Hider;

/**
 * A primitive as the reyes hider handles it. The hider's add() bounds it and buckets it, and
 * its render() dices it into grids, or splits it when it is too large to dice.
 */
class Primitive {
 public:
        Primitive();
        virtual ~Primitive();

        virtual bool diceable(void) const;
        virtual v3d::type::geometry::AABBox bound(void) const;
        /**
         * Break the primitive into smaller ones and submit each to the first pass, which
         * decides each piece's bucket and whether it can be diced. The caller discards this
         * primitive afterwards either way. A primitive that cannot be usefully split submits
         * nothing, and is dropped.
         */
        virtual void split(Hider & hider, RenderContext & rc);
        /**
         * Turn the primitive into a micropolygon grid, one call per grid.
         *
         * A primitive may need more than one, so the caller loops. Each call that produces
         * a grid replaces what the reference holds and returns true. The call after the last
         * grid returns false, and must, or the caller's loop never ends.
         */
        virtual bool dice(boost::shared_ptr<MicroPolygonGrid> & grid, RenderContext & rc);
        virtual void diceable(bool status);

        /**
         * The graphics state this primitive was submitted under: the object to eye
         * transformation the first pass measured it with, the current colour, its plane's
         * geometric normal in object space, and its shading.
         *
         * A split in the second pass resubmits the pieces to the first pass. By then none of
         * that graphics state is current. Each piece is placed with its parent's state, so a
         * piece of one object is not measured against the state of a later one. A split
         * carries each vertex's own colour and shading normal onto the pieces, and
         * interpolates them where an edge is cut. The colour and the normal here fill only a
         * vertex that has none. The normal is also every piece's geometric normal, because a
         * piece lies in its parent's plane.
         */
        bool placed(void) const;
        void place(const glm::mat4x4 & toEye, const glm::vec3 & color, const glm::vec3 & normal,
            const Shading & shading);
        const glm::mat4x4 & placement(void) const;
        const glm::vec3 & color(void) const;
        const glm::vec3 & normal(void) const;
        /**
         * The surface shader and the lights that were on. Carried across a split with the
         * rest of the state, and read when the second pass shades the primitive's grids.
         */
        const Shading & shading(void) const;

        /**
         * The object to eye transformation across the shutter. placement() is its reference
         * end: the open end, or the close end when the open one has no inverse. The vertices
         * are where the reference end puts them, and a sample moves them to its own time.
         * Carried across a split with the rest of the state.
         */
        void motion(const v3d::render::offline::MovingTransform & toEye);
        const v3d::render::offline::MovingTransform & motion(void) const;

 private:
        v3d::render::offline::MovingTransform motion_;
        glm::mat4x4 placement_ = glm::mat4x4(1.0F);
        glm::vec3 color_ = glm::vec3(1.0F);
        glm::vec3 normal_ = glm::vec3(0.0F);
        Shading shading_;
        bool placed_ = false;
        /**
         * False until the first pass has measured the primitive against the grid size. An
         * unmeasured primitive is split rather than diced, and its pieces are measured.
         */
        bool diceable_ = false;
};
};  // namespace v3d::moya::reyes
