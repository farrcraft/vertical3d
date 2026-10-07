/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Reader.h"

#include <cmath>
#include <cstdint>
#include <fstream>
#include <istream>
#include <limits>
#include <string>
#include <vector>

#include <glm/gtc/type_ptr.hpp>

namespace v3d::render::offline::rib {

namespace {

typedef Token::Kind Kind;

std::string position(const Token & token) {
    return " at line " + std::to_string(token.line()) + ", column " + std::to_string(token.column());
}

};  // namespace

Reader::Reader(const boost::shared_ptr<v3d::log::Logger> & logger) : logger_(logger) {
}

bool Reader::fail(const std::string & message, const Token & token) {
    if (error_.empty()) {
        error_ = message + position(token);
    }
    return false;
}

bool Reader::number(Lexer * lexer, float * value) {
    const Token token = lexer->next();
    if (token.kind() != Kind::NUMBER) {
        return fail("expected a number", token);
    }
    *value = token.value();
    return true;
}

bool Reader::text(Lexer * lexer, std::string * value) {
    const Token token = lexer->next();
    if (token.kind() != Kind::STRING) {
        return fail("expected a quoted string", token);
    }
    *value = token.text();
    return true;
}

bool Reader::numbers(Lexer * lexer, unsigned int count, std::vector<float> * out) {
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
        const Token token = lexer->next();
        if (token.kind() != Kind::ARRAY_END) {
            return fail("expected ']'", token);
        }
    }
    return true;
}

bool Reader::matrix(Lexer * lexer, glm::mat4x4 * out) {
    std::vector<float> values;
    if (!numbers(lexer, 16, &values)) {
        return false;
    }
    // read in RIB's order with no transpose, as ParameterList::matrix explains
    *out = glm::make_mat4(values.data());
    return true;
}

bool Reader::counts(Lexer * lexer, std::vector<unsigned int> * out) {
    const Token open = lexer->next();
    if (open.kind() != Kind::ARRAY_BEGIN) {
        return fail("expected '['", open);
    }
    for (;;) {
        const Token token = lexer->next();
        if (token.kind() == Kind::ARRAY_END) {
            return true;
        }
        // the largest float below 2^32, so the conversion to unsigned is defined
        const bool count = token.kind() == Kind::NUMBER && token.value() >= 0.0f && token.value() <= 4294967040.0f;
        if (!count) {
            return fail("expected a count", token);
        }
        out->push_back(static_cast<unsigned int>(token.value()));
    }
}

void Reader::skipArray(Lexer * lexer) {
    lexer->next();  // the opening bracket
    for (;;) {
        const Token token = lexer->next();
        if (token.kind() == Kind::ARRAY_END || token.kind() == Kind::END) {
            return;
        }
    }
}

void Reader::skipArguments(Lexer * lexer) {
    for (;;) {
        const Kind kind = lexer->peek().kind();
        if (kind == Kind::END || kind == Kind::IDENTIFIER) {
            return;
        }
        lexer->next();
    }
}

