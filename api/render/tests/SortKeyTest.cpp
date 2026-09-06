/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <algorithm>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "../realtime/DrawItem.h"

BOOST_AUTO_TEST_SUITE(sortkey_test)

/**
 * A fresh key is the lowest one there is, so an item nobody has classified sorts first
 * rather than landing somewhere arbitrary.
 **/
BOOST_AUTO_TEST_CASE(default_key_is_zero) {
    v3d::render::realtime::SortKey key;

    BOOST_CHECK_EQUAL(key.packed(), 0u);
}

/**
 * Layer dominates everything else. This is the whole reason the field exists: a sprite in a
 * later layer stays on top of an earlier one whatever pipeline or material either uses.
 **/
BOOST_AUTO_TEST_CASE(layer_outranks_every_other_field) {
    v3d::render::realtime::SortKey lower;
    lower.layer = 1;

    v3d::render::realtime::SortKey higher;
    higher.layer = 2;
    // everything the higher layer could be beaten on, it is
    lower.pipeline = 0xFFFF;
    lower.material = 0xFFFF;
    lower.depth = 0xFFFF;

    BOOST_CHECK(lower < higher);
    BOOST_CHECK(!(higher < lower));
}

/**
 * Within a layer the order is pipeline, then material, then depth.
 **/
BOOST_AUTO_TEST_CASE(fields_order_coarsest_first) {
    v3d::render::realtime::SortKey pipeline;
    pipeline.pipeline = 1;

    v3d::render::realtime::SortKey material;
    material.material = 1;
    material.depth = 0xFFFF;

    v3d::render::realtime::SortKey depth;
    depth.depth = 1;

    BOOST_CHECK(depth < material);
    BOOST_CHECK(material < pipeline);
}

/**
 * The packing is reversible in the sense that matters - two different classifications never
 * collapse onto the same key.
 **/
BOOST_AUTO_TEST_CASE(fields_do_not_collide) {
    v3d::render::realtime::SortKey first;
    first.layer = 3;
    first.pipeline = 7;
    first.material = 11;
    first.depth = 13;

    v3d::render::realtime::SortKey second;
    second.layer = 3;
    second.pipeline = 7;
    second.material = 11;
    second.depth = 14;

    BOOST_CHECK(first.packed() != second.packed());
    BOOST_CHECK(first < second);
}

/**
 * Sorting a queue by key groups items the way the recorder will want to merge them.
 **/
BOOST_AUTO_TEST_CASE(sorting_groups_by_pipeline_within_a_layer) {
    std::vector<v3d::render::realtime::SortKey> keys;

    v3d::render::realtime::SortKey second;
    second.layer = 0;
    second.pipeline = 2;
    keys.push_back(second);

    v3d::render::realtime::SortKey first;
    first.layer = 0;
    first.pipeline = 1;
    keys.push_back(first);

    v3d::render::realtime::SortKey overlay;
    overlay.layer = 1;
    overlay.pipeline = 1;
    keys.push_back(overlay);

    std::sort(keys.begin(), keys.end());

    BOOST_CHECK_EQUAL(keys[0].pipeline, 1);
    BOOST_CHECK_EQUAL(keys[0].layer, 0);
    BOOST_CHECK_EQUAL(keys[1].pipeline, 2);
    BOOST_CHECK_EQUAL(keys[1].layer, 0);
    BOOST_CHECK_EQUAL(keys[2].layer, 1);
}

BOOST_AUTO_TEST_SUITE_END()
