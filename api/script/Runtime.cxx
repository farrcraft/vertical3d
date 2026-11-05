/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Runtime.h"

#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>
#include <string_view>
#include <utility>
#include <vector>

#include <boost/json.hpp>
#include <quickjs.h>  // NOLINT(build/include_order)

#include "Strip.h"

namespace v3d::script {

namespace {

/**
 * The most memory a runtime's scripts may hold, and the most stack a run may take.
 **/
constexpr std::size_t MEMORY = std::size_t{ 256 } * 1024 * 1024;
constexpr std::size_t STACK = std::size_t{ 256 } * 1024;

/**
 * @return the text of a value as the engine converts it, which is empty when it cannot
 **/
std::string text(JSContext* context, JSValueConst value) {
    const char* converted = JS_ToCString(context, value);
    if (converted == nullptr) {
        JS_FreeValue(context, JS_GetException(context));
        return {};
    }
    std::string made(converted);
    JS_FreeCString(context, converted);
    return made;
}

/**
 * @return a value as JSON text, or nothing when JSON cannot hold it, as a function or undefined
 **/
std::optional<std::string> json(JSContext* context, JSValueConst value) {
    const JSValue written = JS_JSONStringify(context, value, JS_UNDEFINED, JS_UNDEFINED);
    if (JS_IsException(written)) {
        JS_FreeValue(context, JS_GetException(context));
        return std::nullopt;
    }
    std::optional<std::string> made;
    if (JS_IsString(written)) {
        made = text(context, written);
    }
    JS_FreeValue(context, written);
    return made;
}

/**
 * @return a value as a person reads it: a string in quotes, an object or an array as JSON, and
 *         anything else as the engine writes it. Undefined is nothing
 **/
std::string shown(JSContext* context, JSValueConst value) {
    if (JS_IsUndefined(value)) {
        return {};
    }
    if (JS_IsString(value) || (JS_IsObject(value) && !JS_IsFunction(context, value))) {
        if (const std::optional<std::string> written = json(context, value)) {
            return *written;
        }
    }
    return text(context, value);
}

/**
 * @return where in a source an error arose, read from the first place its stack names in the
 *         source, as "name:line:column"
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the stack, then the source it names
std::optional<std::pair<std::size_t, std::size_t>> place(std::string_view stack, std::string_view name) {
    const std::string mark = std::string(name) + ":";
    for (std::size_t at = stack.find(mark); at != std::string_view::npos; at = stack.find(mark, at + 1)) {
        const char* const end = stack.data() + stack.size();
        std::size_t line = 0;
        std::size_t column = 0;
        const std::from_chars_result lineRead = std::from_chars(stack.data() + at + mark.size(), end, line);
        if (lineRead.ec != std::errc() || lineRead.ptr == end || *lineRead.ptr != ':') {
            continue;
        }
        if (std::from_chars(lineRead.ptr + 1, end, column).ec == std::errc()) {
            return std::make_pair(line, column);
        }
    }
    return std::nullopt;
}

/**
 * @return what an exception the engine holds says, and where it arose in the named source,
 *         which takes it from the engine
 **/
Problem caught(JSContext* context, std::string_view name) {
    const JSValue exception = JS_GetException(context);
    Problem problem;
    if (JS_IsError(exception)) {
        const JSValue message = JS_GetPropertyStr(context, exception, "message");
        const JSValue kind = JS_GetPropertyStr(context, exception, "name");
        const JSValue stack = JS_GetPropertyStr(context, exception, "stack");
        problem.message = text(context, kind) + ": " + text(context, message);
        if (JS_IsString(stack)) {
            if (const auto at = place(text(context, stack), name)) {
                problem.line = at->first;
                problem.column = at->second;
            }
        }
        JS_FreeValue(context, stack);
        JS_FreeValue(context, kind);
        JS_FreeValue(context, message);
    } else {
        problem.message = "uncaught " + shown(context, exception);
    }
    if (JS_IsUncatchableError(exception)) {
        problem.message = "the script was stopped: it ran past its time, or ran out of memory or stack";
    }
    JS_FreeValue(context, exception);
    return problem;
}

/**
 * Call a bound function from a script: its arguments become JSON values, and the value it
 * returns becomes the call's.
 **/
JSValue call(JSContext* context, JSValueConst self, int count, JSValueConst* values, int magic, JSValueConst* data) {
    static_cast<void>(self);
    static_cast<void>(magic);
    auto* function = static_cast<Function*>(JS_GetOpaque(data[0], JS_GetClassID(data[0])));
    boost::json::array arguments;
    for (int each = 0; each < count; ++each) {
        const std::optional<std::string> written = json(context, values[each]);
        // an argument JSON cannot hold arrives as null
        boost::json::value parsed(nullptr);
        if (written) {
            boost::system::error_code error;
            parsed = boost::json::parse(*written, error);
            if (error) {
                parsed = nullptr;
            }
        }
        arguments.push_back(std::move(parsed));
    }
    try {
        const boost::json::value value = (*function)(arguments);
        if (value.is_null()) {
            return JS_UNDEFINED;
        }
        const std::string returned = boost::json::serialize(value);
        return JS_ParseJSON(context, returned.c_str(), returned.size(), "<returned>");
    } catch (const Refused& refused) {
        return JS_ThrowPlainError(context, "%s", refused.what());
    } catch (const std::exception& failed) {
        return JS_ThrowInternalError(context, "%s", failed.what());
    }
}

};  // namespace

/**
 **/
Refused::Refused(const std::string& message) :
    std::runtime_error(message) {
}

/**
 **/
Runtime::Runtime() :
    runtime_(JS_NewRuntime()),
    context_(nullptr) {
    JS_SetMemoryLimit(runtime_, MEMORY);
    JS_SetMaxStackSize(runtime_, STACK);
    JS_SetInterruptHandler(runtime_, &Runtime::interrupted, this);
    // the class of the objects that hold a bound function's address, which a script never sees
    JS_NewClassID(runtime_, &holder_);
    const JSClassDef holder{ .class_name = "Bound", .finalizer = nullptr, .gc_mark = nullptr, .call = nullptr, .exotic = nullptr };
    JS_NewClass(runtime_, holder_, &holder);
    context_ = JS_NewContext(runtime_);
}

/**
 **/
Runtime::~Runtime() {
    JS_FreeContext(context_);
    JS_FreeRuntime(runtime_);
}

/**
 **/
bool Runtime::bind(std::string_view path, Function function) {
    std::vector<std::string> names;
    for (std::size_t start = 0;;) {
        const std::size_t dot = path.find('.', start);
        names.emplace_back(path.substr(start, dot == std::string_view::npos ? std::string_view::npos : dot - start));
        if (names.back().empty()) {
            return false;
        }
        if (dot == std::string_view::npos) {
            break;
        }
        start = dot + 1;
    }
    JSValue holder = JS_GetGlobalObject(context_);
    for (std::size_t each = 0; each + 1 < names.size(); ++each) {
        JSValue next = JS_GetPropertyStr(context_, holder, names[each].c_str());
        if (JS_IsUndefined(next)) {
            next = JS_NewObject(context_);
            JS_SetPropertyStr(context_, holder, names[each].c_str(), JS_DupValue(context_, next));
        }
        JS_FreeValue(context_, holder);
        if (!JS_IsObject(next)) {
            JS_FreeValue(context_, next);
            return false;
        }
        holder = next;
    }
    functions_.push_back(std::make_unique<Function>(std::move(function)));
    // the engine's function holds the C++ function's address in an object of its own
    JSValue opaque = JS_NewObjectClass(context_, static_cast<int>(holder_));
    JS_SetOpaque(opaque, functions_.back().get());
    const JSValue bound = JS_NewCFunctionData(context_, &call, 1, 0, 1, &opaque);
    JS_FreeValue(context_, opaque);
    JS_SetPropertyStr(context_, holder, names.back().c_str(), bound);
    JS_FreeValue(context_, holder);
    return true;
}

/**
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the source, then what it is called
Result Runtime::run(std::string_view source, std::string_view name) {
    const Stripped stripped = strip(source);
    if (stripped.problem) {
        return { .value = {}, .problem = stripped.problem };
    }
    deadline_ = std::chrono::steady_clock::now() + limit_;
    JS_UpdateStackTop(runtime_);
    const std::string file(name);
    const JSValue result = JS_Eval(context_, stripped.code.c_str(), stripped.code.size(), file.c_str(), JS_EVAL_TYPE_GLOBAL);
    Result made;
    if (JS_IsException(result)) {
        made.problem = caught(context_, name);
    } else {
        made.value = shown(context_, result);
    }
    JS_FreeValue(context_, result);
    return made;
}

/**
 **/
void Runtime::limit(std::chrono::milliseconds time) noexcept {
    limit_ = time;
}

/**
 **/
int Runtime::interrupted(JSRuntime* runtime, void* opaque) {
    static_cast<void>(runtime);
    const auto* running = static_cast<const Runtime*>(opaque);
    return std::chrono::steady_clock::now() > running->deadline_ ? 1 : 0;
}

};  // namespace v3d::script
