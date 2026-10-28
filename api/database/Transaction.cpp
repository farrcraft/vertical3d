/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Transaction.h"

#include "Database.h"

namespace v3d::database {

/**
 **/
Transaction::Transaction(Database* database) :
    database_(database),
    active_(false) {
    // immediate, so the write lock is taken here and not at the first write
    active_ = database_ != nullptr && database_->execute("BEGIN IMMEDIATE");
}

/**
 **/
Transaction::~Transaction() {
    if (active_) {
        database_->execute("ROLLBACK");
    }
}

/**
 **/
bool Transaction::active() const noexcept {
    return active_;
}

/**
 **/
bool Transaction::commit() {
    if (!active_) {
        return false;
    }
    if (database_->execute("COMMIT")) {
        active_ = false;
        return true;
    }
    // a rollback that succeeds leaves the database's error the commit's
    database_->execute("ROLLBACK");
    active_ = false;
    return false;
}

};  // namespace v3d::database
