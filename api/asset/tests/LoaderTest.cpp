/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/Loader.h>
#include <api/asset/Manager.h>
#include <api/asset/Type.h>
#include <api/log/Logger.h>

#include <string>

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
        Loader(v3d::asset::Type::Text, logger) {
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
