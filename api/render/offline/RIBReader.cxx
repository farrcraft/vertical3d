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

/**
 * Reads the RI options: what the picture is and what the scene calls things.
 **/
RIBReader::Result RIBReader::optionRequest(const std::string & name, RIBLexer * lexer, RIBHandler * handler) {
    float a = 0.0f;
    float b = 0.0f;
    float c = 0.0f;
    std::string first;
    std::string second;
    ParameterList list;

    if (name == "version") {
        if (!number(lexer, &a)) {
            return Result::Failed;
        }
        handler->version(a);
        return Result::Handled;
    }
    if (name == "Declare") {
        if (!text(lexer, &first) || !text(lexer, &second)) {
            return Result::Failed;
        }
        if (!declarations_.declare(first, second)) {
            logger_->get()->warn("RIB declaration of '{}' does not name a type", first);
        }
        handler->declare(first, second);
        return Result::Handled;
    }
    if (name == "Option") {
        if (!text(lexer, &first) || !parameters(lexer, 1, &list)) {
            return Result::Failed;
        }
        handler->option(first, list);
        return Result::Handled;
    }
    if (name == "Format") {
        if (!number(lexer, &a) || !number(lexer, &b) || !number(lexer, &c)) {
            return Result::Failed;
        }
        handler->format(static_cast<unsigned int>(a), static_cast<unsigned int>(b), c);
        return Result::Handled;
    }
    return Result::Unhandled;
}

/**
 * Reads the camera: what the projection is and what of it reaches the picture.
 **/
RIBReader::Result RIBReader::cameraRequest(const std::string & name, RIBLexer * lexer, RIBHandler * handler) {
    float a = 0.0f;
    float b = 0.0f;
    float c = 0.0f;
    float d = 0.0f;
    std::string first;
    ParameterList list;

    if (name == "FrameAspectRatio") {
        if (!number(lexer, &a)) {
            return Result::Failed;
        }
        handler->frameAspectRatio(a);
        return Result::Handled;
    }
    if (name == "ScreenWindow") {
        if (!number(lexer, &a) || !number(lexer, &b) || !number(lexer, &c) || !number(lexer, &d)) {
            return Result::Failed;
        }
        handler->screenWindow(a, b, c, d);
        return Result::Handled;
    }
    if (name == "CropWindow") {
        if (!number(lexer, &a) || !number(lexer, &b) || !number(lexer, &c) || !number(lexer, &d)) {
            return Result::Failed;
        }
        handler->cropWindow(a, b, c, d);
        return Result::Handled;
    }
    if (name == "Projection") {
        if (!text(lexer, &first) || !parameters(lexer, 1, &list)) {
            return Result::Failed;
        }
        handler->projection(first, list);
        return Result::Handled;
    }
    if (name == "Clipping") {
        if (!number(lexer, &a) || !number(lexer, &b)) {
            return Result::Failed;
        }
        handler->clipping(a, b);
        return Result::Handled;
    }
    return Result::Unhandled;
}

/**
 * Reads where the picture goes and the frame it belongs to.
 **/
RIBReader::Result RIBReader::displayRequest(const std::string & name, RIBLexer * lexer, RIBHandler * handler) {
    float a = 0.0f;
    std::string first;
    std::string second;
    std::string third;
    ParameterList list;

    if (name == "Display") {
        if (!text(lexer, &first) || !text(lexer, &second) || !text(lexer, &third)) {
            return Result::Failed;
        }
        if (!parameters(lexer, 1, &list)) {
            return Result::Failed;
        }
        handler->display(first, second, third, list);
        return Result::Handled;
    }
    if (name == "FrameBegin") {
        if (!number(lexer, &a)) {
            return Result::Failed;
        }
        handler->frameBegin(static_cast<int>(a));
        return Result::Handled;
    }
    if (name == "FrameEnd") {
        handler->frameEnd();
        return Result::Handled;
    }
    return Result::Unhandled;
}

/**
 * Reads the blocks a scene is nested out of. None of them carries an argument.
 **/
RIBReader::Result RIBReader::blockRequest(const std::string & name, RIBHandler * handler) {
    if (name == "WorldBegin") {
        handler->worldBegin();
        return Result::Handled;
    }
    if (name == "WorldEnd") {
        handler->worldEnd();
        return Result::Handled;
    }
    if (name == "AttributeBegin") {
        handler->attributeBegin();
        return Result::Handled;
    }
    if (name == "AttributeEnd") {
        handler->attributeEnd();
        return Result::Handled;
    }
    if (name == "TransformBegin") {
        handler->transformBegin();
        return Result::Handled;
    }
    if (name == "TransformEnd") {
        handler->transformEnd();
        return Result::Handled;
    }
    return Result::Unhandled;
}

/**
 * Reads the current transformation.
 **/
