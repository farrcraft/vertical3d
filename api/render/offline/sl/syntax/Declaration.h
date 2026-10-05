/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/sl/Types.h>

#include <vector>

#include "Declarator.h"
#include "Statement.h"

namespace v3d::render::offline::sl::syntax {

class Declaration final : public Statement {
 public:
    Declaration(Storage storage, Type type, unsigned int line, unsigned int column);

    Storage storage = Storage::UNSPECIFIED;
    Type type = Type::FLOAT;
    std::vector<Declarator> declarators;
};

};  // namespace v3d::render::offline::sl::syntax
