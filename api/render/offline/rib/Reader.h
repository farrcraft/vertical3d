/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>

#include <iosfwd>
#include <set>
#include <string>
#include <vector>

#include "Declarations.h"
#include "Handler.h"
#include "Lexer.h"
#include "Parameters.h"

#include <boost/shared_ptr.hpp>
#include <glm/mat4x4.hpp>

namespace v3d::render::offline::rib {

/**
 * Reads an ASCII RIB stream and drives a handler, per ADR-0023 and ADR-0025.
 *
 * The handler is a parameter rather than a member, so one reader serves both renderers and
 * a suite can drive it with a handler that only counts.
 *
 * An unrecognised request is reported once per name rather than once per occurrence, and
 * its arguments are skipped: only a string, a number or an array can be an argument, so
 * the next identifier begins the next request whatever this one was. **A scene that
 * rendered nothing and a scene that was not understood look identical from outside**,
 * which is why the report exists at all.
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

 private:
    /**
     * What one group of requests made of a name it was offered.
     *
     * Unhandled is not a failure: it means the name belongs to another group, and the next
     * one is asked. Only the last group's Unhandled is a request this reader does not know.
     **/
    enum class Result {
        Unhandled,
        Handled,
        Failed
    };

    bool request(const std::string & name, Lexer * lexer, Handler * handler);

    /**
     * The request set, split the way the RI standard groups it. Each takes the name a
     * request began with and either recognises it or passes.
     **/
    Result optionRequest(const std::string & name, Lexer * lexer, Handler * handler);
    Result cameraRequest(const std::string & name, Lexer * lexer, Handler * handler);
    Result displayRequest(const std::string & name, Lexer * lexer, Handler * handler);
    Result blockRequest(const std::string & name, Handler * handler);
    Result transformRequest(const std::string & name, Lexer * lexer, Handler * handler);
    Result attributeRequest(const std::string & name, Lexer * lexer, Handler * handler);
    Result shaderRequest(const std::string & name, Lexer * lexer, Handler * handler);
    Result primitiveRequest(const std::string & name, Lexer * lexer, Handler * handler);

    void skipArguments(Lexer * lexer);

    /**
     * The token-value pairs that close a request.
     *
     * @param vertices how many vertices the primitive has, or 0 when that is not known
     *        until the list has been read - which is the case for every RIB primitive,
     *        since the count is the length of "P". An unbracketed varying or vertex
     *        parameter is then unreadable and ends the parse.
     **/
    bool parameters(Lexer * lexer, unsigned int vertices, ParameterList * list);
    bool values(Lexer * lexer, const Declaration & declaration, unsigned int vertices,
        const std::string & name, std::vector<float> * floats, std::vector<std::string> * strings);
    void skipArray(Lexer * lexer);

    bool number(Lexer * lexer, float * value);
    bool text(Lexer * lexer, std::string * value);
    bool numbers(Lexer * lexer, unsigned int count, std::vector<float> * out);
    bool matrix(Lexer * lexer, glm::mat4x4 * out);
    bool counts(Lexer * lexer, std::vector<unsigned int> * out);

    bool fail(const std::string & message, const Token & token);

    boost::shared_ptr<v3d::log::Logger> logger_;
    Declarations declarations_;
    std::string error_;
    std::vector<std::string> unrecognised_;
    std::set<std::string> reported_;
};

};  // namespace v3d::render::offline::rib
