/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/log/Logger.h>
#include <api/render/offline/FrameBuffer.h>
#include <api/render/offline/rib/Parameters.h>
#include <api/render/offline/sl/Imager.h>
#include <api/render/offline/sl/ShaderLibrary.h>

#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

namespace {

typedef v3d::render::offline::FrameBuffer FrameBuffer;
typedef v3d::render::offline::rib::Declaration Declaration;
typedef v3d::render::offline::rib::ParameterList ParameterList;
typedef v3d::render::offline::sl::ShaderLibrary ShaderLibrary;
typedef v3d::render::offline::sl::InstancePtr InstancePtr;
typedef v3d::render::offline::sl::ShaderType ShaderType;

/** The colour plane count plus one for coverage, which is what both renderers hold. **/
const unsigned int COVERAGE = 3;

ShaderLibrary & library() {
    static ShaderLibrary shaders(boost::make_shared<v3d::log::Logger>());
    return shaders;
}

/**
 * A tiny frame with one pixel drawn into and the rest of it untouched.
 **/
FrameBuffer drawn() {
    FrameBuffer frame(4, 2, COVERAGE + 1);
    frame.value(0, 1, 0, 0.9f);
    frame.value(1, 1, 0, 0.2f);
    frame.value(2, 1, 0, 0.2f);
    frame.value(COVERAGE, 1, 0, 1.0f);
    return frame;
}

};  // namespace

/**
 * `background` composites the frame over a constant colour where coverage says nothing was
 * drawn, and leaves the pixels that were drawn into alone. That is the whole of what a
 * scene needs to say what a ray that hit nothing is worth.
 **/
BOOST_AUTO_TEST_CASE(slimager_background_test) {
    ParameterList list;
    list.add("background", Declaration(Declaration::Storage::UNIFORM, Declaration::Type::COLOR, 1),
        { 0.15f, 0.25f, 0.45f }, std::vector<std::string>());
    const InstancePtr shader = library().instance("background", ShaderType::IMAGER, list);
    BOOST_REQUIRE(shader);

    FrameBuffer frame = drawn();
    v3d::render::offline::sl::Imager imager(shader, nullptr);
    BOOST_REQUIRE(imager.run(&frame, COVERAGE));

    // the pixel nothing was drawn into takes the colour whole
    BOOST_CHECK_CLOSE(frame.value(0, 0, 0), 0.15f, 0.01f);
    BOOST_CHECK_CLOSE(frame.value(2, 0, 0), 0.45f, 0.01f);
    // and the one that was drawn into is untouched, because its coverage was one
    BOOST_CHECK_CLOSE(frame.value(0, 1, 0), 0.9f, 0.01f);
    BOOST_CHECK_CLOSE(frame.value(1, 1, 0), 0.2f, 0.01f);
    // every row runs, not only the first
    BOOST_CHECK_CLOSE(frame.value(2, 3, 1), 0.45f, 0.01f);

    // and the frame is covered afterwards: a pixel the imager painted is no longer one
    // that nothing was drawn into
    BOOST_CHECK_CLOSE(frame.value(COVERAGE, 0, 0), 1.0f, 0.01f);
}

/**
 * A batch here is a row of pixels, which is neither a grid nor a ray hit - the third
 * consumer of the machine, and the one that says the batch model was not built for grids
 * alone. An imager that varies across the frame is what says each pixel of the row got its
 * own answer rather than the first one's.
 **/
BOOST_AUTO_TEST_CASE(slimager_a_row_is_a_batch_test) {
    ShaderLibrary shaders(boost::make_shared<v3d::log::Logger>());
    {
        std::ofstream file("ramp.sl");
        BOOST_REQUIRE(file.is_open());
        file << "imager ramp() { Ci = color (P . point (1, 0, 0), 0, 0); }\n";
    }
    shaders.searchpath(".");
    const InstancePtr shader = shaders.instance("ramp", ShaderType::IMAGER, ParameterList());
    BOOST_REQUIRE(shader);

    FrameBuffer frame(4, 2, COVERAGE + 1);
    v3d::render::offline::sl::Imager imager(shader, nullptr);
    BOOST_REQUIRE(imager.run(&frame, COVERAGE));

    // P is the pixel's centre in raster space, so column c reads c and a half
    BOOST_CHECK_CLOSE(frame.value(0, 0, 0), 0.5f, 0.01f);
    BOOST_CHECK_CLOSE(frame.value(0, 3, 0), 3.5f, 0.01f);
    BOOST_CHECK_CLOSE(frame.value(0, 2, 1), 2.5f, 0.01f);

    std::remove("ramp.sl");
}

/**
 * A shader that is not an imager is refused rather than run over the frame. The library
 * already reports a name that is the wrong kind of shader; this is the second line of it,
 * for a caller that reached here with one anyway.
 **/
BOOST_AUTO_TEST_CASE(slimager_only_an_imager_test) {
    const InstancePtr surface = library().instance("matte", ShaderType::SURFACE, ParameterList());
    BOOST_REQUIRE(surface);

    FrameBuffer frame = drawn();
    v3d::render::offline::sl::Imager imager(surface, nullptr);
    BOOST_CHECK(!imager.run(&frame, COVERAGE));
    // and the frame is as it was
    BOOST_CHECK_SMALL(frame.value(0, 0, 0), 0.0001f);
}
