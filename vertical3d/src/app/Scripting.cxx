/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Scripting.h"

#include <api/engine/Launch.h>
#include <api/log/Logger.h>
#include <api/script/Runtime.h>
#include <api/script/Strip.h>
#include <vertical3d/src/app/CommandTable.h>
#include <vertical3d/src/script/Declarations.h>
#include <vertical3d/src/script/Journal.h>
#include <vertical3d/src/script/Text.h>

#include <algorithm>
#include <fstream>
#include <ios>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <boost/json.hpp>

namespace v3d::editor::app {

/**
 **/
Scripting::Scripting(CommandTable* table) :
    table_(table) {
}

/**
 **/
void Scripting::bind() {
    CommandTable* table = table_;
    for (const auto& [name, parameters] : table->scripted()) {
        const std::string command = name;
        runtime_.bind(script::path(command), [table, command](const boost::json::array& arguments) {
            boost::json::object given;
            if (arguments.size() > 1 || (!arguments.empty() && !arguments.front().is_object() && !arguments.front().is_null())) {
                throw v3d::script::Refused(script::path(command) + " takes one object of arguments");
            }
            if (!arguments.empty() && arguments.front().is_object()) {
                given = arguments.front().as_object();
            }
            if (const std::optional<std::string> problem = table->run(command, given)) {
                throw v3d::script::Refused(*problem);
            }
            return boost::json::value();
        });
    }
    runtime_.bind("console.log", [table](const boost::json::array& arguments) {
        std::string said;
        for (const boost::json::value& each : arguments) {
            said += (said.empty() ? "" : " ") + (each.is_string() ? std::string(each.as_string()) : boost::json::serialize(each));
        }
        table->journal()->add(script::Journal::Kind::Said, said);
        return boost::json::value();
    });
}

/**
 **/
void Scripting::line(const std::string& source) {
    const boost::shared_ptr<script::Journal>& journal = table_->journal();
    journal->add(script::Journal::Kind::Typed, source);
    const v3d::script::Result result = runtime_.run(source, "console");
    if (result.problem) {
        const v3d::script::Problem& problem = *result.problem;
        const std::string where = problem.line > 0 ? std::to_string(problem.line) + ":" + std::to_string(problem.column) + " " : "";
        journal->add(script::Journal::Kind::Failed, where + problem.message);
    } else if (!result.value.empty()) {
        journal->add(script::Journal::Kind::Said, result.value);
    }
}

/**
 **/
bool Scripting::startup(const v3d::engine::Launch& launch, const v3d::log::Logger& logger) {
    if (std::ranges::find(launch.rest, std::string_view(SCRIPT)) == launch.rest.end()) {
        return true;
    }
    const std::optional<std::string> file = launch.option(SCRIPT);
    if (!file) {
        logger.get()->error("{} needs the script file to run", SCRIPT);
        return false;
    }
    std::ifstream stream(*file, std::ios::binary);
    if (!stream) {
        logger.get()->error("The script {} cannot be read", *file);
        return false;
    }
    const std::string source{ std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>() };
    const v3d::script::Result result = runtime_.run(source, *file);
    if (result.problem) {
        logger.get()->error("The script {} stopped at {}:{}: {}", *file, result.problem->line, result.problem->column, result.problem->message);
        return false;
    }
    logger.get()->info("Ran the script {}", *file);
    return true;
}

/**
 **/
std::string Scripting::declarations() const {
    return script::declarations(table_->scripted());
}

};  // namespace v3d::editor::app
