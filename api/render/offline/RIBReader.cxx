/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "RIBReader.h"

#include <fstream>
#include <istream>
#include <string>
#include <vector>

#include <glm/gtc/type_ptr.hpp>

namespace v3d::render::offline {

namespace {

typedef RIBToken::Kind Kind;

std::string position(const RIBToken & token) {
    return " at line " + std::to_string(token.line()) + ", column " + std::to_string(token.column());
}

};  // namespace

RIBReader::RIBReader(const boost::shared_ptr<v3d::log::Logger> & logger) : logger_(logger) {
}

bool RIBReader::fail(const std::string & message, const RIBToken & token) {
    if (error_.empty()) {
        error_ = message + position(token);
    }
    return false;
}

bool RIBReader::number(RIBLexer * lexer, float * value) {
    const RIBToken token = lexer->next();
    if (token.kind() != Kind::NUMBER) {
        return fail("expected a number", token);
    }
    *value = token.value();
    return true;
}

bool RIBReader::text(RIBLexer * lexer, std::string * value) {
    const RIBToken token = lexer->next();
    if (token.kind() != Kind::STRING) {
        return fail("expected a quoted string", token);
    }
    *value = token.text();
    return true;
}

bool RIBReader::numbers(RIBLexer * lexer, unsigned int count, std::vector<float> * out) {
    const bool bracketed = lexer->peek().kind() == Kind::ARRAY_BEGIN;
    if (bracketed) {
        lexer->next();
    }
    for (unsigned int i = 0; i < count; i++) {
        float value = 0.0f;
        if (!number(lexer, &value)) {
            return false;
        }
        out->push_back(value);
    }
    if (bracketed) {
        const RIBToken token = lexer->next();
        if (token.kind() != Kind::ARRAY_END) {
            return fail("expected ']'", token);
        }
    }
    return true;
}

bool RIBReader::matrix(RIBLexer * lexer, glm::mat4x4 * out) {
    std::vector<float> values;
    if (!numbers(lexer, 16, &values)) {
        return false;
    }
    // reading the floats in the order RIB wrote them is the change of convention: RI
    // writes row major under a row vector convention and glm stores column major under a
    // column vector one, so a transpose here would undo it
    *out = glm::make_mat4(values.data());
    return true;
}

bool RIBReader::counts(RIBLexer * lexer, std::vector<unsigned int> * out) {
    const RIBToken open = lexer->next();
    if (open.kind() != Kind::ARRAY_BEGIN) {
        return fail("expected '['", open);
    }
    for (;;) {
        const RIBToken token = lexer->next();
        if (token.kind() == Kind::ARRAY_END) {
            return true;
        }
        if (token.kind() != Kind::NUMBER || token.value() < 0.0f) {
            return fail("expected a count", token);
        }
        out->push_back(static_cast<unsigned int>(token.value()));
    }
}

void RIBReader::skipArray(RIBLexer * lexer) {
    lexer->next();  // the opening bracket
    for (;;) {
        const RIBToken token = lexer->next();
        if (token.kind() == Kind::ARRAY_END || token.kind() == Kind::END) {
            return;
        }
    }
}

void RIBReader::skipArguments(RIBLexer * lexer) {
    for (;;) {
        const Kind kind = lexer->peek().kind();
        if (kind == Kind::END || kind == Kind::IDENTIFIER) {
            return;
        }
        lexer->next();
    }
}

bool RIBReader::values(RIBLexer * lexer, const RIBDeclaration & declaration, unsigned int vertices,
    const std::string & name, std::vector<float> * floats, std::vector<std::string> * strings) {
    if (lexer->peek().kind() == Kind::ARRAY_BEGIN) {
        lexer->next();
        for (;;) {
            const RIBToken token = lexer->next();
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

    // a bare value is not self-delimiting, which is the whole reason the declaration table
    // exists - without a type there is no knowing how much of what follows belonged to it
    const unsigned int elements = declaration.elements(vertices);
    if (elements == 0) {
        return fail("parameter '" + name + "' is varying or vertex and carries no array, so its length is unknowable",
            lexer->peek());
    }
    const unsigned int wanted = elements * declaration.count();
    if (declaration.type() == RIBDeclaration::Type::STRING) {
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

bool RIBReader::parameters(RIBLexer * lexer, unsigned int vertices, ParameterList * list) {
    while (lexer->peek().kind() == Kind::STRING) {
        const RIBToken token = lexer->next();
        std::string name;
        RIBDeclaration declaration;
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

        // a length that can be known is worth checking: an array disagreeing with its
        // declaration is a scene saying something other than what it means
        if (declaration.elements(vertices) != 0 && declaration.type() != RIBDeclaration::Type::STRING) {
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

bool RIBReader::request(const std::string & name, RIBLexer * lexer, RIBHandler * handler) {
    float a = 0.0f;
    float b = 0.0f;
    float c = 0.0f;
    float d = 0.0f;
    std::string first;
    std::string second;
    std::string third;
    ParameterList list;
    glm::mat4x4 m(1.0f);
    std::vector<float> triple;

    if (name == "version") {
        if (!number(lexer, &a)) {
            return false;
        }
        handler->version(a);
        return true;
    }
    if (name == "Declare") {
        if (!text(lexer, &first) || !text(lexer, &second)) {
            return false;
        }
        if (!declarations_.declare(first, second)) {
            logger_->get()->warn("RIB declaration of '{}' does not name a type", first);
        }
        handler->declare(first, second);
        return true;
    }
    if (name == "Option") {
        if (!text(lexer, &first) || !parameters(lexer, 1, &list)) {
            return false;
        }
        handler->option(first, list);
        return true;
    }
    if (name == "Format") {
        if (!number(lexer, &a) || !number(lexer, &b) || !number(lexer, &c)) {
            return false;
        }
        handler->format(static_cast<unsigned int>(a), static_cast<unsigned int>(b), c);
        return true;
    }
    if (name == "FrameAspectRatio") {
        if (!number(lexer, &a)) {
            return false;
        }
        handler->frameAspectRatio(a);
        return true;
    }
    if (name == "ScreenWindow") {
        if (!number(lexer, &a) || !number(lexer, &b) || !number(lexer, &c) || !number(lexer, &d)) {
            return false;
        }
        handler->screenWindow(a, b, c, d);
        return true;
    }
    if (name == "CropWindow") {
        if (!number(lexer, &a) || !number(lexer, &b) || !number(lexer, &c) || !number(lexer, &d)) {
            return false;
        }
        handler->cropWindow(a, b, c, d);
        return true;
    }
    if (name == "Projection") {
        if (!text(lexer, &first) || !parameters(lexer, 1, &list)) {
            return false;
        }
        handler->projection(first, list);
        return true;
    }
    if (name == "Clipping") {
        if (!number(lexer, &a) || !number(lexer, &b)) {
            return false;
        }
        handler->clipping(a, b);
        return true;
    }
    if (name == "Display") {
        if (!text(lexer, &first) || !text(lexer, &second) || !text(lexer, &third)) {
            return false;
        }
        if (!parameters(lexer, 1, &list)) {
            return false;
        }
        handler->display(first, second, third, list);
        return true;
    }
    if (name == "FrameBegin") {
        if (!number(lexer, &a)) {
            return false;
        }
        handler->frameBegin(static_cast<int>(a));
        return true;
    }
    if (name == "FrameEnd") {
        handler->frameEnd();
        return true;
    }
    if (name == "WorldBegin") {
        handler->worldBegin();
        return true;
    }
    if (name == "WorldEnd") {
        handler->worldEnd();
        return true;
    }
    if (name == "AttributeBegin") {
        handler->attributeBegin();
        return true;
    }
    if (name == "AttributeEnd") {
        handler->attributeEnd();
        return true;
    }
    if (name == "TransformBegin") {
        handler->transformBegin();
        return true;
    }
    if (name == "TransformEnd") {
        handler->transformEnd();
        return true;
    }
    if (name == "Identity") {
        handler->identity();
        return true;
    }
    if (name == "Transform") {
        if (!matrix(lexer, &m)) {
            return false;
        }
        handler->transform(m);
        return true;
    }
    if (name == "ConcatTransform") {
        if (!matrix(lexer, &m)) {
            return false;
        }
        handler->concatTransform(m);
        return true;
    }
    if (name == "Translate") {
        if (!numbers(lexer, 3, &triple)) {
            return false;
        }
        handler->translate(triple[0], triple[1], triple[2]);
        return true;
    }
    if (name == "Rotate") {
        if (!numbers(lexer, 4, &triple)) {
            return false;
        }
        handler->rotate(triple[0], triple[1], triple[2], triple[3]);
        return true;
    }
    if (name == "Scale") {
        if (!numbers(lexer, 3, &triple)) {
            return false;
        }
        handler->scale(triple[0], triple[1], triple[2]);
        return true;
    }
    if (name == "Color") {
        if (!numbers(lexer, 3, &triple)) {
            return false;
        }
        handler->color(glm::vec3(triple[0], triple[1], triple[2]));
        return true;
    }
    if (name == "Opacity") {
        if (!numbers(lexer, 3, &triple)) {
            return false;
        }
        handler->opacity(glm::vec3(triple[0], triple[1], triple[2]));
        return true;
    }
    if (name == "ShadingRate") {
        if (!number(lexer, &a)) {
            return false;
        }
        handler->shadingRate(a);
        return true;
    }
    if (name == "Attribute") {
        if (!text(lexer, &first) || !parameters(lexer, 1, &list)) {
            return false;
        }
        handler->attribute(first, list);
        return true;
    }
    if (name == "Surface") {
        if (!text(lexer, &first) || !parameters(lexer, 1, &list)) {
            return false;
        }
        handler->surface(first, list);
        return true;
    }
    if (name == "LightSource") {
        if (!text(lexer, &first)) {
            return false;
        }
        // the handle follows the shader name and is a sequence number in RIB 3.x
        if (lexer->peek().kind() == Kind::NUMBER) {
            lexer->next();
        }
        if (!parameters(lexer, 1, &list)) {
            return false;
        }
        handler->lightSource(first, list);
        return true;
    }
    if (name == "Polygon") {
        // RIB carries no vertex count: it is the length of the position array
        if (!parameters(lexer, 0, &list)) {
            return false;
        }
        unsigned int vertices = static_cast<unsigned int>(list.floats("P").size() / 3);
        if (vertices == 0) {
            vertices = static_cast<unsigned int>(list.floats("Pw").size() / 4);
        }
        if (vertices == 0) {
            vertices = static_cast<unsigned int>(list.floats("Pz").size());
        }
        handler->polygon(vertices, list);
        return true;
    }
    if (name == "PointsPolygons") {
        std::vector<unsigned int> perPolygon;
        std::vector<unsigned int> indices;
        if (!counts(lexer, &perPolygon) || !counts(lexer, &indices) || !parameters(lexer, 0, &list)) {
            return false;
        }
        handler->pointsPolygons(perPolygon, indices, list);
        return true;
    }
    if (name == "Sphere") {
        if (!number(lexer, &a) || !number(lexer, &b) || !number(lexer, &c) || !number(lexer, &d)) {
            return false;
        }
        if (!parameters(lexer, 1, &list)) {
            return false;
        }
        handler->sphere(a, b, c, d, list);
        return true;
    }

    if (reported_.insert("request " + name).second) {
        unrecognised_.push_back(name);
    }
    skipArguments(lexer);
    return true;
}

bool RIBReader::read(std::istream & stream, RIBHandler * handler) {
    error_.clear();
    unrecognised_.clear();
    reported_.clear();
    declarations_ = RIBDeclarations();

    RIBLexer lexer(stream);
    for (;;) {
        const RIBToken token = lexer.next();
        if (token.kind() == Kind::END) {
            break;
        }
        if (token.kind() != Kind::IDENTIFIER) {
            fail("expected a request", token);
            break;
        }
        if (!request(token.text(), &lexer, handler)) {
            break;
        }
    }

    if (error_.empty() && !lexer.error().empty()) {
        error_ = lexer.error();
    }

    for (const std::string & name : unrecognised_) {
        logger_->get()->warn("RIB request '{}' is not implemented and was skipped", name);
    }
    if (!error_.empty()) {
        logger_->get()->error("RIB parse failed - {}", error_);
        return false;
    }
    return true;
}

bool RIBReader::read(const std::string & path, RIBHandler * handler) {
    // binary rather than text: the lexer sniffs the opening bytes for a binary or gzipped
    // stream, and a carriage return is whitespace to it either way
    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file.is_open()) {
        error_ = "could not open '" + path + "'";
        logger_->get()->error("RIB read failed - {}", error_);
        return false;
    }
    return read(file, handler);
}

const std::string & RIBReader::error() const {
    return error_;
}

const std::vector<std::string> & RIBReader::unrecognised() const {
    return unrecognised_;
}

};  // namespace v3d::render::offline
