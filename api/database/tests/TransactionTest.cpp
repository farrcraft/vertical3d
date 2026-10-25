/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/database/Database.h>
#include <api/database/Statement.h>
#include <api/database/Transaction.h>

#include <cstdint>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>

namespace {

/**
 * @return how many rows the table of the test holds
 **/
std::int64_t rows(v3d::database::Database* database) {
    v3d::database::Statement count = database->prepare("SELECT count(*) FROM things");
    BOOST_REQUIRE(count.step() == v3d::database::Statement::Step::Row);
    return count.integer(0);
}

/**
 * A connection to a new database in memory, with one table.
 **/
struct Things {
    Things() {
        BOOST_REQUIRE(database.open(":memory:", v3d::database::Database::Mode::Create));
        BOOST_REQUIRE(database.execute("CREATE TABLE things (id INTEGER PRIMARY KEY)"));
    }

    v3d::database::Database database;
};

BOOST_FIXTURE_TEST_CASE(transaction_commit_test, Things) {
    {
        v3d::database::Transaction transaction(&database);
        BOOST_REQUIRE(transaction.active());
        BOOST_REQUIRE(database.execute("INSERT INTO things (id) VALUES (1)"));
        BOOST_CHECK(transaction.commit());
        BOOST_CHECK(!transaction.active());

        // a transaction ends once
        BOOST_CHECK(!transaction.commit());
    }
    BOOST_CHECK_EQUAL(rows(&database), 1);
}

BOOST_FIXTURE_TEST_CASE(transaction_rollback_test, Things) {
    {
        const v3d::database::Transaction transaction(&database);
        BOOST_REQUIRE(transaction.active());
        BOOST_REQUIRE(database.execute("INSERT INTO things (id) VALUES (1)"));
        BOOST_CHECK_EQUAL(rows(&database), 1);
    }

    // a transaction that was not committed leaves nothing behind
    BOOST_CHECK_EQUAL(rows(&database), 0);

    // and one that could not begin does nothing at its end
    v3d::database::Database closed;
    const v3d::database::Transaction never(&closed);
    BOOST_CHECK(!never.active());
    const v3d::database::Transaction nothing(nullptr);
    BOOST_CHECK(!nothing.active());
}

BOOST_FIXTURE_TEST_CASE(transaction_failed_commit_test, Things) {
    BOOST_REQUIRE(database.execute(
        "CREATE TABLE children (parent INTEGER REFERENCES things(id) DEFERRABLE INITIALLY DEFERRED)"));
    {
        // a deferred constraint is checked at the commit, which then fails and rolls back
        v3d::database::Transaction transaction(&database);
        BOOST_REQUIRE(database.execute("INSERT INTO things (id) VALUES (1)"));
        BOOST_REQUIRE(database.execute("INSERT INTO children (parent) VALUES (2)"));
        BOOST_CHECK(!transaction.commit());
        BOOST_CHECK(!transaction.active());
        BOOST_CHECK(database.error().find("FOREIGN KEY") != std::string::npos);
    }
    BOOST_CHECK_EQUAL(rows(&database), 0);
}

BOOST_AUTO_TEST_CASE(transaction_one_writer_test) {
    const boost::filesystem::path path = boost::filesystem::temp_directory_path() / "v3d_database_writer.db";
    boost::filesystem::remove(path);
    {
        v3d::database::Database first;
        v3d::database::Database second;
        BOOST_REQUIRE(first.open(path.string(), v3d::database::Database::Mode::Create));
        BOOST_REQUIRE(second.open(path.string(), v3d::database::Database::Mode::ReadWrite));

        // the write lock is taken as the transaction begins, so a second writer is refused at
        // the start rather than part way through its writes
        const v3d::database::Transaction holding(&first);
        BOOST_REQUIRE(holding.active());
        const v3d::database::Transaction waiting(&second);
        BOOST_CHECK(!waiting.active());
        BOOST_CHECK(!second.error().empty());
    }
    boost::filesystem::remove(path);
}

};  // namespace
