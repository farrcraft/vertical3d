/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Scripting.h"

#include <api/script/Runtime.h>
#include <api/script/Strip.h>
#include <vertical3d/src/app/CommandTable.h>
#include <vertical3d/src/script/Declarations.h>
#include <vertical3d/src/script/Journal.h>
#include <vertical3d/src/script/Text.h>

#include <optional>
#include <string>
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
std::string Scripting::declarations() const {
    return script::declarations(table_->scripted());
}

};  // namespace v3d::editor::app
