/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/database/Binder.h>
#include <api/database/Columns.h>
#include <api/database/Database.h>
#include <api/database/Statement.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>

#include <boost/test/unit_test.hpp>

namespace {

/**
 * A connection to a new database in memory, with one strict table of every type.
 **/
struct Rows {
    Rows() {
        BOOST_REQUIRE(database.open(":memory:", v3d::database::Database::Mode::Create));
        BOOST_REQUIRE(database.execute(
            "CREATE TABLE row (whole INTEGER, number REAL, other REAL, words TEXT, bytes BLOB, maybe INTEGER) STRICT"));
    }

    v3d::database::Database database;
};

BOOST_FIXTURE_TEST_CASE(binder_in_order_test, Rows) {
    const std::array<std::byte, 2> bytes = { std::byte{ 0x01 }, std::byte{ 0xFE } };
    v3d::database::Statement insert = database.prepare("INSERT INTO row VALUES (?1, ?2, ?3, ?4, ?5, ?6)");

    // each call binds the next parameter, and two reals take two
    BOOST_CHECK(v3d::database::Binder(&insert).integer(INT64_C(7)).reals({ 0.5, -2.0 }).text("seven").blob(bytes)
        .integer(std::optional<std::int64_t>()).run());

    // a binder resets the statement, so the same one binds and runs a second row
    BOOST_CHECK(v3d::database::Binder(&insert).integer(INT64_C(8)).real(1.5).null().text("eight").blob(bytes)
        .integer(std::optional<std::int64_t>(3)).run());

    v3d::database::Statement select = database.prepare("SELECT * FROM row ORDER BY whole");
    BOOST_REQUIRE(select.step() == v3d::database::Statement::Step::Row);
    v3d::database::Columns first(select);
    BOOST_CHECK_EQUAL(first.integer(), INT64_C(7));
    BOOST_CHECK_EQUAL(first.real(), 0.5);
    BOOST_CHECK_EQUAL(first.real(), -2.0);
    BOOST_CHECK_EQUAL(first.text(), "seven");
    const std::span<const std::byte> read = first.blob();
    BOOST_CHECK(std::equal(read.begin(), read.end(), bytes.begin(), bytes.end()));
    BOOST_CHECK(select.isNull(5));

    BOOST_REQUIRE(select.step() == v3d::database::Statement::Step::Row);
    BOOST_CHECK(select.isNull(2));
    BOOST_CHECK_EQUAL(select.integer(5), INT64_C(3));
}

BOOST_FIXTURE_TEST_CASE(binder_failed_bind_test, Rows) {
    v3d::database::Statement insert = database.prepare("INSERT INTO row (whole) VALUES (?1)");

    // a bind past the last parameter fails, and the binder then refuses to run the statement
    v3d::database::Binder binder(&insert);
    binder.integer(INT64_C(1)).integer(INT64_C(2));
    BOOST_CHECK(!binder.bound());
    BOOST_CHECK(!binder.run());
    v3d::database::Statement count = database.prepare("SELECT count(*) FROM row");
    BOOST_REQUIRE(count.step() == v3d::database::Statement::Step::Row);
    BOOST_CHECK_EQUAL(count.integer(0), INT64_C(0));

    // and a binder for no statement binds nothing
    BOOST_CHECK(!v3d::database::Binder(nullptr).integer(INT64_C(1)).run());
}

};  // namespace
