/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Database.h"

namespace v3d::database {

/**
 * A write transaction that rolls back unless it is committed.
 *
 * It takes the database's write lock when it begins, so a second writer waits or fails at the
 * start rather than part way through. Everything done on the database between the two ends is
 * written whole by commit(), or not at all.
 **/
class Transaction final {
 public:
    /**
     * Begin a transaction. active() says whether it began.
     **/
    explicit Transaction(Database* database);

    /**
     * Roll back a transaction that was not committed.
     **/
    ~Transaction();

    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;

    /**
     * @return whether the transaction began and has not yet ended
     **/
    bool active() const noexcept;

    /**
     * @return whether everything since the beginning was written. A commit that fails rolls
     *         back, and the database's error() says why
     **/
    bool commit();

 private:
    Database* database_;
    bool active_;
};

};  // namespace v3d::database