RIBReader::Result RIBReader::transformRequest(const std::string & name, RIBLexer * lexer, RIBHandler * handler) {
    glm::mat4x4 m(1.0f);
    std::vector<float> triple;

    if (name == "Identity") {
        handler->identity();
        return Result::Handled;
    }
    if (name == "Transform") {
        if (!matrix(lexer, &m)) {
            return Result::Failed;
        }
        handler->transform(m);
        return Result::Handled;
    }
    if (name == "ConcatTransform") {
        if (!matrix(lexer, &m)) {
            return Result::Failed;
        }
        handler->concatTransform(m);
        return Result::Handled;
    }
    if (name == "Translate") {
        if (!numbers(lexer, 3, &triple)) {
            return Result::Failed;
        }
        handler->translate(triple[0], triple[1], triple[2]);
        return Result::Handled;
    }
    if (name == "Rotate") {
        if (!numbers(lexer, 4, &triple)) {
            return Result::Failed;
        }
        handler->rotate(triple[0], triple[1], triple[2], triple[3]);
        return Result::Handled;
    }
    if (name == "Scale") {
        if (!numbers(lexer, 3, &triple)) {
            return Result::Failed;
        }
        handler->scale(triple[0], triple[1], triple[2]);
        return Result::Handled;
    }
    return Result::Unhandled;
}

/**
 * Reads the attributes a primitive is submitted under.
 **/
RIBReader::Result RIBReader::attributeRequest(const std::string & name, RIBLexer * lexer, RIBHandler * handler) {
    float a = 0.0f;
    std::string first;
    ParameterList list;
    std::vector<float> triple;

    if (name == "Color") {
        if (!numbers(lexer, 3, &triple)) {
            return Result::Failed;
        }
        handler->color(glm::vec3(triple[0], triple[1], triple[2]));
        return Result::Handled;
    }
    if (name == "Opacity") {
        if (!numbers(lexer, 3, &triple)) {
            return Result::Failed;
        }
        handler->opacity(glm::vec3(triple[0], triple[1], triple[2]));
        return Result::Handled;
    }
    if (name == "ShadingRate") {
        if (!number(lexer, &a)) {
            return Result::Failed;
        }
        handler->shadingRate(a);
        return Result::Handled;
    }
    if (name == "Attribute") {
        if (!text(lexer, &first) || !parameters(lexer, 1, &list)) {
            return Result::Failed;
        }
        handler->attribute(first, list);
        return Result::Handled;
    }
    return Result::Unhandled;
}

/**
 * Reads the shaders a surface and a light are shaded by.
 **/
RIBReader::Result RIBReader::shaderRequest(const std::string & name, RIBLexer * lexer, RIBHandler * handler) {
    std::string first;
    ParameterList list;

    if (name == "Surface") {
        if (!text(lexer, &first) || !parameters(lexer, 1, &list)) {
            return Result::Failed;
        }
        handler->surface(first, list);
        return Result::Handled;
    }
    if (name == "LightSource") {
        if (!text(lexer, &first)) {
            return Result::Failed;
        }
        // the handle follows the shader name and is a sequence number in RIB 3.x
        if (lexer->peek().kind() == Kind::NUMBER) {
            lexer->next();
        }
        if (!parameters(lexer, 1, &list)) {
            return Result::Failed;
        }
        handler->lightSource(first, list);
        return Result::Handled;
    }
    return Result::Unhandled;
}

/**
 * Reads the geometry.
 **/
RIBReader::Result RIBReader::primitiveRequest(const std::string & name, RIBLexer * lexer, RIBHandler * handler) {
    float a = 0.0f;
    float b = 0.0f;
    float c = 0.0f;
    float d = 0.0f;
    ParameterList list;

    if (name == "Polygon") {
        // RIB carries no vertex count: it is the length of the position array
        if (!parameters(lexer, 0, &list)) {
            return Result::Failed;
        }
        unsigned int vertices = static_cast<unsigned int>(list.floats("P").size() / 3);
        if (vertices == 0) {
            vertices = static_cast<unsigned int>(list.floats("Pw").size() / 4);
        }
        if (vertices == 0) {
            vertices = static_cast<unsigned int>(list.floats("Pz").size());
        }
        handler->polygon(vertices, list);
        return Result::Handled;
    }
    if (name == "PointsPolygons") {
        std::vector<unsigned int> perPolygon;
        std::vector<unsigned int> indices;
        if (!counts(lexer, &perPolygon) || !counts(lexer, &indices) || !parameters(lexer, 0, &list)) {
            return Result::Failed;
        }
        handler->pointsPolygons(perPolygon, indices, list);
        return Result::Handled;
    }
    if (name == "Sphere") {
        if (!number(lexer, &a) || !number(lexer, &b) || !number(lexer, &c) || !number(lexer, &d)) {
            return Result::Failed;
        }
        if (!parameters(lexer, 1, &list)) {
            return Result::Failed;
        }
        handler->sphere(a, b, c, d, list);
        return Result::Handled;
    }
    return Result::Unhandled;
}

bool RIBReader::request(const std::string & name, RIBLexer * lexer, RIBHandler * handler) {
    // the groups are asked in turn, and the first that recognises the name consumes the
    // request's arguments. Order is not significant - no name belongs to two of them.
    Result result = optionRequest(name, lexer, handler);
    if (result == Result::Unhandled) {
        result = cameraRequest(name, lexer, handler);
    }
    if (result == Result::Unhandled) {
        result = displayRequest(name, lexer, handler);
    }
    if (result == Result::Unhandled) {
        result = blockRequest(name, handler);
    }
    if (result == Result::Unhandled) {
        result = transformRequest(name, lexer, handler);
    }
    if (result == Result::Unhandled) {
        result = attributeRequest(name, lexer, handler);
    }
    if (result == Result::Unhandled) {
        result = shaderRequest(name, lexer, handler);
    }
    if (result == Result::Unhandled) {
        result = primitiveRequest(name, lexer, handler);
    }
    if (result != Result::Unhandled) {
        return result == Result::Handled;
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
