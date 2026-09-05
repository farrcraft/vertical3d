/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <iosfwd>
#include <set>
#include <string>
#include <vector>

#include "RIBDeclarations.h"
#include "RIBHandler.h"
#include "RIBLexer.h"
#include "RIBParameters.h"

#include "../../log/Logger.h"

#include <boost/shared_ptr.hpp>
#include <glm/mat4x4.hpp>

namespace v3d::render::offline {

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
class RIBReader final {
 public:
    explicit RIBReader(const boost::shared_ptr<v3d::log::Logger> & logger);

    /**
     * @param path the file to read
     * @param handler where the requests go
     * @return whether the whole stream was understood
     **/
    bool read(const std::string & path, RIBHandler * handler);
    bool read(std::istream & stream, RIBHandler * handler);

    /**
     * What stopped the parse, or empty.
     **/
    const std::string & error() const;

    /**
     * The requests this reader does not implement, in the order they were first met.
     **/
    const std::vector<std::string> & unrecognised() const;

 private:
    bool request(const std::string & name, RIBLexer * lexer, RIBHandler * handler);
    void skipArguments(RIBLexer * lexer);

    /**
     * The token-value pairs that close a request.
     *
     * @param vertices how many vertices the primitive has, or 0 when that is not known
     *        until the list has been read - which is the case for every RIB primitive,
     *        since the count is the length of "P". An unbracketed varying or vertex
     *        parameter is then unreadable and ends the parse.
     **/
    bool parameters(RIBLexer * lexer, unsigned int vertices, ParameterList * list);
    bool values(RIBLexer * lexer, const RIBDeclaration & declaration, unsigned int vertices,
        const std::string & name, std::vector<float> * floats, std::vector<std::string> * strings);
    void skipArray(RIBLexer * lexer);

    bool number(RIBLexer * lexer, float * value);
    bool text(RIBLexer * lexer, std::string * value);
    bool numbers(RIBLexer * lexer, unsigned int count, std::vector<float> * out);
    bool matrix(RIBLexer * lexer, glm::mat4x4 * out);
    bool counts(RIBLexer * lexer, std::vector<unsigned int> * out);

    bool fail(const std::string & message, const RIBToken & token);

    boost::shared_ptr<v3d::log::Logger> logger_;
    RIBDeclarations declarations_;
    std::string error_;
    std::vector<std::string> unrecognised_;
    std::set<std::string> reported_;
};

};  // namespace v3d::render::offline
