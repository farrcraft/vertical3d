/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

#include "Statement.h"

namespace v3d::database {

/**
 * Reads the columns of a statement's current row in order, from the first.
 *
 * Each call reads the next column, converted as Statement converts it. The reader holds the
 * statement, so it is made for one row and must not outlive the statement.
 **/
class Columns final {
 public:
    explicit Columns(const Statement& row);

    std::int64_t integer();
    double real();
    std::string text();

    /**
     * @return the column's bytes, which stay valid until the statement steps, resets or reads
     *         the same column again
     **/
    std::span<const std::byte> blob();

 private:
    const Statement& row_;
    int column_ = 0;
};

};  // namespace v3d::database
