/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/log/Logger.h>
#include <api/render/offline/rib/Declaration.h>
#include <api/render/offline/rib/Declarations.h>
#include <api/render/offline/rib/Lexer.h>
#include <api/render/offline/rib/Parameters.h>
#include <api/render/offline/rib/Token.h>
#include <api/render/offline/rib/requests/Basis.h>
#include <api/render/offline/rib/requests/Declare.h>

#include <set>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/mat4x4.hpp>

namespace v3d::render::offline::rib::internal {

/**
 * Reads the arguments of a RIB request from a lexer: numbers, strings, arrays, handles, and the
 * parameter list that closes a request. It holds what the stream has declared, and resolves each
 * parameter's name through it.
 *
 * Each read returns false when the stream does not give what it asks for. The first such failure
 * is kept as the error, with the line and column of the token that caused it.
 **/
class Operands final {
 public:
    explicit Operands(const boost::shared_ptr<v3d::log::Logger> & logger);

    /** Forget what an earlier stream declared and reported, and what stopped it. **/
    void reset();

    /**
     * Ri Declare.
     *
     * @return false when the declaration text does not parse, leaving the table alone
     **/
    bool declare(const requests::Declare & request);

    /** What stopped the parse, or empty. **/
    const std::string & error() const;

    /**
     * Keep a message and the token's position as the error, unless an earlier failure is kept.
     *
     * @return false
     **/
    bool fail(const std::string & message, const Token & token);

    /** Keep a message that has no token as the error, unless an earlier failure is kept. **/
    void stop(const std::string & message);

    bool number(Lexer * lexer, float * value);
    bool text(Lexer * lexer, std::string * value);
    /** A run of numbers, bare or in one bracketed array. **/
    bool numbers(Lexer * lexer, unsigned int count, std::vector<float> * out);
    bool matrix(Lexer * lexer, glm::mat4x4 * out);
    /** A bracketed array of counts. **/
    bool counts(Lexer * lexer, std::vector<unsigned int> * out);
    /** A bracketed array of numbers of any length. **/
    bool floatArray(Lexer * lexer, std::vector<float> * out);
    /** A bracketed array of quoted strings of any length. **/
    bool strings(Lexer * lexer, std::vector<std::string> * out);

    /**
     * A light's or an object's handle, which RIB 3.03 writes as a number and later RIB writes as
     * a string.
     **/
    bool handle(Lexer * lexer, std::string * value);

    /** One direction of a Basis request: a name or sixteen numbers, then a step from 1 to 4. **/
    bool basis(Lexer * lexer, requests::Basis::Matrix * matrix, unsigned int * step);

    /**
     * The token-value pairs that close a request. A parameter that was not declared is skipped
     * when an array bounds it, and reported once.
     *
     * @param vertices how many vertices the primitive has, or 0 when the count is the
     *        length of "P" and is not known until the list has been read. With 0, an
     *        unbracketed varying or vertex parameter has no known length and ends the parse.
     **/
    bool parameters(Lexer * lexer, unsigned int vertices, ParameterList * list);

 private:
    typedef Token::Kind Kind;

    static std::string position(const Token & token);

    /** One parameter's values: a bracketed array, or the bare values its declaration counts. **/
    bool values(Lexer * lexer, const Declaration & declaration, unsigned int vertices,
        const std::string & name, std::vector<float> * floats, std::vector<std::string> * strings);

    static void skipArray(Lexer * lexer);

    boost::shared_ptr<v3d::log::Logger> logger_;
    Declarations declarations_;
    std::string error_;
    /** The parameters already reported, so each is reported once. **/
    std::set<std::string> reported_;
};

};  // namespace v3d::render::offline::rib::internal
