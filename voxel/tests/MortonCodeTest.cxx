/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <boost/test/unit_test.hpp>

#include "../src/engine/MortonCode.h"

BOOST_AUTO_TEST_CASE(morton_code_round_trip_test) {
    const MortonCode codec;

    // every block position within a chunk, which is what the code is used to key
    for (int x = 0; x < 16; x++) {
        for (int y = 0; y < 16; y++) {
            for (int z = 0; z < 16; z++) {
                const glm::ivec3 position(x, y, z);
                const glm::ivec3 decoded = codec.decode3(codec.encode(position));
                BOOST_REQUIRE_EQUAL(decoded.x, position.x);
                BOOST_REQUIRE_EQUAL(decoded.y, position.y);
                BOOST_REQUIRE_EQUAL(decoded.z, position.z);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(morton_code_is_one_to_one_test) {
    const MortonCode codec;

    // chunks are keyed by their code too, and two chunks sharing one would silently
    // replace each other in the world map
    BOOST_CHECK_NE(codec.encode(glm::ivec3(1, 0, 0)), codec.encode(glm::ivec3(0, 1, 0)));
    BOOST_CHECK_NE(codec.encode(glm::ivec3(0, 1, 0)), codec.encode(glm::ivec3(0, 0, 1)));
    BOOST_CHECK_EQUAL(codec.encode(glm::ivec3(0, 0, 0)), 0u);
}
