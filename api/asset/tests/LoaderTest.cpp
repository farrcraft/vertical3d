/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/Asset.h>
#include <api/asset/Loader.h>
#include <api/asset/kind/Text.h>
#include <api/log/Logger.h>

#include <string_view>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

namespace {

/**
 * Loader's constructor is protected and the case below is about the base class rather than
 * about any format, so the suite supplies its own leaf.
 **/
class Probe final : public v3d::asset::Loader {
 public:
    explicit Probe(const boost::shared_ptr<v3d::log::Logger>& logger) :
        Loader(typeid(v3d::asset::kind::Text), logger) {
    }

    boost::shared_ptr<v3d::asset::Asset> load(std::string_view /* name */) override {
        return boost::shared_ptr<v3d::asset::Asset>();
    }
};

Probe probe() {
    return Probe(boost::make_shared<v3d::log::Logger>());
}

BOOST_AUTO_TEST_CASE(loader_kind_test) {
    auto loader = probe();

    BOOST_TEST((loader.kind() == typeid(v3d::asset::kind::Text)));
}

};  // namespace
