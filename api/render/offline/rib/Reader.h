/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>

#include <cstdint>
#include <iosfwd>
#include <set>
#include <string>
#include <vector>

#include "Declaration.h"
#include "Declarations.h"
#include "Handler.h"
#include "Lexer.h"
#include "Parameters.h"
#include "Requests.h"
#include "Token.h"

#include <boost/shared_ptr.hpp>
#include <glm/mat4x4.hpp>

namespace v3d::render::offline::rib {

/**
 * Reads an ASCII RIB stream and calls a handler for each request.
 *
 * The handler is passed to read() and not stored, so a test can pass a handler that only
 * counts requests.
 *
 * An unrecognised request is reported once per name, and its arguments are skipped. An
 * argument is always a string, a number or an array, so the next identifier begins the next
 * request. The report matters because **a scene that rendered nothing looks, from outside,
 * the same as a scene that was not understood**.
 **/
class Reader final {
 public:
    explicit Reader(const boost::shared_ptr<v3d::log::Logger> & logger);

    /**
     * @param path the file to read
     * @param handler where the requests go
     * @return whether the whole stream was understood
     **/
    bool read(const std::string & path, Handler * handler);
    bool read(std::istream & stream, Handler * handler);

    /**
     * What stopped the parse, or empty.
     **/
    const std::string & error() const;

    /**
     * The requests this reader does not implement, in the order they were first met.
     **/
    const std::vector<std::string> & unrecognised() const;

    /**
     * The requests that were read but not built, once each. A primitive that deforms inside
     * a motion block is one.
     **/
    const std::vector<std::string> & unsupported() const;

 private:
    /**
     * The result of offering a request name to one group of requests.
     *
     * Unhandled is not a failure: it means the name belongs to another group, and the next
     * group is tried. Only the last group's Unhandled is a request this reader does not recognise.
     **/
    enum class Result : std::uint8_t {
        Unhandled,
        Handled,
        Failed
    };

    bool request(const std::string & name, Lexer * lexer, Handler * handler);

    /**
     * The request set, split the way the RI standard groups it. Each group takes a request's
     * name, and reads the request or returns Unhandled.
     **/
    /**
     * The requests that are a name and a parameter list and nothing else. They are read the
     * same way, and differ only in the handler method they call.
     **/
    Result namedRequest(const std::string & name, Lexer * lexer, Handler * handler);
    Result optionRequest(const std::string & name, Lexer * lexer, Handler * handler);
    Result cameraRequest(const std::string & name, Lexer * lexer, Handler * handler);
    Result displayRequest(const std::string & name, Lexer * lexer, Handler * handler);
    Result lensRequest(const std::string & name, Lexer * lexer, Handler * handler);
    Result sampleRequest(const std::string & name, Lexer * lexer, Handler * handler);
    Result blockRequest(const std::string & name, Lexer * lexer, Handler * handler);
    Result transformRequest(const std::string & name, Lexer * lexer, Handler * handler);
    Result motionRequest(const std::string & name, Lexer * lexer, Handler * handler);
    Result attributeRequest(const std::string & name, Lexer * lexer, Handler * handler);
    Result shaderRequest(const std::string & name, Lexer * lexer, Handler * handler);
    Result primitiveRequest(const std::string & name, Lexer * lexer, Handler * handler);
    Result curvesRequest(Lexer * lexer, Handler * handler);

    static void skipArguments(Lexer * lexer);

    /**
     * The token-value pairs that close a request.
     *
     * @param vertices how many vertices the primitive has, or 0 when the count is the
     *        length of "P" and is not known until the list has been read. With 0, an
     *        unbracketed varying or vertex parameter has no known length and ends the parse.
     **/
    bool parameters(Lexer * lexer, unsigned int vertices, ParameterList * list);
    /** One direction of a Basis request: a name or sixteen numbers, then a step from 1 to 4. **/
    bool basis(Lexer * lexer, Basis::Matrix * matrix, unsigned int * step);
    bool values(Lexer * lexer, const Declaration & declaration, unsigned int vertices,
        const std::string & name, std::vector<float> * floats, std::vector<std::string> * strings);
    static void skipArray(Lexer * lexer);

    bool number(Lexer * lexer, float * value);
    /**
     * A light handle, which RIB 3.03 writes as a number and later RIB writes as a string.
     **/
    bool handle(Lexer * lexer, std::string * value);
    bool text(Lexer * lexer, std::string * value);
    bool numbers(Lexer * lexer, unsigned int count, std::vector<float> * out);
    bool matrix(Lexer * lexer, glm::mat4x4 * out);
    bool counts(Lexer * lexer, std::vector<unsigned int> * out);

    bool fail(const std::string & message, const Token & token);

    boost::shared_ptr<v3d::log::Logger> logger_;
    Declarations declarations_;
    std::string error_;
    std::vector<std::string> unrecognised_;
    std::vector<std::string> unsupported_;
    std::set<std::string> reported_;
    /** Whether a motion block is open, and how many primitives it has held. **/
    bool motion_ = false;
    unsigned int motionPrimitives_ = 0;
};

};  // namespace v3d::render::offline::rib
