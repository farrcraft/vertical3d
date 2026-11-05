/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/font/face/Face.h>
#include <api/font/face/Glyph.h>
#include <api/font/face/Family.h>
#include <api/font/face/FallbackChain.h>
#include <api/font/face/Library.h>
#include <api/log/Logger.h>

#include <string>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

namespace {

const char* const kInter = "data/fonts/Inter-Regular.ttf";
const char* const kInterSemiBold = "data/fonts/Inter-SemiBold.ttf";
const char* const kNoto = "data/fonts/NotoSans-Regular.ttf";

// a capital DZ with a caron, which Noto Sans has and Inter does not
const char32_t kDzCaron = 0x01C4;
// a CJK ideograph, which neither has
const char32_t kIdeograph = 0x4E00;

/**
 * The shipped faces, opened through one library.
 **/
struct Faces {
    Faces() :
        logger(boost::make_shared<v3d::log::Logger>()),
        library(boost::make_shared<v3d::font::face::Library>()),
        regular(v3d::font::face::Face::open(library, kInter, logger)),
        semiBold(v3d::font::face::Face::open(library, kInterSemiBold, logger)),
        noto(v3d::font::face::Face::open(library, kNoto, logger)),
        inter(boost::make_shared<v3d::font::face::Family>("Inter")),
        fallback(boost::make_shared<v3d::font::face::Family>("Noto Sans")) {
        BOOST_REQUIRE(regular && semiBold && noto);
        inter->add(regular);
        inter->add(semiBold);
        fallback->add(noto);
    }

    boost::shared_ptr<v3d::log::Logger> logger;
    boost::shared_ptr<v3d::font::face::Library> library;
    boost::shared_ptr<v3d::font::face::Face> regular;
    boost::shared_ptr<v3d::font::face::Face> semiBold;
    boost::shared_ptr<v3d::font::face::Face> noto;
    boost::shared_ptr<v3d::font::face::Family> inter;
    boost::shared_ptr<v3d::font::face::Family> fallback;
};

BOOST_FIXTURE_TEST_SUITE(fallback_chain_test, Faces)

/**
 * A face names its family, its weight from the OS/2 table, and the glyph index of each code
 * point it has.
 **/
BOOST_AUTO_TEST_CASE(a_face_reads_its_name_weight_and_glyphs) {
    BOOST_CHECK_EQUAL(regular->family(), "Inter");
    BOOST_CHECK_EQUAL(regular->weight(), 400U);
    BOOST_CHECK_EQUAL(semiBold->weight(), 600U);
    BOOST_CHECK(!regular->italic());
    BOOST_CHECK(regular->has(U'A'));
    BOOST_CHECK(regular->glyphIndex(U'A') != 0U);
    BOOST_CHECK(!regular->has(kDzCaron));
    BOOST_CHECK(noto->has(kDzCaron));
}

/**
 * A file that does not open gives no face, a family leaves it out, and a chain leaves out a
 * family with no faces.
 **/
BOOST_AUTO_TEST_CASE(a_face_that_will_not_open_is_left_out) {
    const boost::shared_ptr<v3d::font::face::Face> missing = v3d::font::face::Face::open(library, "data/fonts/missing.ttf", logger);
    BOOST_CHECK(!missing);
    const boost::shared_ptr<v3d::font::face::Family> empty = boost::make_shared<v3d::font::face::Family>("missing");
    empty->add(missing);
    BOOST_CHECK(empty->empty());
    BOOST_CHECK(!empty->face({ .weight = 400 }));

    v3d::font::face::FallbackChain chain;
    chain.add(empty);
    chain.add(inter);
    BOOST_REQUIRE_EQUAL(chain.families().size(), 1U);
    BOOST_CHECK(chain.find(U'A', { .weight = 400 }).face == regular);
}

/**
 * A family gives the face of the weight asked for, and otherwise the one CSS would choose.
 **/
BOOST_AUTO_TEST_CASE(a_family_chooses_a_weight_as_css_does) {
    BOOST_CHECK(inter->face({ .weight = 400 }) == regular);
    BOOST_CHECK(inter->face({ .weight = 600 }) == semiBold);
    // 500 looks heavier up to 500 first, then lighter
    BOOST_CHECK(inter->face({ .weight = 500 }) == regular);
    BOOST_CHECK(inter->face({ .weight = 450 }) == regular);
    // above 500 looks heavier first, then lighter
    BOOST_CHECK(inter->face({ .weight = 700 }) == semiBold);
    BOOST_CHECK(inter->face({ .weight = 550 }) == semiBold);
    // below 400 looks lighter first, then heavier
    BOOST_CHECK(inter->face({ .weight = 300 }) == regular);
    // no italic face, so the upright faces are chosen from
    BOOST_CHECK(inter->face({ .weight = 600, .italic = true }) == semiBold);
}

/**
 * A code point is drawn from the first family in the chain that has it.
 **/
BOOST_AUTO_TEST_CASE(the_chain_tries_its_families_in_order) {
    v3d::font::face::FallbackChain chain;
    chain.add(inter);
    chain.add(fallback);
    BOOST_CHECK(chain.find(U'A', { .weight = 400 }).face == regular);
    BOOST_CHECK(chain.find(U'A', { .weight = 600 }).face == semiBold);
    const v3d::font::face::Glyph dz = chain.find(kDzCaron, { .weight = 600 });
    BOOST_CHECK(dz.face == noto);
    BOOST_CHECK_EQUAL(dz.index, noto->glyphIndex(kDzCaron));

    v3d::font::face::FallbackChain reversed;
    reversed.add(fallback);
    reversed.add(inter);
    BOOST_CHECK(reversed.find(U'A', { .weight = 400 }).face == noto);
}

/**
 * A code point no face has is drawn as the first face's missing glyph, and a chain with no
 * families draws nothing.
 **/
BOOST_AUTO_TEST_CASE(a_code_point_no_face_has_is_the_first_faces_missing_glyph) {
    v3d::font::face::FallbackChain chain;
    chain.add(inter);
    chain.add(fallback);
    const v3d::font::face::Glyph missing = chain.find(kIdeograph, { .weight = 600 });
    BOOST_CHECK(missing.face == semiBold);
    BOOST_CHECK_EQUAL(missing.index, 0U);

    BOOST_CHECK(!v3d::font::face::FallbackChain().find(U'A', { .weight = 400 }).face);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
