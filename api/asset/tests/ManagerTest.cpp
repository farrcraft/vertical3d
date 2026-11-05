/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/Asset.h>
#include <api/asset/Loader.h>
#include <api/asset/Manager.h>
#include <api/asset/kind/Json.h>
#include <api/asset/kind/Text.h>
#include <api/log/Logger.h>

#include <stdexcept>
#include <string>
#include <string_view>
#include <typeinfo>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/make_shared.hpp>

namespace {

/**
 * A loader of text that records the name it was asked for and returns nothing, so a case can
 * see which of two loaders of one kind a load reached.
 **/
class Probe final : public v3d::asset::Loader {
 public:
    explicit Probe(const boost::shared_ptr<v3d::log::Logger>& logger) :
        Loader(typeid(v3d::asset::kind::Text), logger) {
    }
    boost::shared_ptr<v3d::asset::Asset> load(std::string_view name) override {
        asked_ = std::string(name);
        return boost::shared_ptr<v3d::asset::Asset>();
    }
    std::string asked_;
};

boost::shared_ptr<v3d::asset::Manager> manager(const std::string& path = "data") {
    return boost::make_shared<v3d::asset::Manager>(path, boost::make_shared<v3d::log::Logger>());
}

/**
 * A manager starts with the document loaders and nothing else. api/asset/media and api/audio
 * register the rest.
 **/
BOOST_AUTO_TEST_CASE(manager_loader_per_registered_kind_test) {
    auto assets = manager();

    auto json = assets->resolveLoader<v3d::asset::kind::Json>();
    BOOST_REQUIRE(json);
    BOOST_TEST((json->kind() == typeid(v3d::asset::kind::Json)));
    auto text = assets->resolveLoader<v3d::asset::kind::Text>();
    BOOST_REQUIRE(text);
    BOOST_TEST((text->kind() == typeid(v3d::asset::kind::Text)));
}

BOOST_AUTO_TEST_CASE(manager_unregistered_kind_test) {
    auto assets = manager();

    BOOST_TEST(!assets->resolveLoader<v3d::asset::Asset>());
    BOOST_TEST(!assets->load<v3d::asset::Asset>("notes"));
}

/**
 * The extension picks the loader, and the asset that comes back is the one that loader
 * builds, which the dynamic_pointer_cast here checks.
 **/
BOOST_AUTO_TEST_CASE(manager_kind_from_extension_test) {
    auto assets = manager();

    auto document = assets->load("document.json");
    BOOST_TEST(static_cast<bool>(boost::dynamic_pointer_cast<v3d::asset::kind::Json>(document)));

    auto text = assets->load("plain.txt");
    BOOST_TEST(static_cast<bool>(boost::dynamic_pointer_cast<v3d::asset::kind::Text>(text)));
}

/**
 * A registered loader is reached by its kind and by each of its extensions, whatever case
 * the file name spells the extension in.
 **/
BOOST_AUTO_TEST_CASE(manager_register_loader_test) {
    auto assets = manager();

    auto probe = boost::make_shared<Probe>(boost::make_shared<v3d::log::Logger>());
    assets->registerLoader(probe, {".log", ".md"});

    BOOST_TEST((assets->resolveLoader<v3d::asset::kind::Text>() == probe));
    assets->load("picture.MD");
    BOOST_TEST(probe->asked_.find("picture.MD") != std::string::npos);
}

/**
 * A second loader of one kind takes the files whose extension no loader reads, and the
 * extensions it was registered with. The first keeps its own extensions.
 **/
BOOST_AUTO_TEST_CASE(manager_two_loaders_of_one_kind_test) {
    auto assets = manager();

    auto probe = boost::make_shared<Probe>(boost::make_shared<v3d::log::Logger>());
    assets->registerLoader(probe, {".log"});

    auto text = assets->load<v3d::asset::kind::Text>("plain.txt");
    BOOST_REQUIRE(text);
    BOOST_TEST(text->content() == "one\ntwo\n");
    BOOST_TEST(probe->asked_.empty());

    assets->load<v3d::asset::kind::Text>("notes");
    BOOST_TEST(probe->asked_.find("notes") != std::string::npos);
}

/**
 * An extension nothing registered is no asset, the same as a missing file, when the caller
 * names no kind.
 **/
BOOST_AUTO_TEST_CASE(manager_unknown_extension_test) {
    auto assets = manager();

    BOOST_TEST(!assets->load("plain.qwe"));
    BOOST_TEST(!assets->load("notes"));
}

/**
 * A file whose extension no loader reads is loaded by the loader for the kind the caller
 * names.
 **/
BOOST_AUTO_TEST_CASE(manager_kind_without_extension_test) {
    auto assets = manager();

    auto notes = assets->load<v3d::asset::kind::Text>("notes");
    BOOST_REQUIRE(notes);
    BOOST_TEST(notes->content() == "no extension\n");
}

/**
 * A typed load returns null for a file of another kind rather than the wrong object, and logs
 * the mismatch so it does not look like a missing file. The extension wins over the kind.
 **/
BOOST_AUTO_TEST_CASE(manager_typed_load_test) {
    auto assets = manager();

    auto document = assets->load<v3d::asset::kind::Json>("document.json");
    BOOST_REQUIRE(document);
    BOOST_TEST(document->name().find("document.json") != std::string::npos);

    BOOST_TEST(!assets->load<v3d::asset::kind::Text>("document.json"));
    BOOST_TEST(!assets->load<v3d::asset::kind::Json>("absent.json"));
}

/**
 * A name is resolved against the manager's own path.
 **/
BOOST_AUTO_TEST_CASE(manager_path_resolution_test) {
    auto assets = manager("data");
    BOOST_TEST(static_cast<bool>(assets->load<v3d::asset::kind::Json>("document.json")));

    auto rooted = manager("nowhere");
    BOOST_TEST(!rooted->load<v3d::asset::kind::Json>("document.json"));
}

/**
 * A name only a later root holds is found there, so an app reads the shared data through the
 * same manager as its own.
 **/
BOOST_AUTO_TEST_CASE(manager_later_root_test) {
    v3d::asset::Manager assets(std::vector<std::string>{ "nowhere", "data" }, boost::make_shared<v3d::log::Logger>());

    BOOST_TEST(static_cast<bool>(assets.load<v3d::asset::kind::Json>("document.json")));
}

/**
 * The first root that holds a name wins, so an app's own file is read before a shared file of
 * the same name.
 **/
BOOST_AUTO_TEST_CASE(manager_first_root_wins_test) {
    const boost::filesystem::path base = boost::filesystem::temp_directory_path() / "v3d_manager_roots";
    boost::filesystem::remove_all(base);
    for (const char* root : { "own", "shared" }) {
        boost::filesystem::create_directories(base / root);
        boost::filesystem::ofstream(base / root / "same.txt") << root;
    }
    const v3d::asset::Manager assets(std::vector<std::string>{ (base / "own").string(), (base / "shared").string() },
                               boost::make_shared<v3d::log::Logger>());

    BOOST_CHECK_EQUAL(assets.path("same.txt"), (base / "own" / "same.txt").string());
    boost::filesystem::remove_all(base);
}

/**
 * A name no root holds resolves against the first root, so the failure names a path in it.
 **/
BOOST_AUTO_TEST_CASE(manager_missing_name_resolves_against_the_first_root_test) {
    const v3d::asset::Manager assets(std::vector<std::string>{ "first", "data" }, boost::make_shared<v3d::log::Logger>());

    BOOST_CHECK_EQUAL(assets.path("absent.json"), (boost::filesystem::path("first") / "absent.json").string());
}

/**
 * A manager with no root has nowhere to resolve a name, and is refused.
 **/
BOOST_AUTO_TEST_CASE(manager_without_a_root_test) {
    BOOST_CHECK_THROW(v3d::asset::Manager(std::vector<std::string>{}, boost::make_shared<v3d::log::Logger>()),
                      std::invalid_argument);
}

/**
 * A file that is not there is a null asset, not a throw. The manifest and every consumer
 * that checks the pointer rely on this.
 **/
BOOST_AUTO_TEST_CASE(manager_missing_asset_test) {
    auto assets = manager();

    BOOST_TEST(!assets->load("absent.json"));
}

};  // namespace
