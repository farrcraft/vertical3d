/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Bucket.h"

#include "Hider.h"

#include <api/render/offline/MovingTransform.h>
#include <moya/libmoya/state/RenderContext.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <vector>

#include <boost/make_shared.hpp>

#include "Footprint.h"
#include "GridShader.h"
#include "MicroPolygon.h"
#include "MicroPolygonGrid.h"
#include "Motions.h"
#include "Placement.h"
#include "Primitive.h"
#include "Samples.h"

namespace v3d::moya::reyes {

namespace {

typedef Placement::Corners Corners;

/**
 * The pixels a bound touches in the frame the samples cover.
 */
std::array<int, 4> pixels(const glm::vec3 & min, const glm::vec3 & max, const Samples & samples) {
    // a side is at most largestResolution, which an int holds
    return footprint(min, max, static_cast<int>(samples.width()), static_cast<int>(samples.height()));
}

/**
 * Hide one placed micropolygon into the samples of the pixels its bound touches. A sample it
 * covers is offered its colour, its opacity and its depth, as a layer of the grid it came
 * from, and keeps it unless an opaque surface is already as near.
 */
void hide(const Placement & placement, const Samples::Layer & shaded, Samples * samples, Motions * motions) {
    const std::array<int, 4> region = pixels(placement.min(), placement.max(), *samples);
    for (int row = region[1]; row <= region[3]; row++) {
        for (int column = region[0]; column <= region[2]; column++) {
            for (unsigned int k = 0; k < samples->perPixel(); k++) {
                Samples::Sample & sample = samples->at(static_cast<unsigned int>(column),
                    static_cast<unsigned int>(row), k);
                if (!placement.reaches(sample)) {
                    continue;
                }
                const Corners corners = motions == nullptr ? placement.at(sample) :
                    placement.at(sample, motions->at({.column = column, .row = row, .index = k, .time = sample.time}));
                float depth = 0.0F;
                if (!covers(corners, sample.raster, &depth) || depth >= sample.layers.opaque()) {
                    continue;
                }
                Samples::Layer layer = shaded;
                layer.depth = depth;
                sample.layers.add(layer);
            }
        }
    }
}

/**
 * Hide every micropolygon of a grid into the frame's samples. The film filters the samples
 * into pixels once every bucket is done.
 */
void hide(MicroPolygonGrid & grid, const Primitive & primitive, Hider & hider, state::RenderContext & rc) {
    const glm::mat4x4 & toRaster = rc.camera().raster;
    // an orthographic camera has no lens to blur through
    const float radius = rc.perspective() ? rc.sampling().lensRadius() : 0.0F;
    Placement placement(toRaster, {.radius = radius, .focus = rc.sampling().focalDistance}, primitive.motion(),
        rc.sampling().shutter);
    Samples & samples = hider.samples();
    const unsigned int surface = samples.surface();

    const auto corners = [&grid](unsigned int column, unsigned int row) {
        MicroPolygon poly = grid.microPolygon(column, row);
        Corners eye;
        for (unsigned int k = 0; k < 4; k++) {
            eye[k] = poly[k].point();
        }
        return eye;
    };

    // a moving grid's micropolygons share one Motions, which covers the pixels all of them
    // sweep. A loop over the grid measures that region before any micropolygon is hidden
    boost::shared_ptr<Motions> motions;
    if (primitive.motion().moving()) {
        glm::vec3 min(std::numeric_limits<float>::max());
        glm::vec3 max(-std::numeric_limits<float>::max());
        for (unsigned int i = 0; i + 1 < grid.size(); i++) {
            for (unsigned int j = 0; j + 1 < grid.size(); j++) {
                placement.place(corners(i, j));
                min = glm::min(min, placement.min());
                max = glm::max(max, placement.max());
            }
        }
        const std::array<int, 4> region = pixels(min, max, samples);
        const std::size_t entries = static_cast<std::size_t>(std::max(0, region[2] - region[0] + 1)) *
            static_cast<std::size_t>(std::max(0, region[3] - region[1] + 1)) * samples.perPixel();
        // a grid that sweeps much of the frame would need gigabytes of motions. Past the
        // context's motionCache() entries, each micropolygon works out its own motions instead,
        // which is slower and needs no memory
        if (entries <= rc.motionCache()) {
            motions = boost::make_shared<Motions>(placement, samples.perPixel(), region);
        }
    }

    for (unsigned int i = 0; i + 1 < grid.size(); i++) {
        for (unsigned int j = 0; j + 1 < grid.size(); j++) {
            placement.place(corners(i, j));
            // flat shaded with the colour and opacity on its first corner, which the surface
            // shader left there
            const Vertex first = grid.microPolygon(i, j)[0];
            hide(placement, {.colour = first.color(), .opacity = first.opacity(), .surface = surface}, &samples,
                motions.get());
        }
    }
}

};  // namespace

Bucket::Bucket() {
}

Bucket::~Bucket() {
}

void Bucket::addPrimitive(const boost::shared_ptr<Primitive>& primitive) {
    primitives_.push_back(primitive);
}

size_t Bucket::primitiveCount(void) const {
    return primitives_.size();
}

bool Bucket::render(Hider & hider, state::RenderContext & rc) {
    bool split = false;
    // splitting resubmits pieces through the hider, which may append to this same bucket, so
    // the loop reads the size on every iteration
    for (size_t i = 0; i < primitives_.size(); i++) {
        // a copy, because split() may append to primitives_ and so move the entry it came from
        const boost::shared_ptr<Primitive> prim = primitives_[i];
        if (prim->diceable()) {
            boost::shared_ptr<MicroPolygonGrid> grid;
            while (prim->dice(grid, rc)) {
                // dicing interpolates the vertices' colours onto the grid as Cs. A displacement
                // shader moves the grid first, so the surface shader sees the moved P and N.
                // The surface shader runs over every vertex at once and leaves Ci there
                rc.shader().displace(prim->shading(), grid.get());
                rc.shader().shade(prim->shading(), grid.get());
                hide(*grid, *prim, hider, rc);
            }
        } else {
            // the hider buckets each piece and decides whether it is diceable, so the original
            // is erased whether or not split() submitted anything
            prim->split(hider, rc);
            primitives_.erase(primitives_.begin() + static_cast<std::ptrdiff_t>(i));
            i--;
            split = true;
        }
    }
    return split;
}

};  // namespace v3d::moya::reyes
