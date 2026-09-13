/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Reference.h"

#include <api/image/Compare.h>
#include <api/image/Image.h>
#include <api/image/reader/Png.h>

#include <string>

#include <boost/filesystem/operations.hpp>
#include <boost/test/unit_test.hpp>

namespace v3d::test {

namespace {

/**
 * Where a picture is committed, and where one that was drawn is left. Both are resolved
 * against the working directory, which v3d_add_test points at the executable.
 **/
const char* const committedDirectory = "data";
const char* const renderedDirectory = "data_out";

};  // namespace

/**
 **/
void checkReference(const boost::shared_ptr<v3d::log::Logger>& logger,
    v3d::render::realtime::vulkan::frame::Capture* capture, const std::string& name) {
    const std::string rendered = std::string(renderedDirectory) + "/" + name + ".png";
    const std::string committed = std::string(committedDirectory) + "/" + name + ".png";

    boost::filesystem::create_directory(renderedDirectory);
    BOOST_REQUIRE_MESSAGE(capture->write(rendered), "the capture wrote no " + rendered);

    v3d::image::reader::Png png(logger);
    boost::shared_ptr<v3d::image::Image> drawn = png.read(rendered);
    BOOST_REQUIRE_MESSAGE(drawn != nullptr, "cannot read back " + rendered);

    boost::shared_ptr<v3d::image::Image> reference = png.read(committed);
    // a reference that is not there reads exactly like one that matched, so this is a
    // failure rather than a picture blessed on the spot
    BOOST_REQUIRE_MESSAGE(reference != nullptr,
        "cannot read the reference " + committed + " - what was drawn is in " + rendered);

    const v3d::image::Difference difference = v3d::image::compare(*drawn, *reference, 0);
    BOOST_CHECK_MESSAGE(difference.match,
        difference.description() + " - what was drawn is in " + rendered);
}

};  // namespace v3d::test
