/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vector>

#include "Statement.h"

#include <boost/shared_ptr.hpp>

namespace v3d::render::offline::sl::syntax {

class Block final : public Statement {
 public:
    Block(unsigned int line, unsigned int column);

    std::vector<StatementPtr> statements;
};

typedef boost::shared_ptr<Block> BlockPtr;

};  // namespace v3d::render::offline::sl::syntax
