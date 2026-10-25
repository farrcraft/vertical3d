/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>
#include <string>

#include "Statement.h"

struct sqlite3;

namespace v3d::database {

/**
 * A connection to one SQLite database.
 *
 * Every connection enforces foreign keys, which SQLite leaves off unless a connection asks.
 * A failed call returns false or an invalid statement, and error() says why. The message
 * stays until the next call that fails.
 *
 * A path is UTF-8, as every narrow string in the tree is. The path ":memory:" opens a database
 * that lives only as long as the connection.
 *
 * A connection is not shared between threads.
 **/
class Database final {
 public:
    enum class Mode : std::uint8_t {
        /** Read an existing database, and refuse every write. **/
        ReadOnly,
        /** Read and write an existing database. **/
        ReadWrite,
        /** Read and write a database, making an empty one when there is none. **/
        Create
    };

    Database() noexcept;
    ~Database();
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    /**
     * Open a database, closing any this connection already has open.
     *
     * @return whether it opened. A path with no database fails in the two modes that need one
     **/
    bool open(const std::string& path, Mode mode);

    void close() noexcept;

    bool isOpen() const noexcept;

    /**
     * Run SQL that returns no rows. Several statements may be given, separated by semicolons,
     * and they stop at the first that fails.
     **/
    bool execute(const std::string& sql);

    /**
     * Compile one statement. Only the first statement of the text is compiled.
     *
     * @return an invalid statement when the text does not compile
     **/
    Statement prepare(const std::string& sql);

    /**
     * The two numbers in a database's header that say what it is. The application id names
     * the program whose format the database holds, and the user version is that format's
     * version. A new database has zero in both.
     **/
    bool applicationId(std::int32_t* id);
    bool applicationId(std::int32_t id);
    bool userVersion(std::int32_t* version);
    bool userVersion(std::int32_t version);

    /**
     * @return why the last call that failed did so
     **/
    const std::string& error() const noexcept;

 private:
    /**
     * Read one integer pragma.
     **/
    bool pragma(const char* name, std::int32_t* value);

    /**
     * Keep the connection's own message for a call that failed, behind what was being done.
     **/
    bool fail(const std::string& doing);

    sqlite3* handle_;
    std::string error_;
};

};  // namespace v3d::database
