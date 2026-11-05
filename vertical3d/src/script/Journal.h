/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>

namespace v3d::editor::script {

/**
 * What the editor ran and what it said, in the order it happened, which the console lists.
 *
 * Each command a script can run is written as the line that runs it again, whichever of a
 * menu, a key or a script ran it. Each notice the editor posts follows the command that made
 * it. A line typed into the console comes before what it ran, and what it printed, came to or
 * stopped on comes after. The journal keeps a number of entries and drops the oldest past
 * that.
 **/
class Journal final {
 public:
    /**
     * What an entry records.
     **/
    enum class Kind : std::uint8_t {
        Command,    /**< a command run, as the line that runs it again **/
        Done,       /**< a notice that something was done **/
        Refused,    /**< a notice that something could not be done **/
        Typed,      /**< a line typed into the console **/
        Said,       /**< what a script printed, or the value a console line came to **/
        Failed,     /**< why a script stopped **/
    };

    struct Entry final {
        std::uint64_t number = 0;   /**< the entry's place among every entry ever made, from 1 **/
        Kind kind = Kind::Command;
        std::string text;
    };

    /**
     * How many entries the editor's journal keeps.
     **/
    static constexpr std::size_t CAPACITY = 1000;

    /**
     * @param capacity how many entries are kept, at least one
     **/
    explicit Journal(std::size_t capacity = CAPACITY);

    /**
     * Add an entry after every other.
     * @return its number
     **/
    std::uint64_t add(Kind kind, const std::string& text);

    /**
     * Write an entry's text again, as a command's is once it has filled every argument it ran
     * with. An entry no longer kept is not written.
     **/
    void rewrite(std::uint64_t number, const std::string& text);

    /**
     * @return the number of the last entry made, or 0 before any
     **/
    std::uint64_t last() const noexcept;

    /**
     * @return the entries kept, oldest first
     **/
    const std::deque<Entry>& entries() const noexcept;

    /**
     * @return a number that moves whenever an entry is added or written again
     **/
    std::uint64_t revision() const noexcept;

 private:
    std::size_t capacity_;
    std::deque<Entry> entries_;
    std::uint64_t made_ = 0;
    std::uint64_t revision_ = 0;
};

};  // namespace v3d::editor::script
