/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <array>
#include <cstddef>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/geometric.hpp>

#include "../Overlay.h"
#include "../TileGrid.h"

using v3d::grid::LineSink;
using v3d::grid::TileCoord;
using v3d::grid::TileGrid;
using v3d::grid::outlineGrid;
using v3d::grid::outlineTile;
using v3d::grid::tileCorners;

namespace {

constexpr TileCoord at(int x, int y) {
    return TileCoord{ x, y };
}

struct Segment {
    glm::vec3 from;
    glm::vec3 to;
    glm::vec4 colour;
};

/**
 * A sink that keeps what it was handed, which is what makes an overlay assertable with no
 * renderer anywhere near it.
 **/
class Recorder {
 public:
    LineSink sink() {
        return [this](const glm::vec3& from, const glm::vec3& to, const glm::vec4& colour) {
            segments_.push_back(Segment{ from, to, colour });
        };
    }

    const std::vector<Segment>& segments() const {
        return segments_;
    }

    std::size_t coloured(const glm::vec4& colour) const {
        std::size_t count = 0;
        for (const Segment& segment : segments_) {
            if (segment.colour == colour) {
                ++count;
            }
        }
        return count;
    }

 private:
    std::vector<Segment> segments_;
};

constexpr glm::vec4 INTERIOR(0.3f, 0.3f, 0.3f, 1.0f);
constexpr glm::vec4 BORDER(0.8f, 0.8f, 0.9f, 1.0f);

};  // namespace

BOOST_AUTO_TEST_CASE(overlay_tile_corners_bound_the_tile_test) {
    const TileGrid grid(4, 4);
    const std::array<glm::vec3, 4> corners = tileCorners(grid, at(2, 1));
    const glm::vec3 centre = grid.tileToWorld(at(2, 1));
    const float half = 0.5f * grid.tileSize();

    for (const glm::vec3& corner : corners) {
        BOOST_CHECK_CLOSE(corner.x - centre.x > 0.0f ? corner.x - centre.x : centre.x - corner.x, half, 0.01f);
        BOOST_CHECK_CLOSE(corner.z - centre.z > 0.0f ? corner.z - centre.z : centre.z - corner.z, half, 0.01f);
    }

    // consecutive corners share an edge, so each is one tile side from the next and the
    // fourth closes back onto the first
    for (std::size_t corner = 0; corner < corners.size(); ++corner) {
        const glm::vec3 next = corners[(corner + 1) % corners.size()];
        BOOST_CHECK_CLOSE(glm::length(next - corners[corner]), grid.tileSize(), 0.01f);
    }
}

BOOST_AUTO_TEST_CASE(overlay_is_lifted_clear_of_the_ground_test) {
    const TileGrid grid(4, 4);

    // coplanar geometry z-fights the ground wherever depth testing is on, and every layer
    // of the overlay shares one lift so none of them sinks into another
    for (const glm::vec3& corner : tileCorners(grid, at(0, 0))) {
        BOOST_CHECK_CLOSE(corner.y, v3d::grid::OVERLAY_LIFT, 0.01f);
    }

    Recorder recorder;
    outlineGrid(grid, INTERIOR, BORDER, recorder.sink());
    for (const Segment& segment : recorder.segments()) {
        BOOST_CHECK_CLOSE(segment.from.y, v3d::grid::OVERLAY_LIFT, 0.01f);
        BOOST_CHECK_CLOSE(segment.to.y, v3d::grid::OVERLAY_LIFT, 0.01f);
    }
}

BOOST_AUTO_TEST_CASE(overlay_outline_tile_closes_test) {
    const TileGrid grid(4, 4);
    Recorder recorder;

    outlineTile(grid, at(1, 1), BORDER, recorder.sink());

    BOOST_CHECK_EQUAL(recorder.segments().size(), 4u);
    // the four edges join end to end and the last comes back to the first
    for (std::size_t edge = 1; edge < recorder.segments().size(); ++edge) {
        BOOST_CHECK_SMALL(glm::length(recorder.segments()[edge].from - recorder.segments()[edge - 1].to), 0.0001f);
    }
    BOOST_CHECK_SMALL(glm::length(recorder.segments().back().to - recorder.segments().front().from), 0.0001f);
}

BOOST_AUTO_TEST_CASE(overlay_outline_tile_off_the_grid_draws_nothing_test) {
    const TileGrid grid(4, 4);
    Recorder recorder;

    outlineTile(grid, at(-1, 0), BORDER, recorder.sink());
    outlineTile(grid, at(4, 4), BORDER, recorder.sink());

    BOOST_CHECK_EQUAL(recorder.segments().empty(), true);
}

BOOST_AUTO_TEST_CASE(overlay_grid_draws_the_boundaries_test) {
    const TileGrid grid(4, 3);
    Recorder recorder;

    outlineGrid(grid, INTERIOR, BORDER, recorder.sink());

    // the lines are the boundaries rather than the tiles: five down the width and four
    // across the height, not twelve tiles' worth
    BOOST_CHECK_EQUAL(recorder.segments().size(), 9u);
    BOOST_CHECK_EQUAL(recorder.coloured(BORDER), 4u);
    BOOST_CHECK_EQUAL(recorder.coloured(INTERIOR), 5u);
}

BOOST_AUTO_TEST_CASE(overlay_grid_spans_the_board_test) {
    const TileGrid grid(4, 3);
    Recorder recorder;

    outlineGrid(grid, INTERIOR, BORDER, recorder.sink());

    const glm::vec3 min = grid.worldMin();
    const float spanX = static_cast<float>(grid.width()) * grid.tileSize();
    const float spanZ = static_cast<float>(grid.height()) * grid.tileSize();

    for (const Segment& segment : recorder.segments()) {
        // every line runs the full extent of the board on one axis and sits on a tile
        // boundary on the other, so the outline is the grid's own arithmetic rather than a
        // second opinion about where the tiles are
        const bool alongZ = segment.from.x == segment.to.x;
        if (alongZ) {
            BOOST_CHECK_CLOSE(segment.from.z, min.z, 0.01f);
            BOOST_CHECK_CLOSE(segment.to.z, min.z + spanZ, 0.01f);
        } else {
            BOOST_CHECK_CLOSE(segment.from.x, min.x, 0.01f);
            BOOST_CHECK_CLOSE(segment.to.x, min.x + spanX, 0.01f);
        }
    }
}

BOOST_AUTO_TEST_CASE(overlay_a_tile_outline_lies_on_the_grid_outline_test) {
    const TileGrid grid(4, 4);

    // the corner tile's outer corner is the board's own corner, so a highlight cannot sit
    // half a tile off the lines it is drawn over
    const std::array<glm::vec3, 4> corners = tileCorners(grid, at(0, 0));
    const glm::vec3 min = grid.worldMin();

    BOOST_CHECK_CLOSE(corners[0].x, min.x, 0.01f);
    BOOST_CHECK_CLOSE(corners[0].z, min.z, 0.01f);
}

BOOST_AUTO_TEST_CASE(overlay_a_default_sink_draws_nothing_test) {
    const TileGrid grid(4, 4);

    // an empty std::function would be called through if it were not guarded
    outlineTile(grid, at(0, 0), BORDER, LineSink());
    outlineGrid(grid, INTERIOR, BORDER, LineSink());
}
