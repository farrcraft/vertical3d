/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <boost/json.hpp>

#include "Strip.h"

struct JSRuntime;
struct JSContext;

namespace v3d::script {

/**
 * What a function a script calls throws to refuse the call. The script sees it as an Error with
 * the same message, which it can catch.
 **/
class Refused final : public std::runtime_error {
 public:
    explicit Refused(const std::string& message);
};

/**
 * Something a script can call: it takes the call's arguments as JSON values, and returns one.
 * An argument a JSON value cannot hold, such as a function or undefined, arrives as null. A
 * null returned is undefined in the script, so a function with nothing to return gives nothing.
 **/
using Function = std::function<boost::json::value(const boost::json::array& arguments)>;

/**
 * What one run of a script did.
 **/
struct Result final {
    std::string value;                  /**< the value of the last statement run, as a person reads it, or empty for none **/
    std::optional<Problem> problem;     /**< why it stopped early, and where when that is known **/
};

/**
 * Runs TypeScript, on QuickJS-ng.
 *
 * A source has its types stripped, as strip() does, and then runs as a script in one global
 * scope that every run shares, so what one run declares the next can use. A script reaches the
 * host only through the functions bound into it, each by a path of names such as
 * "timeline.set". It has the language's own built-ins and nothing else: no files, no network
 * and no modules.
 *
 * A run that takes longer than its time limit is stopped, and so is one that asks for more
 * memory or stack than the runtime allows. The runtime is used from one thread.
 **/
class Runtime final {
 public:
    Runtime();
    ~Runtime();
    Runtime(const Runtime&) = delete;
    Runtime& operator=(const Runtime&) = delete;

    /**
     * Make a function callable from a script by a path of names joined by dots. The objects
     * along the path are made when they do not exist. A path bound again replaces the function.
     * @return false, binding nothing, when the path is empty, holds an empty name, or passes
     *         through something that is not an object
     **/
    bool bind(std::string_view path, Function function);

    /**
     * Strip a source's types and run it.
     * @param source TypeScript, or JavaScript, which has no types to strip
     * @param name what the source is called in a problem it reports
     **/
    Result run(std::string_view source, std::string_view name);

    /**
     * How long one run may take before it is stopped, unless limit() says otherwise.
     **/
    static constexpr std::chrono::milliseconds LIMIT{ 1000 };

    /**
     * How long one run may take before it is stopped.
     **/
    void limit(std::chrono::milliseconds time) noexcept;

 private:
    /**
     * Whether the run under way has passed its deadline, which the engine asks as it runs.
     **/
    static int interrupted(JSRuntime* runtime, void* opaque);

    JSRuntime* runtime_;
    JSContext* context_;
    // the class of the objects that hold a bound function's address
    std::uint32_t holder_ = 0;
    // each bound function, by the index the engine's function holds
    std::vector<std::unique_ptr<Function>> functions_;
    std::chrono::milliseconds limit_{ LIMIT };
    std::chrono::steady_clock::time_point deadline_;
};

};  // namespace v3d::script
