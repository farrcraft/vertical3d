/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/Pass.h>
#include <api/render/realtime/vulkan/frame/Recorder.h>
#include <api/render/realtime/vulkan/pipeline/Resources.h>

#include <stdexcept>

#include <boost/test/unit_test.hpp>

using v3d::render::realtime::Pass;
using v3d::render::realtime::vulkan::frame::Recorder;
using v3d::render::realtime::vulkan::pipeline::Pipeline;

namespace {

/**
 * Something to stand for a descriptor set. check() asks only whether a pass names one, so
 * nothing ever binds it.
 **/
int placeholder = 0;

VkDescriptorSet someSet() {
    return reinterpret_cast<VkDescriptorSet>(&placeholder);
}

};  // namespace

BOOST_AUTO_TEST_SUITE(recorder_check_test)

/**
 * A pipeline built with neither asks nothing of a pass, which is every pipeline in the tree
 * before ADR-0064.
 **/
BOOST_AUTO_TEST_CASE(a_plain_pipeline_draws_in_any_pass) {
    Pass pass("plain");
    BOOST_CHECK_NO_THROW(Recorder::check(pass, Pipeline()));
}

/**
 * A biased pipeline in a pass with no bias would draw at whatever bias was last set, which
 * vulkan does not report.
 **/
BOOST_AUTO_TEST_CASE(a_biased_pipeline_needs_a_biased_pass) {
    Pipeline biased;
    biased.biased = true;

    Pass unbiased("unbiased");
    BOOST_CHECK_THROW(Recorder::check(unbiased, biased), std::runtime_error);

    Pass shadow("shadow");
    shadow.depthBias(1.5f, 2.75f);
    BOOST_CHECK_NO_THROW(Recorder::check(shadow, biased));
}

/**
 * A pass that names a bias can still draw a pipeline built without one, so a shadow pass can
 * carry other geometry.
 **/
BOOST_AUTO_TEST_CASE(an_unbiased_pipeline_ignores_the_bias) {
    Pass shadow("shadow");
    shadow.depthBias(1.5f, 2.75f);
    BOOST_CHECK_NO_THROW(Recorder::check(shadow, Pipeline()));
}

/**
 * A pipeline declaring a set 2 needs the pass to name what is bound there.
 **/
BOOST_AUTO_TEST_CASE(a_scene_pipeline_needs_a_scene_set) {
    Pipeline lit;
    lit.scene = true;

    Pass bare("bare");
    BOOST_CHECK_THROW(Recorder::check(bare, lit), std::runtime_error);

    Pass scene("scene");
    scene.scene(someSet());
    BOOST_CHECK_NO_THROW(Recorder::check(scene, lit));
}

/**
 * A pipeline is built for one colour format and draws into one, and the pass is told which
 * pass drew into which rather than finding a wrong picture - ADR-0068.
 **/
BOOST_AUTO_TEST_CASE(a_pipeline_draws_only_into_its_colour_format) {
    Pipeline built;
    built.colourFormats = {VK_FORMAT_R8G8B8A8_UNORM};

    Recorder::Target into;
    into.format = VK_FORMAT_B8G8R8A8_UNORM;
    Pass pass("colour");
    BOOST_CHECK_THROW(Recorder::check(pass, built, into), std::runtime_error);

    into.format = VK_FORMAT_R8G8B8A8_UNORM;
    BOOST_CHECK_NO_THROW(Recorder::check(pass, built, into));
}

/**
 * Where either side states nothing there is nothing to compare: a target described without
 * its format, and a pipeline that writes no colour, which a shadow pipeline is.
 **/
BOOST_AUTO_TEST_CASE(an_unstated_format_is_not_checked) {
    Pipeline built;
    built.colourFormats = {VK_FORMAT_R8G8B8A8_UNORM};
    Pass pass("colour");
    BOOST_CHECK_NO_THROW(Recorder::check(pass, built, Recorder::Target()));

    Recorder::Target into;
    into.format = VK_FORMAT_B8G8R8A8_UNORM;
    BOOST_CHECK_NO_THROW(Recorder::check(pass, Pipeline(), into));
}

/**
 * The depth format is compared only for a pass that attaches depth, since one that does not
 * has nothing a pipeline's depth format could disagree with.
 **/
BOOST_AUTO_TEST_CASE(a_depth_pass_checks_the_depth_format) {
    Pipeline built;
    built.depthFormat = VK_FORMAT_D32_SFLOAT;

    Recorder::Target into;
    into.depthFormat = VK_FORMAT_D24_UNORM_S8_UINT;

    Pass flat("flat");
    BOOST_CHECK_NO_THROW(Recorder::check(flat, built, into));

    Pass tested("tested");
    tested.depth(true);
    BOOST_CHECK_THROW(Recorder::check(tested, built, into), std::runtime_error);

    into.depthFormat = VK_FORMAT_D32_SFLOAT;
    BOOST_CHECK_NO_THROW(Recorder::check(tested, built, into));
}

BOOST_AUTO_TEST_SUITE_END()
