/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/database/Database.h>
#include <api/database/Statement.h>

#include <cstdint>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>

namespace {

/**
 * A database file in the temporary directory, removed before and after the test.
 **/
class Scratch final {
 public:
    explicit Scratch(const std::string& name) :
        path_(boost::filesystem::temp_directory_path() / ("v3d_database_" + name)) {
        boost::filesystem::remove(path_);
    }

    ~Scratch() {
        boost::system::error_code error;
        boost::filesystem::remove(path_, error);
    }

    Scratch(const Scratch&) = delete;
    Scratch& operator=(const Scratch&) = delete;

    std::string path() const {
        return path_.string();
    }

 private:
    boost::filesystem::path path_;
};

BOOST_AUTO_TEST_CASE(database_open_test) {
    const Scratch scratch("open.db");
    v3d::database::Database database;
    BOOST_CHECK(!database.isOpen());

    // the two modes that need a database refuse a path with none, and say why
    BOOST_CHECK(!database.open(scratch.path(), v3d::database::Database::Mode::ReadOnly));
    BOOST_CHECK(!database.error().empty());
    BOOST_CHECK(!database.open(scratch.path(), v3d::database::Database::Mode::ReadWrite));
    BOOST_CHECK(!boost::filesystem::exists(scratch.path()));

    BOOST_REQUIRE(database.open(scratch.path(), v3d::database::Database::Mode::Create));
    BOOST_CHECK(database.isOpen());
    BOOST_CHECK(database.execute("CREATE TABLE things (id INTEGER PRIMARY KEY)"));
    database.close();
    BOOST_CHECK(!database.isOpen());

    // what was written is there to read
    BOOST_REQUIRE(database.open(scratch.path(), v3d::database::Database::Mode::ReadOnly));
    v3d::database::Statement count = database.prepare("SELECT count(*) FROM things");
    BOOST_REQUIRE(count.valid());
    BOOST_CHECK(count.step() == v3d::database::Statement::Step::Row);
    BOOST_CHECK_EQUAL(count.integer(0), 0);

    // and a read only connection refuses a write
    BOOST_CHECK(!database.execute("INSERT INTO things (id) VALUES (1)"));
    BOOST_CHECK(!database.error().empty());
}

BOOST_AUTO_TEST_CASE(database_header_test) {
    const Scratch scratch("header.db");
    {
        v3d::database::Database database;
        BOOST_REQUIRE(database.open(scratch.path(), v3d::database::Database::Mode::Create));

        // a new database says nothing about what it holds
        std::int32_t id = -1;
        std::int32_t version = -1;
        BOOST_REQUIRE(database.applicationId(&id));
        BOOST_REQUIRE(database.userVersion(&version));
        BOOST_CHECK_EQUAL(id, 0);
        BOOST_CHECK_EQUAL(version, 0);

        BOOST_CHECK(database.applicationId(0x56334453));
        BOOST_CHECK(database.userVersion(7));
    }

    // both are in the file, so they outlive the connection that set them
    v3d::database::Database database;
    BOOST_REQUIRE(database.open(scratch.path(), v3d::database::Database::Mode::ReadOnly));
    std::int32_t id = 0;
    std::int32_t version = 0;
    BOOST_REQUIRE(database.applicationId(&id));
    BOOST_REQUIRE(database.userVersion(&version));
    BOOST_CHECK_EQUAL(id, 0x56334453);
    BOOST_CHECK_EQUAL(version, 7);
}

BOOST_AUTO_TEST_CASE(database_failure_test) {
    v3d::database::Database closed;
    BOOST_CHECK(!closed.execute("SELECT 1"));
    BOOST_CHECK(!closed.prepare("SELECT 1").valid());
    BOOST_CHECK(!closed.error().empty());

    v3d::database::Database database;
    BOOST_REQUIRE(database.open(":memory:", v3d::database::Database::Mode::Create));

    // SQL that does not compile is refused with the reason
    BOOST_CHECK(!database.execute("SELEKT 1"));
    BOOST_CHECK(database.error().find("SELEKT") != std::string::npos);
    BOOST_CHECK(!database.prepare("SELECT * FROM nothing").valid());
    BOOST_CHECK(database.error().find("nothing") != std::string::npos);

    // text with no statement in it compiles to nothing, which is not a statement
    BOOST_CHECK(!database.prepare("-- only a comment").valid());
}

BOOST_AUTO_TEST_CASE(database_foreign_keys_test) {
    v3d::database::Database database;
    BOOST_REQUIRE(database.open(":memory:", v3d::database::Database::Mode::Create));
    BOOST_REQUIRE(database.execute(
        "CREATE TABLE parents (id INTEGER PRIMARY KEY);"
        "CREATE TABLE children (parent INTEGER NOT NULL REFERENCES parents(id) ON DELETE CASCADE);"
        "INSERT INTO parents (id) VALUES (1);"
        "INSERT INTO children (parent) VALUES (1);"));

    // every connection enforces foreign keys, which SQLite otherwise leaves off
    BOOST_CHECK(!database.execute("INSERT INTO children (parent) VALUES (2)"));

    BOOST_REQUIRE(database.execute("DELETE FROM parents WHERE id = 1"));
    v3d::database::Statement count = database.prepare("SELECT count(*) FROM children");
    BOOST_REQUIRE(count.step() == v3d::database::Statement::Step::Row);
    BOOST_CHECK_EQUAL(count.integer(0), 0);
}

};  // namespace
