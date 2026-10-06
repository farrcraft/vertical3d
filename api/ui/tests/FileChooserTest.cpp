/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/ui/Engine.h>
#include <api/ui/component/SelectList.h>
#include <api/ui/shell/FileChooser.h>

#include <fstream>
#include <string>
#include <vector>

#include <boost/filesystem/operations.hpp>
#include <boost/json.hpp>
#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

#include <entt/entt.hpp>

namespace {

/**
 * A directory of its own per case, holding two directories and three files out of name order,
 * removed with everything under it.
 **/
class Sandbox final {
 public:
    explicit Sandbox(const std::string& name) :
        path_(boost::filesystem::temp_directory_path() / ("v3d_chooser_" + name)) {
        boost::system::error_code ignored;
        boost::filesystem::remove_all(path_, ignored);
        boost::filesystem::create_directories(path_ / "zoo");
        boost::filesystem::create_directories(path_ / "art");
        touch("b.json");
        touch("a.json");
        touch("notes.txt");
    }

    ~Sandbox() {
        boost::system::error_code ignored;
        boost::filesystem::remove_all(path_, ignored);
    }

    Sandbox(const Sandbox&) = delete;
    Sandbox& operator=(const Sandbox&) = delete;

    void touch(const std::string& name) const {
        std::ofstream((path_ / name).string()) << "{}";
    }

    boost::filesystem::path path() const {
        return boost::filesystem::canonical(path_);
    }

 private:
    boost::filesystem::path path_;
};

/**
 * A container holding what a chooser writes into: the list, the name field and the folder
 * label, under the names a chooser looks for by default.
 **/
boost::shared_ptr<v3d::ui::Engine> chooserUi() {
    boost::shared_ptr<entt::dispatcher> dispatcher = boost::make_shared<entt::dispatcher>();
    boost::shared_ptr<v3d::ui::Engine> ui = boost::make_shared<v3d::ui::Engine>(
        boost::make_shared<v3d::event::Engine>(dispatcher), dispatcher, boost::make_shared<v3d::log::Logger>());
    const char* const document = R"({
      "themes": [ { "name": "default" } ],
      "containers": [
        {
          "name": "chooser",
          "visible": false,
          "components": [
            { "name": "files", "type": "list" },
            { "name": "name", "type": "textbox" },
            { "name": "folder", "type": "label" }
          ]
        }
      ]
    })";
    BOOST_REQUIRE(ui->load(boost::json::parse(document).as_object()));
    return ui;
}

/**
 * The names of a listing, in order, with a directory marked by a trailing slash.
 **/
std::vector<std::string> names(const v3d::ui::shell::FileChooser& chooser) {
    std::vector<std::string> found;
    for (const v3d::ui::shell::FileChooser::Entry& entry : chooser.entries()) {
        found.push_back(entry.directory ? entry.name + "/" : entry.name);
    }
    return found;
}

};  // namespace

BOOST_AUTO_TEST_SUITE(file_chooser_test)

/**
 * A listing is the way up, then the directories, then the files that pass the filter, each
 * sorted by name.
 **/
BOOST_AUTO_TEST_CASE(a_listing_is_directories_then_the_files_that_pass) {
    const Sandbox sandbox("listing");
    v3d::ui::shell::FileChooser chooser(nullptr);
    chooser.open(v3d::ui::shell::FileChooser::Mode::Open, sandbox.path(), ".json", {});

    BOOST_CHECK(chooser.visible());
    BOOST_CHECK(chooser.directory() == sandbox.path());
    const std::vector<std::string> expected = { "../", "art/", "zoo/", "a.json", "b.json" };
    BOOST_CHECK(names(chooser) == expected);
}

/**
 * Stepping into a directory and up out of it again gives back the listing it started from.
 **/
BOOST_AUTO_TEST_CASE(stepping_in_and_up_returns_to_the_listing) {
    const Sandbox sandbox("stepping");
    v3d::ui::shell::FileChooser chooser(nullptr);
    chooser.open(v3d::ui::shell::FileChooser::Mode::Open, sandbox.path(), ".json", {});
    const std::vector<std::string> before = names(chooser);

    chooser.pick(1);
    BOOST_CHECK(chooser.directory() == sandbox.path() / "art");
    BOOST_CHECK(names(chooser) == std::vector<std::string>{ "../" });

    chooser.pick(0);
    BOOST_CHECK(chooser.directory() == sandbox.path());
    BOOST_CHECK(names(chooser) == before);
}

