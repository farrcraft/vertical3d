/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <boost/json.hpp>

namespace v3d::editor::script {

/**
 * What a parameter of a command takes. An integer is one an int holds, and a frame is one. An
 * item is named by its name, and a part by the text form Part describes.
 **/
enum class Type : std::uint8_t { Boolean, Number, Integer, Text, Item, Items, Parts };

/**
 * One argument a command takes from a script, by name. Every parameter may be left out, and the
 * command then fills it from the editor's state, as it does for a menu or a key.
 **/
struct Parameter final {
    std::string name;
    Type type = Type::Boolean;
};

/**
 * The arguments one run of a command had: those a script gave, and those the command filled
 * from the editor's state. The values are kept by name in the form a script writes them, so a
 * run can be listed as a line that runs it again.
 **/
class Arguments final {
 public:
    Arguments() = default;

    /**
     * @param given the values a script gave, which check() has passed
     **/
    explicit Arguments(boost::json::object given);

    /**
     * @return why the values given do not suit the parameters, or nothing when they do. A name
     *         no parameter has, and a value of the wrong type, are each refused
     **/
    static std::optional<std::string> check(const boost::json::object& given, const std::vector<Parameter>& parameters);

    /**
     * The value given for a parameter, or the one fill makes, which is kept as if given.
     **/
    bool boolean(std::string_view name, const std::function<bool()>& fill);
    double number(std::string_view name, const std::function<double()>& fill);
    int integer(std::string_view name, const std::function<int()>& fill);
    std::string text(std::string_view name, const std::function<std::string()>& fill);

    /**
     * The names given for a parameter of items or parts, or those fill makes.
     **/
    std::vector<std::string> names(std::string_view name, const std::function<std::vector<std::string>()>& fill);

    /**
     * @return whether a script gave the parameter, or a command has filled it
     **/
    bool has(std::string_view name) const;

    /**
     * @return every value given or filled, by name
     **/
    const boost::json::object& values() const noexcept;

 private:
    boost::json::object values_;
};

};  // namespace v3d::editor::script
