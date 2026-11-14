/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/log/Logger.h>
#include <api/render/offline/rib/internal/Operands.h>

#include <cstdint>
#include <iosfwd>
#include <set>
#include <string>
#include <vector>

#include "Handler.h"
#include "Lexer.h"
#include "Token.h"

#include <boost/shared_ptr.hpp>

/**
 * RIB: the lexer and the reader of an ASCII RIB stream, the handler a renderer implements, the
 * declarations and parameters of a request, and a writer and a recorder of requests.
 **/
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
    typedef Token::Kind Kind;

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
    Result networkRequest(const std::string & name, Lexer * lexer, Handler * handler);
    Result objectRequest(const std::string & name, Lexer * lexer, Handler * handler);
    /** The geometric primitives, which internal::Primitives reads. **/
    Result primitiveRequest(const std::string & name, Lexer * lexer, Handler * handler);

    /** Skip an unrecognised request's arguments, up to the next request's name. **/
    static void skipArguments(Lexer * lexer);

    boost::shared_ptr<v3d::log::Logger> logger_;
    /** Reads each request's arguments, and holds what the stream declared and what stopped it. **/
    internal::Operands operands_;
    std::vector<std::string> unrecognised_;
    std::vector<std::string> unsupported_;
    /** The requests and filters already reported, so each is reported once. **/
    std::set<std::string> reported_;
    /** Whether a motion block is open, and how many primitives it has held. **/
    bool motion_ = false;
    unsigned int motionPrimitives_ = 0;
};

};  // namespace v3d::render::offline::rib
