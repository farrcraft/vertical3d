/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/Manager.h>
#include <api/asset/media/Loaders.h>
#include <api/log/Logger.h>
#include <api/render/realtime/DecodedModel.h>
#include <demos/diorama/src/Layout.h>

#include <algorithm>
#include <cmath>
#include <set>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

namespace {

/**
 * Every model the village places is a glTF file in the data directory that the renderer can
 * read: it decodes into a model with at least one part, and every part has a material. The
 * decode makes no device call, so this reads the files exactly as MeshRegistry::load() would,
 * without uploading them.
 **/
BOOST_AUTO_TEST_CASE(every_placed_model_decodes) {
    const boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::asset::Manager assets("data/", logger);
    v3d::asset::media::registerLoaders(assets, logger);

    std::set<std::string> models;
    for (const diorama::Placement& placement : diorama::layout()) {
        models.insert(placement.model);
    }
    BOOST_REQUIRE_EQUAL(models.size(), 4U);
    for (const std::string& model : models) {
        BOOST_TEST_CONTEXT(model) {
            const v3d::render::realtime::DecodedModel decoded = v3d::render::realtime::decodeModel(assets, model);
            BOOST_REQUIRE(decoded.model);
            BOOST_TEST(!decoded.model->parts().empty());
            BOOST_TEST(decoded.albedos.size() == decoded.model->materials().size());
        }
    }
}

/**
 * The ground is the one placement that casts no shadow, so the shadow map is fitted around the
 * village and not the whole slab, and the well stands where its voice is placed.
 **/
BOOST_AUTO_TEST_CASE(the_ground_casts_nothing_and_the_well_is_where_it_sounds) {
    const std::vector<diorama::Placement> placed = diorama::layout();
    const auto castingNothing = std::ranges::count_if(placed, [](const diorama::Placement& placement) { return !placement.castsShadow; });
    BOOST_TEST(castingNothing == 1);
    BOOST_TEST((std::ranges::find_if(placed, [](const diorama::Placement& placement) {
        return !placement.castsShadow && placement.model == "models/ground.glb";
    }) != placed.end()));
    BOOST_TEST((std::ranges::find_if(placed, [](const diorama::Placement& placement) {
        return placement.model == "models/well.glb" && placement.position == diorama::WELL;
    }) != placed.end()));
}

/**
 * The sun stays above the horizon all day, and comes back to where it started after a day.
 **/
BOOST_AUTO_TEST_CASE(the_sun_never_sets) {
    for (int hour = 0; hour <= 24; hour++) {
        const float time = diorama::DAY * static_cast<float>(hour) / 24.0F;
        BOOST_TEST_CONTEXT("hour " << hour) {
            const glm::vec3 towards = diorama::sunlight(time);
            BOOST_TEST(towards.y > 0.5F);
            BOOST_TEST(std::abs(glm::length(towards) - 1.0F) < 0.0001F);
        }
    }
    BOOST_TEST(glm::length(diorama::sunlight(0.0F) - diorama::sunlight(diorama::DAY)) < 0.0001F);
    BOOST_TEST(glm::length(diorama::sunlight(0.0F) - diorama::sunlight(diorama::DAY / 2.0F)) > 1.0F);
}

};  // namespace
