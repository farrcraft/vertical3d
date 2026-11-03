/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/script/Runtime.h>
#include <vertical3d/src/app/CommandTable.h>

#include <string>

namespace v3d::editor::app {

/**
 * Runs scripts and console lines against the editor's commands.
 *
 * Each command a script can run is a function of the runtime, by its path, which takes one
 * object of arguments. A call runs the command as CommandTable::run() does, and a command
 * that refuses, or arguments that do not suit it, raise an Error in the script with the
 * refusal's text. console.log lists what it is given in the journal.
 **/
class Scripting final {
 public:
    /**
     * @param table the commands, which every command a script can run is registered in before
     *        bind() is called
     **/
    explicit Scripting(CommandTable* table);

    /**
     * Bind every command a script can run, and console.log.
     **/
    void bind();

    /**
     * Run a line typed into the console. The journal lists the line, the commands it ran and
     * what they said, then its value, or the problem it stopped on with where it arose.
     **/
    void line(const std::string& source);

    /**
     * @return the declarations of the commands a script can run, as data/scripts/editor.d.ts
     *         holds them
     **/
    std::string declarations() const;

 private:
    CommandTable* table_;
    v3d::script::Runtime runtime_;
};

};  // namespace v3d::editor::app
