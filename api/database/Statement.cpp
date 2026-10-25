/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Statement.h"

#include <sqlite3.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <utility>

namespace v3d::database {

/**
 **/
Statement::Statement() noexcept :
    database_(nullptr),
    handle_(nullptr) {
}

/**
 **/
Statement::Statement(sqlite3* database, sqlite3_stmt* handle) noexcept :
    database_(database),
    handle_(handle) {
}

/**
 **/
Statement::~Statement() {
    sqlite3_finalize(handle_);
}

/**
 **/
Statement::Statement(Statement&& other) noexcept :
    database_(std::exchange(other.database_, nullptr)),
    handle_(std::exchange(other.handle_, nullptr)),
    error_(std::move(other.error_)) {
}

/**
 **/
Statement& Statement::operator=(Statement&& other) noexcept {
    if (this != &other) {
        sqlite3_finalize(handle_);
        database_ = std::exchange(other.database_, nullptr);
        handle_ = std::exchange(other.handle_, nullptr);
        error_ = std::move(other.error_);
    }
    return *this;
}

/**
 **/
bool Statement::valid() const noexcept {
    return handle_ != nullptr;
}

/**
 **/
bool Statement::bindInteger(int index, std::int64_t value) {
    if (handle_ == nullptr) {
        return false;
    }
    return fail(sqlite3_bind_int64(handle_, index, value));
}

/**
 **/
bool Statement::bindReal(int index, double value) {
    if (handle_ == nullptr) {
        return false;
    }
    return fail(sqlite3_bind_double(handle_, index, value));
}

/**
 **/
bool Statement::bindText(int index, const std::string& value) {
    if (handle_ == nullptr) {
        return false;
    }
    return fail(sqlite3_bind_text64(handle_, index, value.data(), value.size(), SQLITE_TRANSIENT, SQLITE_UTF8));
}

/**
 **/
bool Statement::bindBlob(int index, std::span<const std::byte> value) {
    if (handle_ == nullptr) {
        return false;
    }
    // an empty blob is still a blob rather than a null, and a null pointer would bind a null
    static const std::byte nothing{};
    const void* data = value.empty() ? &nothing : value.data();
    return fail(sqlite3_bind_blob64(handle_, index, data, value.size(), SQLITE_TRANSIENT));
}

/**
 **/
bool Statement::bindNull(int index) {
    if (handle_ == nullptr) {
        return false;
    }
    return fail(sqlite3_bind_null(handle_, index));
}

/**
 **/
Statement::Step Statement::step() {
    if (handle_ == nullptr) {
        error_ = "the statement did not compile";
        return Step::Failed;
    }
    const int code = sqlite3_step(handle_);
    if (code == SQLITE_ROW) {
        return Step::Row;
    }
    if (code == SQLITE_DONE) {
        return Step::Done;
    }
    fail(code);
    return Step::Failed;
}

/**
 **/
bool Statement::reset() {
    if (handle_ == nullptr) {
        return false;
    }
    return fail(sqlite3_reset(handle_));
}

/**
 **/
std::int64_t Statement::integer(int column) const {
    return handle_ != nullptr ? sqlite3_column_int64(handle_, column) : 0;
}

/**
 **/
double Statement::real(int column) const {
    return handle_ != nullptr ? sqlite3_column_double(handle_, column) : 0.0;
}

/**
 **/
std::string Statement::text(int column) const {
    if (handle_ == nullptr) {
        return std::string();
    }
    // the text has to be fetched before its size, which the fetch may change
    const unsigned char* data = sqlite3_column_text(handle_, column);
    const int size = sqlite3_column_bytes(handle_, column);
    if (data == nullptr || size <= 0) {
        return std::string();
    }
    return std::string(reinterpret_cast<const char*>(data), static_cast<std::size_t>(size));
}

/**
 **/
std::span<const std::byte> Statement::blob(int column) const {
    if (handle_ == nullptr) {
        return {};
    }
    const void* data = sqlite3_column_blob(handle_, column);
    const int size = sqlite3_column_bytes(handle_, column);
    if (data == nullptr || size <= 0) {
        return {};
    }
    return {static_cast<const std::byte*>(data), static_cast<std::size_t>(size)};
}

/**
 **/
bool Statement::isNull(int column) const {
    return handle_ == nullptr || sqlite3_column_type(handle_, column) == SQLITE_NULL;
}

/**
 **/
const std::string& Statement::error() const noexcept {
    return error_;
}

/**
 **/
bool Statement::fail(int code) {
    if (code == SQLITE_OK) {
        return true;
    }
    error_ = database_ != nullptr ? std::string(sqlite3_errmsg(database_)) : std::string(sqlite3_errstr(code));
    return false;
}

};  // namespace v3d::database
