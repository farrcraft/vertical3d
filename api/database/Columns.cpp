/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Columns.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

#include "Statement.h"

namespace v3d::database {

/**
 **/
Columns::Columns(const Statement& row) :
    row_(row) {
}

/**
 **/
std::int64_t Columns::integer() {
    return row_.integer(column_++);
}

/**
 **/
double Columns::real() {
    return row_.real(column_++);
}

/**
 **/
std::string Columns::text() {
    return row_.text(column_++);
}

/**
 **/
std::span<const std::byte> Columns::blob() {
    return row_.blob(column_++);
}

};  // namespace v3d::database
