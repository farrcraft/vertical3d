/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/Manager.h>
#include <api/asset/Type.h>
#include <api/asset/kind/Json.h>
#include <api/asset/kind/Text.h>
#include <api/log/Logger.h>

#include <string>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

namespace {

boost::shared_ptr<v3d::asset::Manager> manager(const std::string& path = "data") {
    return boost::make_shared<v3d::asset::Manager>(path, boost::make_shared<v3d::log::Logger>());
}

};  // namespace

/**
 * A manager starts with the two loaders that read a document, and nothing else - the rest are
 * registered by api/asset/media and api/audio.
 **/
BOOST_AUTO_TEST_CASE(manager_loader_per_registered_type_test) {
    auto assets = manager();

    for (auto type : {v3d::asset::Type::JsonDocument, v3d::asset::Type::Text}) {
        auto loader = assets->resolveLoader(type);
        BOOST_TEST(static_cast<bool>(loader));
        BOOST_TEST((loader->type() == type));
    }
}

BOOST_AUTO_TEST_CASE(manager_unregistered_type_test) {
    auto assets = manager();

    BOOST_TEST(!assets->resolveLoader(v3d::asset::Type::Undefined));
    BOOST_TEST(!assets->resolveLoader(v3d::asset::Type::ImagePng));
    BOOST_TEST(!assets->load("pixel.png", v3d::asset::Type::ImagePng));
}

/**
 * The extension picks the loader, and the asset that comes back is the one that loader
 * builds, which the dynamic_pointer_cast here checks.
 **/
BOOST_AUTO_TEST_CASE(manager_type_from_extension_test) {
    auto assets = manager();

    auto document = assets->loadTypeFromExt("document.json");
    BOOST_TEST(static_cast<bool>(boost::dynamic_pointer_cast<v3d::asset::kind::Json>(document)));

    auto text = assets->loadTypeFromExt("plain.txt");
    BOOST_TEST(static_cast<bool>(boost::dynamic_pointer_cast<v3d::asset::kind::Text>(text)));
}

/**
 * A registered loader is reached by its type and by each of its extensions, whatever case
 * the file name spells the extension in.
 **/
BOOST_AUTO_TEST_CASE(manager_register_loader_test) {
    auto assets = manager();

    class Probe final : public v3d::asset::Loader {
     public:
        explicit Probe(const boost::shared_ptr<v3d::log::Logger>& logger) :
            Loader(v3d::asset::Type::ImageBmp, logger) {
        }
        boost::shared_ptr<v3d::asset::Asset> load(std::string_view name) override {
            asked_ = std::string(name);
            return boost::shared_ptr<v3d::asset::Asset>();
        }
        std::string asked_;
    };
    auto probe = boost::make_shared<Probe>(boost::make_shared<v3d::log::Logger>());
    assets->registerLoader(probe, {".bmp", ".dib"});

    BOOST_TEST((assets->resolveLoader(v3d::asset::Type::ImageBmp) == probe));
    assets->loadTypeFromExt("picture.DIB");
    BOOST_TEST(probe->asked_.find("picture.DIB") != std::string::npos);
}

/**
 * An extension nothing registered is no asset, the same as a missing file.
 **/
BOOST_AUTO_TEST_CASE(manager_unknown_extension_test) {
    auto assets = manager();

    BOOST_TEST(!assets->loadTypeFromExt("plain.qwe"));
    BOOST_TEST(!assets->loadTypeFromExt("document"));
}

/**
 * A typed load returns null for a file of another kind rather than the wrong object, and logs
 * the mismatch so it does not look like a missing file.
 **/
BOOST_AUTO_TEST_CASE(manager_typed_load_test) {
    auto assets = manager();

    auto document = assets->load<v3d::asset::kind::Json>("document.json");
    BOOST_REQUIRE(document);
    BOOST_TEST((document->type() == v3d::asset::Type::JsonDocument));
    BOOST_TEST(document->name().find("document.json") != std::string::npos);

    BOOST_TEST(!assets->load<v3d::asset::kind::Text>("document.json"));
    BOOST_TEST(!assets->load<v3d::asset::kind::Json>("absent.json"));
}

/**
 * A name is resolved against the manager's own path.
 **/
BOOST_AUTO_TEST_CASE(manager_path_resolution_test) {
    auto assets = manager("data");
    BOOST_TEST(static_cast<bool>(assets->load("document.json", v3d::asset::Type::JsonDocument)));

    auto rooted = manager("nowhere");
    BOOST_TEST(!rooted->load("document.json", v3d::asset::Type::JsonDocument));
}

/**
 * A file that is not there is a null asset, not a throw - Config::load reads its own return
 * that way and so does every consumer that guards the pointer.
 **/
BOOST_AUTO_TEST_CASE(manager_missing_asset_test) {
    auto assets = manager();

    BOOST_TEST(!assets->loadTypeFromExt("absent.json"));
}