bool Reader::values(Lexer * lexer, const Declaration & declaration, unsigned int vertices,
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

bool Reader::parameters(Lexer * lexer, unsigned int vertices, ParameterList * list) {
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

Reader::Result Reader::namedRequest(const std::string & name, Lexer * lexer, Handler * handler) {
    typedef void (Handler::*Named)(const std::string &, const ParameterList &);
    static const struct { const char* name; Named forward; } table[] = {
        { "Option", &Handler::option },
        { "Hider", &Handler::hider },
        { "Projection", &Handler::projection },
        { "Attribute", &Handler::attribute },
        { "Surface", &Handler::surface },
        { "Imager", &Handler::imager }
    };
    for (const auto & entry : table) {
        if (name != entry.name) {
            continue;
        }
        std::string first;
        ParameterList list;
        if (!text(lexer, &first) || !parameters(lexer, 1, &list)) {
            return Result::Failed;
        }
        (handler->*entry.forward)(first, list);
        return Result::Handled;
    }
    return Result::Unhandled;
}

/**
 * Reads the RI options: what the picture is and what the scene calls things.
 **/
Reader::Result Reader::optionRequest(const std::string & name, Lexer * lexer, Handler * handler) {
    float a = 0.0f;
    float b = 0.0f;
    float c = 0.0f;
    std::string first;
    std::string second;

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
    if (name == "Format") {
        if (!number(lexer, &a) || !number(lexer, &b) || !number(lexer, &c)) {
            return Result::Failed;
        }
        // a size below one pixel or above 65536 is skipped, because converting it to
        // unsigned is undefined. A fraction is truncated. An aspect that is not a positive
        // finite number asks for the device's own, which is square pixels.
        const bool size = resolution(a) && resolution(b);
        if (!size) {
            logger_->get()->warn("RIB Format {} {} {} is not a picture size and was skipped", a, b, c);
            return Result::Handled;
        }
        handler->format(static_cast<unsigned int>(a), static_cast<unsigned int>(b), c > 0.0f && std::isfinite(c) ? c : 1.0f);
        return Result::Handled;
    }
    return Result::Unhandled;
}

/**
 * Reads the camera: what the projection is and what of it reaches the picture.
 **/
Reader::Result Reader::cameraRequest(const std::string & name, Lexer * lexer, Handler * handler) {
    float a = 0.0f;
    float b = 0.0f;
    float c = 0.0f;
    float d = 0.0f;

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
Reader::Result Reader::displayRequest(const std::string & name, Lexer * lexer, Handler * handler) {
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
        if (!std::isfinite(a) || std::fabs(a) > 2.0e9f) {
            logger_->get()->warn("RIB FrameBegin {} is not a frame number and was skipped", a);
            return Result::Handled;
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
 * Reads the lens and the shutter: the depth of field, and when the shutter opens and closes.
 **/
Reader::Result Reader::lensRequest(const std::string & name, Lexer * lexer, Handler * handler) {
    float a = 0.0f;
    float b = 0.0f;
    float c = 0.0f;

    if (name == "DepthOfField") {
        // the form with no arguments sets a pinhole
        if (lexer->peek().kind() != Kind::NUMBER) {
            handler->depthOfField(std::numeric_limits<float>::infinity(), 0.0f, 0.0f);
            return Result::Handled;
        }
        if (!number(lexer, &a) || !number(lexer, &b) || !number(lexer, &c)) {
            return Result::Failed;
        }
        handler->depthOfField(a, b, c);
        return Result::Handled;
    }
    if (name == "Shutter") {
        if (!number(lexer, &a) || !number(lexer, &b)) {
            return Result::Failed;
        }
        handler->shutter(a, b);
        return Result::Handled;
    }
    return Result::Unhandled;
}

/**
 * Reads how a pixel is sampled and filtered.
 **/
Reader::Result Reader::sampleRequest(const std::string & name, Lexer * lexer, Handler * handler) {
    float a = 0.0f;
    float b = 0.0f;
    std::string first;

    if (name == "PixelSamples") {
        if (!number(lexer, &a) || !number(lexer, &b)) {
            return Result::Failed;
        }
        handler->pixelSamples(sampleCount(a), sampleCount(b));
        return Result::Handled;
    }
    if (name == "PixelFilter") {
        if (!text(lexer, &first) || !number(lexer, &a) || !number(lexer, &b)) {
            return Result::Failed;
        }
        Filter filter = Filter::Gaussian;
        if (!filterNamed(first, &filter)) {
            // the request was understood and its filter was not, so the renderer keeps the
            // one it had rather than the parse failing
            if (reported_.insert("filter " + first).second) {
                logger_->get()->warn("RIB PixelFilter '{}' is not a filter and was skipped", first);
            }
            return Result::Handled;
        }
        if (!filterWidth(glm::vec2(a, b))) {
            logger_->get()->warn("RIB PixelFilter width {} {} is not a width and was skipped", a, b);
            return Result::Handled;
        }
        handler->pixelFilter(filter, a, b);
        return Result::Handled;
    }
    if (name == "PixelVariance") {
        if (!number(lexer, &a)) {
            return Result::Failed;
        }
        handler->pixelVariance(a);
        return Result::Handled;
    }
    return Result::Unhandled;
}

/**
 * Reads the blocks a scene is nested out of. None of them carries an argument.
 **/
// NOLINTNEXTLINE(readability-convert-member-functions-to-static) - a member, so it sits in the group table beside the others
Reader::Result Reader::blockRequest(const std::string & name, Lexer * lexer, Handler * handler) {
    (void)lexer;
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
Reader::Result Reader::transformRequest(const std::string & name, Lexer * lexer, Handler * handler) {
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
 * Reads a motion block's bounds. What it moves is the transform requests between them.
 **/
Reader::Result Reader::motionRequest(const std::string & name, Lexer * lexer, Handler * handler) {
    if (name == "MotionBegin") {
        std::vector<float> times;
        const Token open = lexer->next();
        if (open.kind() != Kind::ARRAY_BEGIN) {
            fail("expected '['", open);
            return Result::Failed;
        }
        while (lexer->peek().kind() == Kind::NUMBER) {
            times.push_back(lexer->next().value());
        }
        const Token close = lexer->next();
        if (close.kind() != Kind::ARRAY_END) {
            fail("expected ']'", close);
            return Result::Failed;
        }
        motion_ = true;
        motionPrimitives_ = 0;
        handler->motionBegin(times);
        return Result::Handled;
    }
    if (name == "MotionEnd") {
        motion_ = false;
        handler->motionEnd();
        return Result::Handled;
    }
    return Result::Unhandled;
}

/**
 * Reads the attributes a primitive is submitted under.
 **/
Reader::Result Reader::attributeRequest(const std::string & name, Lexer * lexer, Handler * handler) {
    float a = 0.0f;
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
    return Result::Unhandled;
}

/**
 * Reads the shaders a surface and a light are shaded by.
 **/
Reader::Result Reader::shaderRequest(const std::string & name, Lexer * lexer, Handler * handler) {
    std::string first;
    std::string second;
    ParameterList list;

    if (name == "LightSource" || name == "AreaLightSource") {
        if (!text(lexer, &first) || !handle(lexer, &second)) {
            return Result::Failed;
        }
        if (!parameters(lexer, 1, &list)) {
            return Result::Failed;
        }
        if (name == "AreaLightSource") {
            handler->areaLightSource(first, second, list);
        } else {
            handler->lightSource(first, second, list);
        }
        return Result::Handled;
    }
    if (name == "Illuminate") {
        float on = 0.0f;
        if (!handle(lexer, &first) || !number(lexer, &on)) {
            return Result::Failed;
        }
        handler->illuminate(first, on != 0.0f);
        return Result::Handled;
    }
    if (name == "MakeTexture") {
        std::string swrap;
        std::string twrap;
        std::string filter;
        float swidth = 0.0f;
        float twidth = 0.0f;
        if (!text(lexer, &first) || !text(lexer, &second) || !text(lexer, &swrap) || !text(lexer, &twrap) ||
            !text(lexer, &filter) || !number(lexer, &swidth) || !number(lexer, &twidth) || !parameters(lexer, 1, &list)) {
            return Result::Failed;
        }
        handler->makeTexture(first, second, swrap, twrap, filter, swidth, twidth, list);
        return Result::Handled;
    }
    return Result::Unhandled;
}

bool Reader::handle(Lexer * lexer, std::string * value) {
    // a number or a string, and a string to the handler either way
    const Token token = lexer->peek();
    if (token.kind() == Kind::NUMBER) {
        lexer->next();
        // a handle within a 32 bit integer either way, so the conversion is defined
        if (!(std::fabs(token.value()) <= 2.0e9f)) {
            return fail("expected a light handle", token);
        }
        *value = std::to_string(static_cast<std::int64_t>(token.value()));
        return true;
    }
    return text(lexer, value);
}

/**
 * Reads the geometry.
 **/
Reader::Result Reader::primitiveRequest(const std::string & name, Lexer * lexer, Handler * handler) {
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

bool Reader::request(const std::string & name, Lexer * lexer, Handler * handler) {
    // the groups are tried in turn, and the first that recognises the name consumes the
    // request's arguments. Order is not significant - no name belongs to two of them.
    typedef Result (Reader::*Group)(const std::string &, Lexer *, Handler *);
    static const Group groups[] = {
        &Reader::namedRequest, &Reader::optionRequest, &Reader::cameraRequest, &Reader::displayRequest,
        &Reader::lensRequest, &Reader::sampleRequest, &Reader::blockRequest, &Reader::transformRequest,
        &Reader::attributeRequest, &Reader::shaderRequest, &Reader::motionRequest
    };
    Result result = Result::Unhandled;
    for (const Group group : groups) {
        result = (this->*group)(name, lexer, handler);
        if (result != Result::Unhandled) {
            break;
        }
    }
    if (result == Result::Unhandled) {
        // a primitive after the first in a motion block is the same primitive deforming. It
        // goes to the handler's deformation(), and is dropped when that is null
        Handler nobody;
        const bool deforming = motion_ && motionPrimitives_ > 0;
        Handler* deformation = deforming ? handler->deformation() : nullptr;
        Handler* into = handler;
        if (deforming) {
            into = deformation != nullptr ? deformation : &nobody;
        }
        result = primitiveRequest(name, lexer, into);
        if (result == Result::Handled && motion_) {
            motionPrimitives_++;
            if (deforming && deformation == nullptr && reported_.insert("deforming " + name).second) {
                unsupported_.push_back("deforming " + name);
                logger_->get()->warn("RIB {} inside a motion block deforms, which is not supported; "
                    "it is drawn at the block's first time", name);
            }
        }
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

bool Reader::read(std::istream & stream, Handler * handler) {
    error_.clear();
    unrecognised_.clear();
    unsupported_.clear();
    reported_.clear();
    motion_ = false;
    motionPrimitives_ = 0;
    declarations_ = Declarations();

    Lexer lexer(stream);
    for (;;) {
        const Token token = lexer.next();
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

bool Reader::read(const std::string & path, Handler * handler) {
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

const std::string & Reader::error() const {
    return error_;
}

const std::vector<std::string> & Reader::unrecognised() const {
    return unrecognised_;
}

const std::vector<std::string> & Reader::unsupported() const {
    return unsupported_;
}

};  // namespace v3d::render::offline::rib
