/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include "Reader.h"

#include <api/log/Logger.h>
#include <api/rib/Sampling.h>
#include <api/rib/internal/Operands.h>
#include <api/rib/internal/Primitives.h>
#include <api/rib/requests/Basis.h>
#include <api/type/Checked.h>

#include "Handler.h"
#include "Lexer.h"
#include "Parameters.h"
#include "Token.h"

#include <cmath>
#include <cstdint>
#include <fstream>
#include <ios>
#include <istream>
#include <limits>
#include <optional>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::rib {

Reader::Reader(const boost::shared_ptr<v3d::log::Logger> & logger) : logger_(logger), operands_(logger) {
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

Reader::Result Reader::namedRequest(const std::string & name, Lexer * lexer, Handler * handler) {
    typedef void (Handler::*Named)(const std::string &, const ParameterList &);
    static const struct { const char* name; Named forward; } table[] = {
        { "Option", &Handler::option },
        { "Hider", &Handler::hider },
        { "Projection", &Handler::projection },
        { "Attribute", &Handler::attribute },
        { "Surface", &Handler::surface },
        { "Displacement", &Handler::displacement },
        { "Imager", &Handler::imager }
    };
    for (const auto & entry : table) {
        if (name != entry.name) {
            continue;
        }
        std::string first;
        ParameterList list;
        if (!operands_.text(lexer, &first) || !operands_.parameters(lexer, 1, &list)) {
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
    float arg1 = 0.0F;
    float arg2 = 0.0F;
    float arg3 = 0.0F;
    std::string first;
    std::string second;

    if (name == "version") {
        if (!operands_.number(lexer, &arg1)) {
            return Result::Failed;
        }
        handler->version(arg1);
        return Result::Handled;
    }
    if (name == "Declare") {
        if (!operands_.text(lexer, &first) || !operands_.text(lexer, &second)) {
            return Result::Failed;
        }
        if (!operands_.declare({.name = first, .type = second})) {
            logger_->get()->warn("RIB declaration of '{}' does not name a type", first);
        }
        handler->declare({.name = first, .type = second});
        return Result::Handled;
    }
    if (name == "Format") {
        if (!operands_.number(lexer, &arg1) || !operands_.number(lexer, &arg2) || !operands_.number(lexer, &arg3)) {
            return Result::Failed;
        }
        // a side that formatSide() refuses skips the request. A side of zero or less arrives
        // as 0, which asks for the renderer's default. An aspect that is not a positive finite
        // number becomes 1, square pixels.
        const std::optional<uint32_t> width = formatSide(arg1);
        const std::optional<uint32_t> height = formatSide(arg2);
        if (!width || !height) {
            logger_->get()->warn("RIB Format {} {} {} is not a picture size and was skipped", arg1, arg2, arg3);
            return Result::Handled;
        }
        handler->format({.width = *width, .height = *height, .pixelAspect = arg3 > 0.0F && std::isfinite(arg3) ? arg3 : 1.0F});
        return Result::Handled;
    }
    return Result::Unhandled;
}

/**
 * Reads the camera: what the projection is and what of it reaches the picture.
 **/
Reader::Result Reader::cameraRequest(const std::string & name, Lexer * lexer, Handler * handler) {
    float arg1 = 0.0F;
    float arg2 = 0.0F;
    float arg3 = 0.0F;
    float arg4 = 0.0F;

    if (name == "FrameAspectRatio") {
        if (!operands_.number(lexer, &arg1)) {
            return Result::Failed;
        }
        handler->frameAspectRatio(arg1);
        return Result::Handled;
    }
    if (name == "ScreenWindow") {
        if (!operands_.number(lexer, &arg1) || !operands_.number(lexer, &arg2) || !operands_.number(lexer, &arg3) || !operands_.number(lexer, &arg4)) {
            return Result::Failed;
        }
        handler->screenWindow({.left = arg1, .right = arg2, .bottom = arg3, .top = arg4});
        return Result::Handled;
    }
    if (name == "CropWindow") {
        if (!operands_.number(lexer, &arg1) || !operands_.number(lexer, &arg2) || !operands_.number(lexer, &arg3) || !operands_.number(lexer, &arg4)) {
            return Result::Failed;
        }
        handler->cropWindow({.xmin = arg1, .xmax = arg2, .ymin = arg3, .ymax = arg4});
        return Result::Handled;
    }
    if (name == "Clipping") {
        if (!operands_.number(lexer, &arg1) || !operands_.number(lexer, &arg2)) {
            return Result::Failed;
        }
        handler->clipping({.hither = arg1, .yon = arg2});
        return Result::Handled;
    }
    return Result::Unhandled;
}

/**
 * Reads where the picture goes and the frame it belongs to.
 **/
Reader::Result Reader::displayRequest(const std::string & name, Lexer * lexer, Handler * handler) {
    float arg1 = 0.0F;
    std::string first;
    std::string second;
    std::string third;
    ParameterList list;

    if (name == "Display") {
        if (!operands_.text(lexer, &first) || !operands_.text(lexer, &second) || !operands_.text(lexer, &third)) {
            return Result::Failed;
        }
        if (!operands_.parameters(lexer, 1, &list)) {
            return Result::Failed;
        }
        handler->display({.name = first, .type = second, .mode = third}, list);
        return Result::Handled;
    }
    if (name == "DisplayChannel") {
        if (!operands_.text(lexer, &first) || !operands_.parameters(lexer, 1, &list)) {
            return Result::Failed;
        }
        handler->displayChannel({ .declaration = first }, list);
        return Result::Handled;
    }
    if (name == "SampleFilter") {
        if (!operands_.text(lexer, &first) || !operands_.handle(lexer, &second) || !operands_.parameters(lexer, 1, &list)) {
            return Result::Failed;
        }
        handler->sampleFilter({ .type = first, .handle = second }, list);
        return Result::Handled;
    }
    if (name == "DisplayFilter") {
        if (!operands_.text(lexer, &first) || !operands_.handle(lexer, &second) || !operands_.parameters(lexer, 1, &list)) {
            return Result::Failed;
        }
        handler->displayFilter({ .type = first, .handle = second }, list);
        return Result::Handled;
    }
    if (name == "FrameBegin") {
        if (!operands_.number(lexer, &arg1)) {
            return Result::Failed;
        }
        const std::optional<int32_t> frame = v3d::type::toInteger(arg1, {.minimum = -2000000000, .maximum = 2000000000});
        if (!frame) {
            logger_->get()->warn("RIB FrameBegin {} is not a frame number and was skipped", arg1);
            return Result::Handled;
        }
        handler->frameBegin(*frame);
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
    float arg1 = 0.0F;
    float arg2 = 0.0F;
    float arg3 = 0.0F;

    if (name == "DepthOfField") {
        // the form with no arguments sets a pinhole
        if (lexer->peek().kind() != Kind::NUMBER) {
            handler->depthOfField({.fstop = std::numeric_limits<float>::infinity(), .focalLength = 0.0F, .focalDistance = 0.0F});
            return Result::Handled;
        }
        if (!operands_.number(lexer, &arg1) || !operands_.number(lexer, &arg2) || !operands_.number(lexer, &arg3)) {
            return Result::Failed;
        }
        handler->depthOfField({.fstop = arg1, .focalLength = arg2, .focalDistance = arg3});
        return Result::Handled;
    }
    if (name == "Shutter") {
        if (!operands_.number(lexer, &arg1) || !operands_.number(lexer, &arg2)) {
            return Result::Failed;
        }
        handler->shutter({.open = arg1, .close = arg2});
        return Result::Handled;
    }
    return Result::Unhandled;
}

/**
 * Reads how a pixel is sampled and filtered.
 **/
Reader::Result Reader::sampleRequest(const std::string & name, Lexer * lexer, Handler * handler) {
    float arg1 = 0.0F;
    float arg2 = 0.0F;
    std::string first;

    if (name == "PixelSamples") {
        if (!operands_.number(lexer, &arg1) || !operands_.number(lexer, &arg2)) {
            return Result::Failed;
        }
        handler->pixelSamples(glm::uvec2(sampleCount(arg1), sampleCount(arg2)));
        return Result::Handled;
    }
    if (name == "PixelFilter") {
        if (!operands_.text(lexer, &first) || !operands_.number(lexer, &arg1) || !operands_.number(lexer, &arg2)) {
            return Result::Failed;
        }
        Filter filter = Filter::Gaussian;
        if (!filterNamed(first, &filter)) {
            // an unknown filter skips the request without failing the parse, and the
            // renderer keeps its current filter
            if (reported_.insert("filter " + first).second) {
                logger_->get()->warn("RIB PixelFilter '{}' is not a filter and was skipped", first);
            }
            return Result::Handled;
        }
        if (!filterWidth(glm::vec2(arg1, arg2))) {
            logger_->get()->warn("RIB PixelFilter width {} {} is not a width and was skipped", arg1, arg2);
            return Result::Handled;
        }
        handler->pixelFilter(filter, arg1, arg2);
        return Result::Handled;
    }
    if (name == "PixelVariance") {
        if (!operands_.number(lexer, &arg1)) {
            return Result::Failed;
        }
        handler->pixelVariance(arg1);
        return Result::Handled;
    }
    return Result::Unhandled;
}

/**
 * Reads the blocks a scene is nested out of. None of them carries an argument. It is a member,
 * not static, so it sits in the group table beside the others.
 **/
// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
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
    glm::mat4x4 transformation(1.0F);
    std::vector<float> triple;

    if (name == "Identity") {
        handler->identity();
        return Result::Handled;
    }
    if (name == "Transform") {
        if (!operands_.matrix(lexer, &transformation)) {
            return Result::Failed;
        }
        handler->transform(transformation);
        return Result::Handled;
    }
    if (name == "ConcatTransform") {
        if (!operands_.matrix(lexer, &transformation)) {
            return Result::Failed;
        }
        handler->concatTransform(transformation);
        return Result::Handled;
    }
    if (name == "Translate") {
        if (!operands_.numbers(lexer, 3, &triple)) {
            return Result::Failed;
        }
        handler->translate(triple[0], triple[1], triple[2]);
        return Result::Handled;
    }
    if (name == "Rotate") {
        if (!operands_.numbers(lexer, 4, &triple)) {
            return Result::Failed;
        }
        handler->rotate(triple[0], glm::vec3(triple[1], triple[2], triple[3]));
        return Result::Handled;
    }
    if (name == "Scale") {
        if (!operands_.numbers(lexer, 3, &triple)) {
            return Result::Failed;
        }
        handler->scale(triple[0], triple[1], triple[2]);
        return Result::Handled;
    }
    if (name == "CoordinateSystem" || name == "CoordSysTransform") {
        std::string space;
        if (!operands_.text(lexer, &space)) {
            return Result::Failed;
        }
        if (name == "CoordinateSystem") {
            handler->coordinateSystem(space);
        } else {
            handler->coordSysTransform(space);
        }
        return Result::Handled;
    }
    return Result::Unhandled;
}

/**
 * Reads MotionBegin and MotionEnd, and records whether a motion block is open.
 **/
Reader::Result Reader::motionRequest(const std::string & name, Lexer * lexer, Handler * handler) {
    if (name == "MotionBegin") {
        std::vector<float> times;
        const Token open = lexer->next();
        if (open.kind() != Kind::ARRAY_BEGIN) {
            operands_.fail("expected '['", open);
            return Result::Failed;
        }
        while (lexer->peek().kind() == Kind::NUMBER) {
            times.push_back(lexer->next().value());
        }
        const Token close = lexer->next();
        if (close.kind() != Kind::ARRAY_END) {
            operands_.fail("expected ']'", close);
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
    float arg1 = 0.0F;
    std::vector<float> triple;

    if (name == "Color") {
        if (!operands_.numbers(lexer, 3, &triple)) {
            return Result::Failed;
        }
        handler->color(glm::vec3(triple[0], triple[1], triple[2]));
        return Result::Handled;
    }
    if (name == "Opacity") {
        if (!operands_.numbers(lexer, 3, &triple)) {
            return Result::Failed;
        }
        handler->opacity(glm::vec3(triple[0], triple[1], triple[2]));
        return Result::Handled;
    }
    if (name == "ShadingRate") {
        if (!operands_.number(lexer, &arg1)) {
            return Result::Failed;
        }
        handler->shadingRate(arg1);
        return Result::Handled;
    }
    if (name == "Sides") {
        if (!operands_.number(lexer, &arg1)) {
            return Result::Failed;
        }
        const std::optional<int32_t> count = v3d::type::toInteger(arg1, {.minimum = 1, .maximum = 2});
        if (!count) {
            logger_->get()->warn("RIB Sides {} is neither 1 nor 2 and was skipped", arg1);
            return Result::Handled;
        }
        handler->sides(*count);
        return Result::Handled;
    }
    if (name == "Basis") {
        requests::Basis request;
        if (!operands_.basis(lexer, &request.u, &request.ustep) || !operands_.basis(lexer, &request.v, &request.vstep)) {
            return Result::Failed;
        }
        handler->basis(request);
        return Result::Handled;
    }
    return Result::Unhandled;
}

/**
 * Reads RenderMan's nodes of a shading network: a Pattern or a Bxdf, of a type and a handle.
 **/
Reader::Result Reader::networkRequest(const std::string & name, Lexer * lexer, Handler * handler) {
    if (name != "Pattern" && name != "Bxdf") {
        return Result::Unhandled;
    }
    std::string type;
    std::string handle;
    ParameterList list;
    if (!operands_.text(lexer, &type) || !operands_.handle(lexer, &handle) || !operands_.parameters(lexer, 1, &list)) {
        return Result::Failed;
    }
    if (name == "Pattern") {
        handler->pattern({ .type = type, .handle = handle }, list);
    } else {
        handler->bxdf({ .type = type, .handle = handle }, list);
    }
    return Result::Handled;
}

/**
 * Reads the shaders a surface and a light are shaded by.
 **/
Reader::Result Reader::shaderRequest(const std::string & name, Lexer * lexer, Handler * handler) {
    std::string first;
    std::string second;
    ParameterList list;

    if (name == "LightSource" || name == "AreaLightSource" || name == "Light") {
        if (!operands_.text(lexer, &first) || !operands_.handle(lexer, &second)) {
            return Result::Failed;
        }
        if (!operands_.parameters(lexer, 1, &list)) {
            return Result::Failed;
        }
        if (name == "AreaLightSource") {
            handler->areaLightSource({.shader = first, .handle = second}, list);
        } else if (name == "Light") {
            handler->light({.shader = first, .handle = second}, list);
        } else {
            handler->lightSource({.shader = first, .handle = second}, list);
        }
        return Result::Handled;
    }
    if (name == "Illuminate") {
        float on = 0.0F;
        if (!operands_.handle(lexer, &first) || !operands_.number(lexer, &on)) {
            return Result::Failed;
        }
        handler->illuminate(first, on != 0.0F);
        return Result::Handled;
    }
    if (name == "MakeTexture") {
        std::string swrap;
        std::string twrap;
        std::string filter;
        float swidth = 0.0F;
        float twidth = 0.0F;
        if (!operands_.text(lexer, &first) || !operands_.text(lexer, &second) || !operands_.text(lexer, &swrap) || !operands_.text(lexer, &twrap) ||
            !operands_.text(lexer, &filter) || !operands_.number(lexer, &swidth) || !operands_.number(lexer, &twidth) || !operands_.parameters(lexer, 1, &list)) {
            return Result::Failed;
        }
        handler->makeTexture({.picture = first, .texture = second, .swrap = swrap, .twrap = twrap, .filter = filter, .swidth = swidth, .twidth = twidth}, list);
        return Result::Handled;
    }
    return Result::Unhandled;
}

/**
 * Reads the instancing requests. An object's handle is a number or a string, as a light's is.
 **/
Reader::Result Reader::objectRequest(const std::string & name, Lexer * lexer, Handler * handler) {
    if (name == "ObjectBegin" || name == "ObjectInstance") {
        std::string value;
        if (!operands_.handle(lexer, &value)) {
            return Result::Failed;
        }
        if (name == "ObjectBegin") {
            handler->objectBegin(value);
        } else {
            handler->objectInstance(value);
        }
        return Result::Handled;
    }
    if (name == "ObjectEnd") {
        handler->objectEnd();
        return Result::Handled;
    }
    return Result::Unhandled;
}

Reader::Result Reader::primitiveRequest(const std::string & name, Lexer * lexer, Handler * handler) {
    if (!internal::Primitives::recognises(name)) {
        return Result::Unhandled;
    }
    internal::Primitives primitives(&operands_);
    return primitives.read(name, lexer, handler) ? Result::Handled : Result::Failed;
}

bool Reader::request(const std::string & name, Lexer * lexer, Handler * handler) {
    // the groups are tried in turn, and the first that recognises the name consumes the
    // request's arguments. Order is not significant - no name belongs to two of them.
    typedef Result (Reader::*Group)(const std::string &, Lexer *, Handler *);
    static const Group groups[] = {
        &Reader::namedRequest, &Reader::optionRequest, &Reader::cameraRequest, &Reader::displayRequest,
        &Reader::lensRequest, &Reader::sampleRequest, &Reader::blockRequest, &Reader::transformRequest,
        &Reader::attributeRequest, &Reader::shaderRequest, &Reader::networkRequest, &Reader::motionRequest, &Reader::objectRequest
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
    operands_.reset();
    unrecognised_.clear();
    unsupported_.clear();
    reported_.clear();
    motion_ = false;
    motionPrimitives_ = 0;

    Lexer lexer(stream);
    for (;;) {
        const Token token = lexer.next();
        if (token.kind() == Kind::END) {
            break;
        }
        if (token.kind() != Kind::IDENTIFIER) {
            operands_.fail("expected a request", token);
            break;
        }
        if (!request(token.text(), &lexer, handler)) {
            break;
        }
    }

    if (!lexer.error().empty()) {
        operands_.stop(lexer.error());
    }

    for (const std::string & name : unrecognised_) {
        logger_->get()->warn("RIB request '{}' is not implemented and was skipped", name);
    }
    if (!operands_.error().empty()) {
        logger_->get()->error("RIB parse failed - {}", operands_.error());
        return false;
    }
    return true;
}

bool Reader::read(const std::string & path, Handler * handler) {
    // opened in binary mode so the lexer sees the opening bytes unchanged when it checks for
    // binary or gzipped RIB. The lexer treats a carriage return as whitespace.
    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file.is_open()) {
        operands_.reset();
        operands_.stop("could not open '" + path + "'");
        logger_->get()->error("RIB read failed - {}", operands_.error());
        return false;
    }
    return read(file, handler);
}

const std::string & Reader::error() const {
    return operands_.error();
}

const std::vector<std::string> & Reader::unrecognised() const {
    return unrecognised_;
}

const std::vector<std::string> & Reader::unsupported() const {
    return unsupported_;
}

};  // namespace v3d::rib
