/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/colour/Transform.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/integrate/Picture.h>
#include <moya/libmoya/integrate/Pictures.h>
#include <moya/libmoya/integrate/Render.h>
#include <moya/libmoya/integrate/Session.h>
#include <moya/libmoya/output/Png.h>
#include <moya/libmoya/scene/Change.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/vec3.hpp>

#include "Staged.h"
#include "Tracing.h"

namespace v3d::moya::tracing {
namespace {

namespace colour = v3d::moya::colour;
namespace film = v3d::moya::film;
namespace integrate = v3d::moya::integrate;
namespace output = v3d::moya::output;
namespace scene = v3d::moya::scene;
typedef v3d::moya::device::CpuDevice CpuDevice;

/** An edit of a scene whose objects have been applied. It commits nothing. **/
typedef std::function<void(Staged &)> Edit;

/**
 * Two quads of their own materials, red and blue, under a dome and a sphere light, in four passes.
 * Everything is committed and nothing applied.
 **/
void lit(Staged & staged) {
    const scene::Handle red = staged.material("red", { { "base_color", scene::Value::colour(glm::vec3 { 0.8F, 0.1F, 0.1F }) } });
    const scene::Handle blue = staged.material("blue", { { "base_color", scene::Value::colour(glm::vec3 { 0.1F, 0.1F, 0.8F }) } });
    staged.quad("left", { { -1.0F, -1.0F, 0.0F }, { 0.0F, -1.0F, 0.0F }, { 0.0F, 1.0F, 0.0F }, { -1.0F, 1.0F, 0.0F } }, red);
    staged.quad("right", { { 0.0F, -1.0F, 0.0F }, { 1.0F, -1.0F, 0.0F }, { 1.0F, 1.0F, 0.0F }, { 0.0F, 1.0F, 0.0F } }, blue);
    staged.light("domeLight", "sky", "");
    const scene::Handle bulb = staged.light("sphereLight", "bulb", "");
    staged.made.set(bulb, "radius", scene::Value::number(0.25F));
    staged.made.set(bulb, "transform", scene::Value::matrix(moved(0.5F, 0.5F, -2.0F)));
    staged.made.commit();
}

/** A staged scene of three bounces and 64 samples, four passes of them. **/
Staged staged() {
    return Staged(3, 64);
}

/** The object of a kind with a name, which the scene holds. **/
scene::Handle found(const Staged & staged, const std::string & kind, const std::string & name) {
    return *staged.made.find(kind, name);
}

/** The picture of the lit scene with every edit made, rendered from nothing. **/
film::Snapshot fresh(const std::vector<Edit> & edits) {
    Staged made = staged();
    lit(made);
    made.made.apply();
    for (const Edit & edit : edits) {
        edit(made);
        made.made.commit();
    }
    made.made.apply();
    CpuDevice device;
    return integrate::render(device, made.made, {}, colour::Config());
}

/** Whether two pictures hold the same samples and the same bytes in every channel. **/
bool sameBytes(const film::Snapshot & one, const film::Snapshot & other) {
    return one.values.size() == other.values.size() && one.samples == other.samples &&
        std::memcmp(one.values.data(), other.values.data(), one.values.size() * sizeof(float)) == 0;
}

/** The lit scene's first pass, from a render stopped after it. **/
film::Snapshot stoppedAfterOne() {
    Staged made = staged();
    lit(made);
    made.made.apply();
    CpuDevice device;
    return integrate::render(device, made.made, {}, colour::Config(), { .stop = [](std::uint32_t passes) { return passes == 1U; } });
}

/**
 * The picture of a session over the lit scene that takes one pass, then the edits, each a batch
 * of its own, and samples to its end.
 **/
film::Snapshot edited(const std::vector<Edit> & edits, bool * madeFilm = nullptr) {
    Staged made = staged();
    lit(made);
    CpuDevice device;
    const colour::Config config;
    integrate::Session session(device, made.made, {}, config);
    BOOST_TEST_REQUIRE(session.advance());
    BOOST_TEST(session.passes() == 1U);
    for (const Edit & edit : edits) {
        edit(made);
        made.made.commit();
    }
    session.edited();
    while (session.advance()) {
    }
    BOOST_TEST(session.version() == made.made.version());
    if (madeFilm != nullptr) {
        *madeFilm = session.madeFilm();
    }
    return session.picture();
}

/** Moves the camera to one side. **/
void cameraMoved(Staged & staged) {
    const scene::Handle camera = staged.made.value(staged.options, "camera").handleArray().front();
    staged.made.set(camera, "transform", scene::Value::matrix(moved(0.25F, 0.0F, -5.0F)));
}

/** Moves the left quad's instance. **/
void instanceMoved(Staged & staged) {
    staged.made.set(found(staged, "instance", "left"), "transform", scene::Value::matrix(moved(-0.25F, 0.25F, 0.0F)));
}

/** Makes the right quad's mesh a narrower one. **/
void geometryChanged(Staged & staged) {
    staged.made.set(found(staged, "mesh", "right"), "P",
        scene::Value::points({ { 0.0F, -1.0F, 0.0F }, { 0.5F, -1.0F, 0.0F }, { 0.5F, 1.0F, 0.0F }, { 0.0F, 1.0F, 0.0F } }));
}

/** Makes the red material green. **/
void materialChanged(Staged & staged) {
    staged.made.set(found(staged, "shader", "red"), "base_color", scene::Value::colour(glm::vec3 { 0.1F, 0.8F, 0.1F }));
}

/** Makes the sphere light brighter. **/
void lightChanged(Staged & staged) {
    staged.made.set(found(staged, "sphereLight", "bulb"), "intensity", scene::Value::number(4.0F));
}

/** Seeds the sampler from another frame. **/
void optionChanged(Staged & staged) {
    staged.made.set(staged.options, "frame", scene::Value::integer(2));
}

/** Writes the albedo beside the beauty. **/
void outputAdded(Staged & staged) {
    const scene::Handle albedo = staged.made.create("channel", "albedo");
    staged.made.set(albedo, "sourceType", scene::Value::string("raw"));
    staged.made.set(albedo, "sourceName", scene::Value::string("albedo"));
    staged.made.set(albedo, "components", scene::Value::integer(3));
    const scene::Handle out = staged.made.create("output", "picture");
    staged.made.set(out, "file", scene::Value::string("picture.exr"));
    staged.made.set(out, "channels", scene::Value::handles({ albedo }));
}

/** Places a third quad, of the blue material, in front of the two's upper half. **/
void objectCreated(Staged & staged) {
    staged.quad("front", { { -1.0F, 0.0F, -0.5F }, { 1.0F, 0.0F, -0.5F }, { 1.0F, 1.0F, -0.5F }, { -1.0F, 1.0F, -0.5F } }, found(staged, "material", "blue"));
}

/** Removes the right quad's instance. **/
void objectRemoved(Staged & staged) {
    staged.made.remove(found(staged, "instance", "right"));
}

/** Halves the picture's side. **/
void resolutionChanged(Staged & staged) {
    staged.made.set(staged.options, "resolution", scene::Value::integers({ Staged::kSide / 2, Staged::kSide / 2 }));
}

/**
 * Stops each pixel once its noise falls below a threshold some pixels reach before the end, checked
 * from four samples on, so that a check could come between the passes that grow.
 **/
void adaptive(Staged & staged) {
    staged.made.set(staged.options, "noiseThreshold", scene::Value::number(0.1F));
    staged.made.set(staged.options, "minSamples", scene::Value::integer(4));
}

/** Pictures into a list: coarse ones from blocks of four pixels, then one after every pass. **/
integrate::Pictures every(std::vector<integrate::Picture> & into) {
    return integrate::Pictures { .coarsest = 4, .shown = [&into](const integrate::Picture & picture) { into.push_back(picture); } };
}

/**
 * The lit scene rendered with one sample for each block of a side, as the display shows it, with
 * each block's colour given to every pixel in it.
 **/
std::vector<std::uint8_t> inBlocks(std::uint32_t side) {
    const std::int32_t blocks = Staged::kSide / static_cast<std::int32_t>(side);
    const film::Snapshot small = fresh({ [blocks](Staged & staged) {
        staged.made.set(staged.options, "resolution", scene::Value::integers({ blocks, blocks }));
        staged.made.set(staged.options, "samples", scene::Value::integer(1));
    } });
    const colour::Config config;
    const std::string display = config.defaultDisplay();
    const std::vector<std::uint8_t> shown = output::displayed(small, config.display(display, config.defaultView(display)));
    const auto pixels = static_cast<std::size_t>(Staged::kSide);
    std::vector<std::uint8_t> out(pixels * pixels * 3U);
    for (std::size_t y = 0; y < pixels; ++y) {
        for (std::size_t x = 0; x < pixels; ++x) {
            const std::size_t from = (((y / side) * static_cast<std::size_t>(blocks)) + (x / side)) * 3U;
            std::copy_n(shown.begin() + static_cast<std::ptrdiff_t>(from), 3, out.begin() + static_cast<std::ptrdiff_t>(((y * pixels) + x) * 3U));
        }
    }
    return out;
}

};  // namespace

BOOST_AUTO_TEST_SUITE(moya_session)

// a session that takes no edit ends where render() does
BOOST_AUTO_TEST_CASE(moya_session_unedited_ends_where_a_render_does_test) {
    Staged made = staged();
    lit(made);
    CpuDevice device;
    const colour::Config config;
    integrate::Session session(device, made.made, {}, config);
    std::uint32_t ran = 0;
    while (session.advance()) {
        ++ran;
    }
    BOOST_TEST(ran == 4U);
    BOOST_TEST(session.passes() == 4U);
    BOOST_TEST(sameBytes(session.picture(), fresh({})));
}

// after each kind of edit, a session edited after its first pass ends on the bytes of a render of
// the edited scene from nothing
BOOST_AUTO_TEST_CASE(moya_session_each_kind_of_edit_ends_where_a_fresh_render_does_test) {
    const std::vector<std::pair<std::string, Edit>> kinds { { "a camera", cameraMoved }, { "an instance's transformation", instanceMoved },
        { "a geometry", geometryChanged }, { "a material", materialChanged }, { "a light", lightChanged }, { "an option", optionChanged },
        { "an output", outputAdded }, { "an object created", objectCreated }, { "an object removed", objectRemoved } };
    for (const auto & [name, edit] : kinds) {
        const film::Snapshot want = fresh({ edit });
        BOOST_TEST(!sameBytes(want, fresh({})), name << " changes the picture");
        BOOST_TEST(sameBytes(edited({ edit }), want), name);
    }
}

// the film keeps its storage while its resolution and channels are the same, and is made again
// when either changes
BOOST_AUTO_TEST_CASE(moya_session_makes_a_film_only_for_new_pixels_or_channels_test) {
    bool madeFilm = true;
    BOOST_TEST(sameBytes(edited({ materialChanged }, &madeFilm), fresh({ materialChanged })));
    BOOST_TEST(!madeFilm);
    BOOST_TEST(sameBytes(edited({ resolutionChanged }, &madeFilm), fresh({ resolutionChanged })));
    BOOST_TEST(madeFilm);
    BOOST_TEST(sameBytes(edited({ outputAdded }, &madeFilm), fresh({ outputAdded })));
    BOOST_TEST(madeFilm);
}

// a session edited many times before a pass ends takes every batch
BOOST_AUTO_TEST_CASE(moya_session_takes_every_batch_committed_between_passes_test) {
    const std::vector<Edit> edits { cameraMoved, materialChanged, objectCreated, lightChanged };
    Staged made = staged();
    lit(made);
    CpuDevice device;
    const colour::Config config;
    integrate::Session session(device, made.made, {}, config);
    BOOST_TEST_REQUIRE(session.advance());
    const std::uint64_t before = session.version();
    for (const Edit & edit : edits) {
        edit(made);
        made.made.commit();
    }
    BOOST_TEST_REQUIRE(session.advance());
    BOOST_TEST(session.version() == before + edits.size());
    BOOST_TEST(session.passes() == 1U);
    while (session.advance()) {
    }
    BOOST_TEST(sameBytes(session.picture(), fresh(edits)));
}

// a commit made during a pass is not in that pass's samples, and is applied after it
BOOST_AUTO_TEST_CASE(moya_session_a_commit_during_a_pass_waits_for_its_end_test) {
    Staged made = staged();
    lit(made);
    made.made.apply();
    const scene::Handle red = found(made, "shader", "red");
    CpuDevice device;
    const colour::Config config;
    std::uint32_t asks = 0;
    // the stop is asked before the pass and then before its one tile, where the edit is committed
    const integrate::Checkpoints checkpoints { .stop = [&](std::uint32_t) {
        if (++asks == 2U) {
            made.made.set(red, "base_color", scene::Value::colour(glm::vec3 { 0.1F, 0.8F, 0.1F }));
            made.made.commit();
        }
        return false;
    } };
    integrate::Session session(device, made.made, {}, config, checkpoints);
    const std::uint64_t before = made.made.version();
    BOOST_TEST_REQUIRE(session.advance());
    BOOST_TEST_REQUIRE(asks >= 2U);
    BOOST_TEST(session.passes() == 1U);
    BOOST_TEST(session.version() == before);
    // the first pass of the scene before the edit
    BOOST_TEST(sameBytes(session.picture(), stoppedAfterOne()));
    BOOST_TEST_REQUIRE(session.advance());
    BOOST_TEST(session.version() == before + 1U);
    while (session.advance()) {
    }
    BOOST_TEST(sameBytes(session.picture(), fresh({ materialChanged })));
}

// an edit said during a pass drops it: the film holds the passes before it, and the session
// applies the edit next
BOOST_AUTO_TEST_CASE(moya_session_an_edit_drops_the_pass_in_progress_test) {
    Staged made = staged();
    lit(made);
    CpuDevice device;
    const colour::Config config;
    made.made.apply();
    const scene::Handle red = found(made, "shader", "red");
    integrate::Session * held = nullptr;
    bool said = false;
    const integrate::Checkpoints checkpoints { .stop = [&](std::uint32_t passes) {
        if (passes == 1U && !said) {
            made.made.set(red, "base_color", scene::Value::colour(glm::vec3 { 0.1F, 0.8F, 0.1F }));
            made.made.commit();
            held->edited();
            said = true;
        }
        return false;
    } };
    integrate::Session session(device, made.made, {}, config, checkpoints);
    held = &session;
    BOOST_TEST_REQUIRE(session.advance());
    const std::uint64_t before = session.version();
    // the edit is said before the second pass, whose one tile it then drops
    BOOST_TEST_REQUIRE(session.advance());
    BOOST_TEST(said);
    BOOST_TEST(session.passes() == 1U);
    BOOST_TEST(session.version() == before);
    BOOST_TEST_REQUIRE(session.advance());
    BOOST_TEST(session.version() == before + 1U);
    BOOST_TEST(session.passes() == 1U);
    while (session.advance()) {
    }
    BOOST_TEST(sameBytes(session.picture(), fresh({ materialChanged })));
}

// a session on its own thread takes the edits clients commit and say while it runs, and ends
// where a fresh render of the edited scene does
BOOST_AUTO_TEST_CASE(moya_session_on_its_own_thread_takes_edits_test) {
    Staged made = staged();
    lit(made);
    made.made.apply();
    const scene::Handle camera = made.made.value(made.options, "camera").handleArray().front();
    const scene::Handle right = found(made, "instance", "right");
    const scene::Handle bulb = found(made, "sphereLight", "bulb");
    CpuDevice device;
    const colour::Config config;
    integrate::Session session(device, made.made, {}, config);
    session.start();
    made.made.set(camera, "transform", scene::Value::matrix(moved(0.25F, 0.0F, -5.0F)));
    made.made.commit();
    session.edited();
    made.made.remove(right);
    made.made.commit();
    session.edited();
    made.made.set(bulb, "intensity", scene::Value::number(4.0F));
    made.made.commit();
    session.edited();
    session.wait();
    session.stop();
    BOOST_TEST(session.passes() == 4U);
    BOOST_TEST(sameBytes(session.picture(), fresh({ cameraMoved, objectRemoved, lightChanged })));
}

// the session forgets the scene's removals once it has built past them
BOOST_AUTO_TEST_CASE(moya_session_forgets_the_removals_it_built_past_test) {
    Staged made = staged();
    lit(made);
    CpuDevice device;
    const colour::Config config;
    integrate::Session session(device, made.made, {}, config);
    BOOST_TEST_REQUIRE(session.advance());
    objectRemoved(made);
    made.made.commit();
    const std::uint64_t built = session.version();
    BOOST_TEST_REQUIRE(session.advance());
    const std::vector<scene::Change> changes = made.made.changes(built);
    BOOST_TEST(std::none_of(changes.begin(), changes.end(), [](const scene::Change & change) { return change.what == scene::Change::What::Removed; }));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(moya_session_pictures)

// a session that makes pictures shows coarse blocks first, then passes that double the samples the
// film holds, and each picture holds more than the one before
BOOST_AUTO_TEST_CASE(moya_session_pictures_come_before_the_last_pass_and_each_holds_more_test) {
    Staged made = staged();
    lit(made);
    CpuDevice device;
    const colour::Config config;
    integrate::Session session(device, made.made, {}, config);
    std::vector<integrate::Picture> pictures;
    session.show(every(pictures));
    while (session.advance()) {
    }
    std::vector<std::uint32_t> blocks;
    std::vector<std::uint32_t> samples;
    for (const integrate::Picture & picture : pictures) {
        blocks.push_back(picture.block);
        samples.push_back(picture.samples);
        BOOST_TEST(picture.version == session.version());
        BOOST_TEST(picture.resolution.x == static_cast<std::uint32_t>(Staged::kSide));
        BOOST_TEST(picture.rgb.size() == static_cast<std::size_t>(Staged::kSide * Staged::kSide * 3));
    }
    const std::vector<std::uint32_t> wantBlocks { 4, 2, 1, 1, 1, 1, 1, 1, 1, 1 };
    const std::vector<std::uint32_t> wantSamples { 1, 1, 1, 2, 4, 8, 16, 32, 48, 64 };
    BOOST_TEST(blocks == wantBlocks, boost::test_tools::per_element());
    BOOST_TEST(samples == wantSamples, boost::test_tools::per_element());
    BOOST_TEST(session.passes() == 8U);
    BOOST_TEST_REQUIRE(pictures.size() >= 3U);
    // a coarse picture is a render of one sample a block
    BOOST_TEST((pictures[0].rgb == inBlocks(4U)));
    BOOST_TEST((pictures[1].rgb == inBlocks(2U)));
    BOOST_TEST((pictures[2].rgb != inBlocks(2U)));
}

// the pictures after an edit name the new version, and start again from coarse blocks
BOOST_AUTO_TEST_CASE(moya_session_pictures_after_an_edit_name_the_new_version_test) {
    Staged made = staged();
    lit(made);
    CpuDevice device;
    const colour::Config config;
    integrate::Session session(device, made.made, {}, config);
    std::vector<integrate::Picture> pictures;
    session.show(every(pictures));
    for (std::uint32_t i = 0; i < 4U; ++i) {
        BOOST_TEST_REQUIRE(session.advance());
    }
    const std::uint64_t before = session.version();
    BOOST_TEST_REQUIRE(!pictures.empty());
    BOOST_TEST(pictures.back().version == before);
    BOOST_TEST(pictures.back().samples == 2U);
    materialChanged(made);
    made.made.commit();
    session.edited();
    pictures.clear();
    BOOST_TEST_REQUIRE(session.advance());
    BOOST_TEST_REQUIRE(pictures.size() == 1U);
    BOOST_TEST(pictures.front().version == before + 1U);
    BOOST_TEST(pictures.front().block == 4U);
    while (session.advance()) {
    }
    BOOST_TEST(pictures.back().version == before + 1U);
    BOOST_TEST(pictures.back().samples == 64U);
    BOOST_TEST(sameBytes(session.picture(), fresh({ materialChanged })));
}

// an edit said during a coarse pass drops it, and its picture is not made
BOOST_AUTO_TEST_CASE(moya_session_pictures_an_edit_drops_a_coarse_pass_test) {
    Staged made = staged();
    lit(made);
    made.made.apply();
    const scene::Handle red = found(made, "shader", "red");
    CpuDevice device;
    const colour::Config config;
    integrate::Session * held = nullptr;
    bool said = false;
    // the stop is first asked before the first pass, which the edit then drops at its one tile
    const integrate::Checkpoints checkpoints { .stop = [&](std::uint32_t) {
        if (!said) {
            made.made.set(red, "base_color", scene::Value::colour(glm::vec3 { 0.1F, 0.8F, 0.1F }));
            made.made.commit();
            held->edited();
            said = true;
        }
        return false;
    } };
    integrate::Session session(device, made.made, {}, config, checkpoints);
    held = &session;
    std::vector<integrate::Picture> pictures;
    session.show(every(pictures));
    const std::uint64_t before = made.made.version();
    BOOST_TEST_REQUIRE(session.advance());
    BOOST_TEST(said);
    BOOST_TEST(pictures.empty());
    BOOST_TEST(session.version() == before);
    BOOST_TEST_REQUIRE(session.advance());
    BOOST_TEST_REQUIRE(pictures.size() == 1U);
    BOOST_TEST(pictures.front().version == before + 1U);
    BOOST_TEST(pictures.front().block == 4U);
    while (session.advance()) {
    }
    BOOST_TEST(sameBytes(session.picture(), fresh({ materialChanged })));
}

// the last picture, and the film, are those of a render that made no pictures, though pixels stop
// on their noise as the passes of growing size end
BOOST_AUTO_TEST_CASE(moya_session_pictures_end_on_a_render_that_made_none_test) {
    Staged made = staged();
    lit(made);
    made.made.apply();
    adaptive(made);
    made.made.commit();
    CpuDevice device;
    const colour::Config config;
    integrate::Session session(device, made.made, {}, config);
    std::vector<integrate::Picture> pictures;
    session.show(every(pictures));
    while (session.advance()) {
    }
    const film::Snapshot want = fresh({ adaptive });
    BOOST_TEST(std::any_of(want.samples.begin(), want.samples.end(), [](std::uint32_t count) { return count < 64U; }));
    BOOST_TEST(std::any_of(want.samples.begin(), want.samples.end(), [](std::uint32_t count) { return count > 16U; }));
    BOOST_TEST(sameBytes(session.picture(), want));
    const std::string display = config.defaultDisplay();
    BOOST_TEST_REQUIRE(!pictures.empty());
    BOOST_TEST((pictures.back().rgb == output::displayed(want, config.display(display, config.defaultView(display)))));
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace v3d::moya::tracing