/**
 * A file's row puts its name in the field, and accepting it chooses that file in the
 * directory shown, closing the chooser.
 **/
BOOST_AUTO_TEST_CASE(a_picked_file_is_chosen_in_its_directory) {
    const Sandbox sandbox("picked");
    boost::filesystem::path chosen;
    v3d::ui::shell::FileChooser chooser(nullptr);
    chooser.open(v3d::ui::shell::FileChooser::Mode::Open, sandbox.path(), ".json",
        [&chosen](const boost::filesystem::path& path) { chosen = path; });

    chooser.pick(4);
    BOOST_CHECK_EQUAL(chooser.name(), "b.json");
    BOOST_CHECK(chooser.accept());
    BOOST_CHECK(chosen == sandbox.path() / "b.json");
    BOOST_CHECK(!chooser.visible());
}

/**
 * A name that is empty, that would step out of the directory, or that names no file to open
 * is refused, and nothing is chosen.
 **/
BOOST_AUTO_TEST_CASE(a_name_that_is_not_one_is_refused) {
    const Sandbox sandbox("refused");
    bool called = false;
    v3d::ui::shell::FileChooser chooser(nullptr);
    chooser.open(v3d::ui::shell::FileChooser::Mode::Open, sandbox.path(), ".json",
        [&called](const boost::filesystem::path&) { called = true; });

    for (const std::string& name : { std::string(), std::string("../a.json"), std::string("art\\a.json"),
            std::string(".."), std::string("missing.json"), std::string("art") }) {
        chooser.name(name);
        BOOST_CHECK(!chooser.accept());
    }
    BOOST_CHECK(!called);
    BOOST_CHECK(chooser.visible());
}

/**
 * Saving gives a bare name the extension, and a new file is chosen at once.
 **/
BOOST_AUTO_TEST_CASE(a_saved_name_is_given_the_extension) {
    const Sandbox sandbox("extension");
    boost::filesystem::path chosen;
    v3d::ui::shell::FileChooser chooser(nullptr);
    chooser.open(v3d::ui::shell::FileChooser::Mode::Save, sandbox.path(), ".json",
        [&chosen](const boost::filesystem::path& path) { chosen = path; });

    chooser.name("scene");
    BOOST_CHECK(chooser.accept());
    BOOST_CHECK(chosen == sandbox.path() / "scene.json");
}

/**
 * Saving over a file that is there asks once: the first accept chooses nothing, and the same
 * name again replaces it. A different name asks again.
 **/
BOOST_AUTO_TEST_CASE(saving_over_a_file_asks_first) {
    const Sandbox sandbox("replace");
    int calls = 0;
    v3d::ui::shell::FileChooser chooser(nullptr);
    chooser.open(v3d::ui::shell::FileChooser::Mode::Save, sandbox.path(), ".json",
        [&calls](const boost::filesystem::path&) { calls++; });

    chooser.name("a");
    BOOST_CHECK(!chooser.accept());
    BOOST_CHECK(chooser.confirming());
    BOOST_CHECK_EQUAL(calls, 0);

    chooser.name("b.json");
    BOOST_CHECK(!chooser.accept());
    BOOST_CHECK_EQUAL(calls, 0);

    BOOST_CHECK(chooser.accept());
    BOOST_CHECK_EQUAL(calls, 1);
    BOOST_CHECK(!chooser.confirming());
}

/**
 * A directory opened from the list shows its own listing with no row chosen, so a pick that
 * follows does not open whatever now sits at the row chosen before.
 **/
BOOST_AUTO_TEST_CASE(a_new_listing_has_nothing_chosen) {
    const Sandbox sandbox("chosen");
    // enough in the directory opened that the row chosen before still names one of its rows
    sandbox.touch("art/c.json");
    sandbox.touch("art/d.json");
    const boost::shared_ptr<v3d::ui::Engine> ui = chooserUi();
    v3d::ui::shell::FileChooser chooser(ui);
    chooser.open(v3d::ui::shell::FileChooser::Mode::Open, sandbox.path(), ".json", {});

    const boost::shared_ptr<v3d::ui::component::SelectList> files =
        boost::dynamic_pointer_cast<v3d::ui::component::SelectList>(ui->container("chooser")->get("files"));
    BOOST_REQUIRE(files);
    // "art/", the first directory after the way up
    files->selected(1);
    chooser.pick();

    BOOST_CHECK(chooser.directory() == sandbox.path() / "art");
    BOOST_CHECK_EQUAL(files->selected(), v3d::ui::component::SelectList::none);
}

BOOST_AUTO_TEST_SUITE_END()
