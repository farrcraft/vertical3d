/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

struct sqlite3;
struct sqlite3_stmt;

namespace v3d::database {

/**
 * One compiled SQL statement, made by Database::prepare().
 *
 * Parameters are numbered from 1, as SQLite numbers them, and the columns of a result row from
 * 0. A value bound is copied, so the caller's storage need not outlive the call. A text or blob
 * column read stays valid until the next step(), reset() or read of the same column.
 *
 * A statement must not outlive the connection that made it. It can be moved but not copied.
 **/
class Statement final {
 public:
    enum class Step : std::uint8_t {
        /** A row of the result is ready to read. **/
        Row,
        /** The statement has finished. **/
        Done,
        /** The statement failed, and error() says why. **/
        Failed
    };

    /**
     * An invalid statement, which every call refuses.
     **/
    Statement() noexcept;
    ~Statement();
    Statement(Statement&& other) noexcept;
    Statement& operator=(Statement&& other) noexcept;
    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;

    /**
     * @return whether the statement compiled
     **/
    bool valid() const noexcept;

    bool bindInteger(int index, std::int64_t value);
    bool bindReal(int index, double value);
    bool bindText(int index, const std::string& value);
    bool bindBlob(int index, std::span<const std::byte> value);
    bool bindNull(int index);

    Step step();

    /**
     * Make the statement ready to run again. The values bound stay bound.
     **/
    bool reset();

    /**
     * The value of one column of the current row, converted as SQLite converts it when the
     * column holds another type. A strict table holds only its declared types.
     **/
    std::int64_t integer(int column) const;
    double real(int column) const;
    std::string text(int column) const;
    std::span<const std::byte> blob(int column) const;
    bool isNull(int column) const;

    /**
     * @return why the last call that failed did so
     **/
    const std::string& error() const noexcept;

 private:
    friend class Database;

    Statement(sqlite3* database, sqlite3_stmt* handle) noexcept;

    /**
     * Keep the connection's message for a call that failed.
     **/
    bool fail(int code);

    sqlite3* database_;
    sqlite3_stmt* handle_;
    std::string error_;
};

};  // namespace v3d::database
