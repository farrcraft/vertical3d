/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Binder.h"

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <span>
#include <string>

#include "Statement.h"

namespace v3d::database {

/**
 **/
Binder::Binder(Statement* statement) :
    statement_(statement),
    bound_(statement != nullptr && statement->reset()) {
}

/**
 **/
Binder& Binder::integer(std::int64_t value) {
    bound_ = bound_ && statement_->bindInteger(parameter_, value);
    parameter_++;
    return *this;
}

/**
 **/
Binder& Binder::real(double value) {
    bound_ = bound_ && statement_->bindReal(parameter_, value);
    parameter_++;
    return *this;
}

/**
 **/
Binder& Binder::text(const std::string& value) {
    bound_ = bound_ && statement_->bindText(parameter_, value);
    parameter_++;
    return *this;
}

/**
 **/
Binder& Binder::blob(std::span<const std::byte> value) {
    bound_ = bound_ && statement_->bindBlob(parameter_, value);
    parameter_++;
    return *this;
}

/**
 **/
Binder& Binder::null() {
    bound_ = bound_ && statement_->bindNull(parameter_);
    parameter_++;
    return *this;
}

/**
 **/
Binder& Binder::integer(std::optional<std::int64_t> value) {
    return value ? integer(*value) : null();
}

/**
 **/
Binder& Binder::reals(std::initializer_list<double> values) {
    for (const double value : values) {
        real(value);
    }
    return *this;
}

/**
 **/
bool Binder::bound() const noexcept {
    return bound_;
}

/**
 **/
bool Binder::run() {
    return bound_ && statement_->step() == Statement::Step::Done;
}

};  // namespace v3d::database
