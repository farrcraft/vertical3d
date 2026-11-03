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

/**
 * A declaration of one or more variables of one storage class and type, each of which may have an
 * initialiser.
 **/
class Declaration final : public Statement {
 public:
    Declaration(Storage storage, Type type, const v3d::render::offline::source::Position & at);

    Storage storage = Storage::UNSPECIFIED;
    Type type = Type::FLOAT;
    std::vector<Declarator> declarators;
};

};  // namespace v3d::render::offline::sl::syntax
