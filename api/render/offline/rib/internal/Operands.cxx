/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include "Operands.h"

#include <api/log/Logger.h>
#include <api/render/offline/rib/Bases.h>
#include <api/render/offline/rib/Declaration.h>
#include <api/render/offline/rib/Declarations.h>
#include <api/render/offline/rib/Lexer.h>
#include <api/render/offline/rib/Parameters.h>
#include <api/render/offline/rib/Token.h>
#include <api/render/offline/rib/requests/Basis.h>
#include <api/render/offline/rib/requests/Declare.h>
#include <api/type/Checked.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>

namespace v3d::render::offline::rib::internal {

Operands::Operands(const boost::shared_ptr<v3d::log::Logger> & logger) : logger_(logger) {
}

void Operands::reset() {
    declarations_ = Declarations();
    error_.clear();
    reported_.clear();
}

bool Operands::declare(const requests::Declare & request) {
    return declarations_.declare(request);
}

const std::string & Operands::error() const {
    return error_;
}

void Operands::stop(const std::string & message) {
    if (error_.empty()) {
        error_ = message;
    }
}

std::string Operands::position(const Token & token) {
    return " at line " + std::to_string(token.line()) + ", column " + std::to_string(token.column());
}

bool Operands::fail(const std::string & message, const Token & token) {
    if (error_.empty()) {
        error_ = message + position(token);
    }
    return false;
}

bool Operands::number(Lexer * lexer, float * value) {
    const Token token = lexer->next();
    if (token.kind() != Kind::NUMBER) {
        return fail("expected a number", token);
    }
    *value = token.value();
    return true;
}

bool Operands::text(Lexer * lexer, std::string * value) {
    const Token token = lexer->next();
    if (token.kind() != Kind::STRING) {
        return fail("expected a quoted string", token);
    }
    *value = token.text();
    return true;
}

bool Operands::numbers(Lexer * lexer, unsigned int count, std::vector<float> * out) {
    const bool bracketed = lexer->peek().kind() == Kind::ARRAY_BEGIN;
    if (bracketed) {
        lexer->next();
    }
    for (unsigned int i = 0; i < count; i++) {
        float value = 0.0F;
        if (!number(lexer, &value)) {
            return false;
        }
        out->push_back(value);
    }
    if (bracketed) {
        const Token token = lexer->next();
        if (token.kind() != Kind::ARRAY_END) {
            return fail("expected ']'", token);
        }
    }
    return true;
}

bool Operands::matrix(Lexer * lexer, glm::mat4x4 * out) {
    std::vector<float> values;
    if (!numbers(lexer, 16, &values)) {
        return false;
    }
    // read in RIB's order with no transpose, as ParameterList::matrix explains
    *out = glm::make_mat4(values.data());
    return true;
}

bool Operands::basis(Lexer * lexer, requests::Basis::Matrix * matrix, unsigned int * step) {
    const Token token = lexer->peek();
    if (token.kind() == Kind::STRING) {
        lexer->next();
        if (!namedBasis(token.text(), matrix)) {
            return fail("expected a basis RI names, or sixteen numbers", token);
        }
    } else {
        std::vector<float> values;
        if (!numbers(lexer, 16, &values)) {
            return false;
        }
        std::copy(values.begin(), values.end(), matrix->begin());
    }
    const Token stepped = lexer->peek();
    float value = 0.0F;
    if (!number(lexer, &value)) {
        return false;
    }
    const std::optional<uint32_t> count = v3d::type::toCount(value, 4);
    if (!count || *count == 0) {
        return fail("expected a step from 1 to 4", stepped);
    }
    *step = *count;
    return true;
}

bool Operands::counts(Lexer * lexer, std::vector<unsigned int> * out) {
    const Token open = lexer->next();
    if (open.kind() != Kind::ARRAY_BEGIN) {
        return fail("expected '['", open);
    }
    for (;;) {
        const Token token = lexer->next();
        if (token.kind() == Kind::ARRAY_END) {
            return true;
        }
        const std::optional<uint32_t> count = token.kind() == Kind::NUMBER ?
            v3d::type::toCount(token.value(), std::numeric_limits<uint32_t>::max()) : std::nullopt;
        if (!count) {
            return fail("expected a count", token);
        }
        out->push_back(*count);
    }
}

void Operands::skipArray(Lexer * lexer) {
    lexer->next();  // the opening bracket
    for (;;) {
        const Token token = lexer->next();
        if (token.kind() == Kind::ARRAY_END || token.kind() == Kind::END) {
            return;
        }
    }
}

bool Operands::values(Lexer * lexer, const Declaration & declaration, unsigned int vertices,
    const std::string & name, std::vector<float> * floats, std::vector<std::string> * strings) {
    if (lexer->peek().kind() == Kind::ARRAY_BEGIN) {
        lexer->next();
        for (;;) {
            const Token token = lexer->next();
            if (token.kind() == Kind::ARRAY_END) {
                return true;
            }
            if (token.kind() == Kind::NUMBER) {
                floats->push_back(token.value());
            } else if (token.kind() == Kind::STRING) {
                strings->push_back(token.text());
            } else {
                return fail("unterminated array for parameter '" + name + "'", token);
            }
        }
    }

    // a bare value is not self-delimiting: only the declaration's type says how much of
    // what follows belongs to it
    const unsigned int elements = declaration.elements(vertices);
    if (elements == 0) {
        return fail("parameter '" + name + "' is varying or vertex and carries no array, so its length is unknowable",
            lexer->peek());
    }
    const unsigned int wanted = elements * declaration.count();
    if (declaration.type() == Declaration::Type::STRING) {
        for (unsigned int i = 0; i < wanted; i++) {
            std::string value;
            if (!text(lexer, &value)) {
                return false;
            }
            strings->push_back(value);
        }
        return true;
    }
    return numbers(lexer, wanted * declaration.floats(), floats);
}

bool Operands::parameters(Lexer * lexer, unsigned int vertices, ParameterList * list) {
    while (lexer->peek().kind() == Kind::STRING) {
        const Token token = lexer->next();
        std::string name;
        Declaration declaration;
        if (!declarations_.resolve(token.text(), &name, &declaration)) {
            if (lexer->peek().kind() != Kind::ARRAY_BEGIN) {
                return fail("undeclared parameter '" + name + "' carries no array to bound it", token);
            }
            // a bracketed array is self-delimiting, so an undeclared name is recoverable
            skipArray(lexer);
            if (reported_.insert("parameter " + name).second) {
                logger_->get()->warn("RIB parameter '{}' was not declared and was skipped", name);
            }
            continue;
        }

        std::vector<float> floats;
        std::vector<std::string> strings;
        if (!values(lexer, declaration, vertices, name, &floats, &strings)) {
            return false;
        }

        // check the length when it is known: an array that disagrees with its declaration
        // is an error in the scene
        if (declaration.elements(vertices) != 0 && declaration.type() != Declaration::Type::STRING) {
            const std::size_t expected = static_cast<std::size_t>(declaration.elements(vertices)) *
                declaration.count() * declaration.floats();
            if (floats.size() != expected && reported_.insert("length " + name).second) {
                logger_->get()->warn("RIB parameter '{}' carries {} values where its declaration asks for {}",
                    name, floats.size(), expected);
            }
        }

        list->add(name, declaration, floats, strings);
    }
    return true;
}

bool Operands::handle(Lexer * lexer, std::string * value) {
    // a number or a string, and a string to the handler either way
    const Token token = lexer->peek();
    if (token.kind() == Kind::NUMBER) {
        lexer->next();
        const std::optional<int32_t> number = v3d::type::toInteger(token.value(), {.minimum = -2000000000, .maximum = 2000000000});
        if (!number) {
            return fail("expected a handle", token);
        }
        *value = std::to_string(*number);
        return true;
    }
    return text(lexer, value);
}

bool Operands::floatArray(Lexer * lexer, std::vector<float> * out) {
    const Token open = lexer->next();
    if (open.kind() != Kind::ARRAY_BEGIN) {
        return fail("expected '['", open);
    }
    for (;;) {
        const Token token = lexer->next();
        if (token.kind() == Kind::ARRAY_END) {
            return true;
        }
        if (token.kind() != Kind::NUMBER) {
            return fail("expected a number", token);
        }
        out->push_back(token.value());
    }
}

bool Operands::strings(Lexer * lexer, std::vector<std::string> * out) {
    const Token open = lexer->next();
    if (open.kind() != Kind::ARRAY_BEGIN) {
        return fail("expected '['", open);
    }
    for (;;) {
        const Token token = lexer->next();
        if (token.kind() == Kind::ARRAY_END) {
            return true;
        }
        if (token.kind() != Kind::STRING) {
            return fail("expected a quoted string", token);
        }
        out->push_back(token.text());
    }
}

};  // namespace v3d::render::offline::rib::internal
