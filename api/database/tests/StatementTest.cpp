/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/database/Database.h>
#include <api/database/Statement.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <utility>

#include <boost/test/unit_test.hpp>

namespace {

/**
 * A connection to a new database in memory, with one strict table of every type.
 **/
struct Values {
    Values() {
        BOOST_REQUIRE(database.open(":memory:", v3d::database::Database::Mode::Create));
        BOOST_REQUIRE(database.execute(
            "CREATE TABLE value (whole INTEGER, number REAL, words TEXT, bytes BLOB) STRICT"));
    }

    v3d::database::Database database;
};

BOOST_FIXTURE_TEST_CASE(statement_round_trip_test, Values) {
    const std::array<std::byte, 3> bytes = { std::byte{ 0x00 }, std::byte{ 0xFF }, std::byte{ 0x7F } };

    v3d::database::Statement insert = database.prepare("INSERT INTO value VALUES (?1, ?2, ?3, ?4)");
    BOOST_REQUIRE(insert.valid());
    BOOST_CHECK(insert.bindInteger(1, INT64_C(-9007199254740993)));
    BOOST_CHECK(insert.bindReal(2, 0.1));
    BOOST_CHECK(insert.bindText(3, std::string("a\0b", 3)));
    BOOST_CHECK(insert.bindBlob(4, bytes));
    BOOST_CHECK(insert.step() == v3d::database::Statement::Step::Done);

    // every value comes back exactly, a zero inside a text and a zero byte inside a blob too
    v3d::database::Statement select = database.prepare("SELECT whole, number, words, bytes FROM value");
    BOOST_REQUIRE(select.step() == v3d::database::Statement::Step::Row);
    BOOST_CHECK_EQUAL(select.integer(0), INT64_C(-9007199254740993));
    BOOST_CHECK_EQUAL(select.real(1), 0.1);
    BOOST_CHECK(select.text(2) == std::string("a\0b", 3));
    const std::span<const std::byte> read = select.blob(3);
    BOOST_REQUIRE_EQUAL(read.size(), bytes.size());
    BOOST_CHECK(std::equal(read.begin(), read.end(), bytes.begin()));
    BOOST_CHECK(select.step() == v3d::database::Statement::Step::Done);
}

BOOST_FIXTURE_TEST_CASE(statement_null_and_empty_test, Values) {
    v3d::database::Statement insert = database.prepare("INSERT INTO value (words, bytes) VALUES (?1, ?2)");
    BOOST_CHECK(insert.bindNull(1));
    // an empty blob is a value, not a missing one
    BOOST_CHECK(insert.bindBlob(2, std::span<const std::byte>()));
    BOOST_REQUIRE(insert.step() == v3d::database::Statement::Step::Done);

    v3d::database::Statement select = database.prepare("SELECT words, bytes, typeof(bytes) FROM value");
    BOOST_REQUIRE(select.step() == v3d::database::Statement::Step::Row);
    BOOST_CHECK(select.isNull(0));
    BOOST_CHECK(select.text(0).empty());
    BOOST_CHECK(!select.isNull(1));
    BOOST_CHECK(select.blob(1).empty());
    BOOST_CHECK_EQUAL(select.text(2), "blob");
}

BOOST_FIXTURE_TEST_CASE(statement_reset_test, Values) {
    v3d::database::Statement insert = database.prepare("INSERT INTO value (whole) VALUES (?1)");
    for (std::int64_t whole = 1; whole <= 3; whole++) {
        BOOST_CHECK(insert.bindInteger(1, whole));
        BOOST_CHECK(insert.step() == v3d::database::Statement::Step::Done);
        BOOST_CHECK(insert.reset());
    }

    // a reset keeps the values bound, so running again repeats the last insert
    BOOST_CHECK(insert.step() == v3d::database::Statement::Step::Done);

    v3d::database::Statement sum = database.prepare("SELECT sum(whole), count(*) FROM value");
    BOOST_REQUIRE(sum.step() == v3d::database::Statement::Step::Row);
    BOOST_CHECK_EQUAL(sum.integer(0), 9);
    BOOST_CHECK_EQUAL(sum.integer(1), 4);
}

BOOST_FIXTURE_TEST_CASE(statement_failure_test, Values) {
    // a strict table refuses a value of the wrong type, at the step
    v3d::database::Statement insert = database.prepare("INSERT INTO value (whole) VALUES (?1)");
    BOOST_CHECK(insert.bindText(1, "seven"));
    BOOST_CHECK(insert.step() == v3d::database::Statement::Step::Failed);
    BOOST_CHECK(!insert.error().empty());

    // a parameter that is not in the statement is refused at the bind
    BOOST_CHECK(!insert.bindInteger(2, 1));

    // an invalid statement refuses everything and reads as empty
    v3d::database::Statement invalid;
    BOOST_CHECK(!invalid.valid());
    BOOST_CHECK(!invalid.bindInteger(1, 1));
    BOOST_CHECK(invalid.step() == v3d::database::Statement::Step::Failed);
    BOOST_CHECK(!invalid.reset());
    BOOST_CHECK_EQUAL(invalid.integer(0), 0);
    BOOST_CHECK(invalid.isNull(0));
    BOOST_CHECK(invalid.blob(0).empty());
}

BOOST_FIXTURE_TEST_CASE(statement_move_test, Values) {
    v3d::database::Statement first = database.prepare("SELECT 42");
    v3d::database::Statement second(std::move(first));
    BOOST_CHECK(!first.valid());  // NOLINT(bugprone-use-after-move) - a moved-from statement is defined to be invalid
    BOOST_REQUIRE(second.valid());

    v3d::database::Statement third;
    third = std::move(second);
    BOOST_REQUIRE(third.step() == v3d::database::Statement::Step::Row);
    BOOST_CHECK_EQUAL(third.integer(0), 42);
}

};  // namespace
