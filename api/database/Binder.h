/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <span>
#include <string>

#include "Statement.h"

namespace v3d::database {

/**
 * Binds a statement's parameters in order, from the first, and runs it.
 *
 * It resets the statement when it is made, so one statement can be bound and run again for each
 * row. Each call binds the next parameter. A bind that fails is remembered, the binds after it
 * are skipped, and run() then refuses to run the statement.
 **/
class Binder final {
 public:
    explicit Binder(Statement* statement);

    Binder& integer(std::int64_t value);
    Binder& real(double value);
    Binder& text(const std::string& value);
    Binder& blob(std::span<const std::byte> value);
    Binder& null();

    /**
     * Bind an integer, or a null when there is none.
     **/
    Binder& integer(std::optional<std::int64_t> value);

    /**
     * Bind several reals, one parameter each, in the order given.
     **/
    Binder& reals(std::initializer_list<double> values);

    /**
     * @return whether every bind so far succeeded
     **/
    bool bound() const noexcept;

    /**
     * @return whether every bind succeeded and the statement then ran to its end. A statement
     *         that returns a row is not run to its end, so this is for one that returns none
     **/
    bool run();

 private:
    Statement* statement_;
    int parameter_ = 1;
    bool bound_;
};

};  // namespace v3d::database
