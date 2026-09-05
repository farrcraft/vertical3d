/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <string>
#include <variant>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

#include "../Loader.h"
#include "../Manager.h"
#include "../Type.h"

namespace {

/**
 * Loader's constructor is protected and every parameter case below is about the base
 * class rather than about any format, so the suite supplies its own leaf.
 **/
class Probe final : public v3d::asset::Loader {
 public:
    explicit Probe(const boost::shared_ptr<v3d::log::Logger>& logger) :
        Loader(nullptr, v3d::asset::Type::Text, logger) {
    }

    boost::shared_ptr<v3d::asset::Asset> load(std::string_view /* name */) override {
        return boost::shared_ptr<v3d::asset::Asset>();
    }
};

Probe probe() {
    return Probe(boost::make_shared<v3d::log::Logger>());
}

};  // namespace

BOOST_AUTO_TEST_CASE(loader_type_test) {
    auto loader = probe();

    BOOST_TEST((loader.type() == v3d::asset::Type::Text));
}

BOOST_AUTO_TEST_CASE(loader_parameter_test) {
    auto loader = probe();

    loader.parameter("width", v3d::asset::ParameterValue(static_cast<unsigned int>(64)));
    loader.parameter("scale", v3d::asset::ParameterValue(1.5f));
    loader.parameter("face", v3d::asset::ParameterValue(std::string("regular")));

    auto width = loader.parameter("width");
    BOOST_TEST(static_cast<bool>(width));
    BOOST_TEST(std::get<unsigned int>(*width) == 64u);

    auto scale = loader.parameter("scale");
    BOOST_TEST(static_cast<bool>(scale));
    BOOST_TEST(std::get<float>(*scale) == 1.5f);

    auto face = loader.parameter("face");
    BOOST_TEST(static_cast<bool>(face));
    BOOST_TEST(std::get<std::string>(*face) == "regular");
}

/**
 * A second write of the same name replaces the first, which is what a loader reused across
 * two loads depends on - the font loaders set a size per face.
 **/
BOOST_AUTO_TEST_CASE(loader_parameter_replacement_test) {
    auto loader = probe();

    loader.parameter("size", v3d::asset::ParameterValue(static_cast<unsigned int>(12)));
    loader.parameter("size", v3d::asset::ParameterValue(static_cast<unsigned int>(24)));

    auto size = loader.parameter("size");
    BOOST_TEST(static_cast<bool>(size));
    BOOST_TEST(std::get<unsigned int>(*size) == 24u);
}

BOOST_AUTO_TEST_CASE(loader_absent_parameter_test) {
    auto loader = probe();

    BOOST_TEST(!loader.parameter("size"));

    loader.parameter("size", v3d::asset::ParameterValue(static_cast<unsigned int>(12)));
    loader.reset();

    // reset() clears intermediate state per its own contract; parameters are not that state.
    BOOST_TEST(static_cast<bool>(loader.parameter("size")));
}
