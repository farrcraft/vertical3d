/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include "Reference.h"

#include <api/asset/Writer.h>
#include <api/image/Compare.h>
#include <api/image/Factory.h>
#include <api/image/Image.h>
#include <api/log/Logger.h>
#include <api/render/offline/film/FrameBuffer.h>
#include <api/render/offline/film/Sampling.h>
#include <api/render/offline/rib/Reader.h>
#include <moya/libmoya/ri/RIBHandler.h>
#include <moya/libmoya/state/RenderContext.h>

#include <fstream>
#include <optional>
#include <sstream>
#include <string>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>
#include <boost/filesystem/operations.hpp>

#include <glm/vec2.hpp>

namespace v3d::moya::reference {

void pixelCentres(v3d::moya::state::RenderContext & rc) {
    rc.sampling().samples = glm::uvec2(1, 1);
    rc.sampling().filter = v3d::render::offline::film::Filter::Box;
    rc.sampling().width = glm::vec2(1.0F, 1.0F);
}

void keep(const boost::shared_ptr<v3d::image::Image> & rendered, const std::string & output) {
    boost::filesystem::create_directory("data_out");
    v3d::image::Factory factory(boost::make_shared<v3d::log::Logger>());
    const std::optional<std::string> encoded = factory.encode(rendered, v3d::image::Factory::kind(output));
    if (encoded) {
        v3d::asset::writeFile(output, *encoded);
    }
}

void check(const boost::shared_ptr<v3d::image::Image> & rendered, const char* reference,
    const char* output) {
    const boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::image::Factory factory(logger);
    const boost::shared_ptr<v3d::image::Image> committed = factory.read(reference);
    if (committed == nullptr) {
        keep(rendered, output);
    }
    BOOST_REQUIRE_MESSAGE(committed != nullptr,
        std::string("cannot read the reference image ") + reference +
        " - what was rendered is in " + output);

    const v3d::image::Difference difference = v3d::image::compare(*rendered, *committed, 1);
    if (!difference.match) {
        keep(rendered, output);
    }
    BOOST_CHECK_MESSAGE(difference.match,
        difference.description() + " - what was rendered instead is in " + output);
}

boost::shared_ptr<v3d::render::offline::film::FrameBuffer> read(const Scene & scene) {
    v3d::moya::ri::RIBHandler handler;
    if (scene.hider != nullptr) {
        handler.context().hider(scene.hider);
    }
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());
    if (!scene.centres) {
        BOOST_REQUIRE(reader.read(scene.file, &handler));
        BOOST_CHECK_EQUAL(reader.error(), "");
        return handler.context().picture();
    }
    // the file read without its own sampling, so each pixel keeps one sample at its centre
    pixelCentres(handler.context());
    std::ifstream file(scene.file, std::ios::binary);
    BOOST_REQUIRE(file.is_open());
    std::string kept;
    for (std::string line; std::getline(file, line);) {
        if (!line.starts_with("PixelSamples") && !line.starts_with("PixelFilter")) {
            kept += line + "\n";
        }
    }
    std::istringstream stream(kept);
    BOOST_REQUIRE(reader.read(stream, &handler));
    BOOST_CHECK_EQUAL(reader.error(), "");
    return handler.context().picture();
}

};  // namespace v3d::moya::reference
