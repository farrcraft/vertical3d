/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Database.h"

#include <sqlite3.h>

#include <cstdint>
#include <string>

#include "Statement.h"

namespace v3d::database {

/**
 **/
Database::Database() noexcept :
    handle_(nullptr) {
}

/**
 **/
Database::~Database() {
    close();
}

/**
 **/
bool Database::open(const std::string& path, Mode mode) {
    close();
    int flags = SQLITE_OPEN_READONLY;
    switch (mode) {
        case Mode::ReadOnly:
            flags = SQLITE_OPEN_READONLY;
            break;
        case Mode::ReadWrite:
            flags = SQLITE_OPEN_READWRITE;
            break;
        case Mode::Create:
            flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
            break;
    }
    // a failed open still makes a handle, which carries the reason and has to be closed
    const int code = sqlite3_open_v2(path.c_str(), &handle_, flags | SQLITE_OPEN_EXRESCODE, nullptr);
    if (code != SQLITE_OK) {
        error_ = "cannot open " + path + ": " +
            (handle_ != nullptr ? std::string(sqlite3_errmsg(handle_)) : std::string(sqlite3_errstr(code)));
        close();
        return false;
    }
    if (!execute("PRAGMA foreign_keys = ON")) {
        close();
        return false;
    }
    return true;
}

/**
 **/
void Database::close() noexcept {
    if (handle_ != nullptr) {
        // every statement has to be finalized before this, which Statement's destructor does.
        // A statement left alive keeps the connection open until it goes
        sqlite3_close_v2(handle_);
        handle_ = nullptr;
    }
}

/**
 **/
bool Database::isOpen() const noexcept {
    return handle_ != nullptr;
}

/**
 **/
bool Database::execute(const std::string& sql) {
    if (handle_ == nullptr) {
        error_ = "the database is not open";
        return false;
    }
    if (sqlite3_exec(handle_, sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
        return fail("cannot run SQL");
    }
    return true;
}

/**
 **/
Statement Database::prepare(const std::string& sql) {
    if (handle_ == nullptr) {
        error_ = "the database is not open";
        return Statement();
    }
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(handle_, sql.c_str(), static_cast<int>(sql.size()), &statement, nullptr) != SQLITE_OK) {
        fail("cannot compile SQL");
        return Statement();
    }
    // text with no statement in it, such as a comment, compiles to nothing
    if (statement == nullptr) {
        error_ = "there is no statement in the SQL";
        return Statement();
    }
    return Statement(handle_, statement);
}

/**
 **/
bool Database::applicationId(std::int32_t* id) {
    return pragma("application_id", id);
}

/**
 **/
bool Database::applicationId(std::int32_t id) {
    return execute("PRAGMA application_id = " + std::to_string(id));
}

/**
 **/
bool Database::userVersion(std::int32_t* version) {
    return pragma("user_version", version);
}

/**
 **/
bool Database::userVersion(std::int32_t version) {
    return execute("PRAGMA user_version = " + std::to_string(version));
}

/**
 **/
const std::string& Database::error() const noexcept {
    return error_;
}

/**
 **/
bool Database::pragma(const char* name, std::int32_t* value) {
    Statement statement = prepare(std::string("PRAGMA ") + name);
    if (!statement.valid()) {
        return false;
    }
    if (statement.step() != Statement::Step::Row) {
        error_ = statement.error();
        return false;
    }
    *value = static_cast<std::int32_t>(statement.integer(0));
    return true;
}

/**
 **/
bool Database::fail(const std::string& doing) {
    error_ = doing + ": " + sqlite3_errmsg(handle_);
    return false;
}

};  // namespace v3d::database
