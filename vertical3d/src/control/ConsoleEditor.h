/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/Immediate.h>
#include <api/ui/paint/Dressing.h>
#include <vertical3d/src/script/Journal.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor::control {

/**
 * The console: an editor that lists what the editor ran and what it said, above a line that
 * takes a script.
 *
 * The list is the journal, newest at the foot. A line typed is set in the active text after a
 * >, a command run in the secondary text, a refusal and a problem in red, and anything else in
 * the ordinary text. An entry over lines takes a row for each. The wheel scrolls back through
 * it, and an entry added scrolls it to the foot again.
 *
 * Return runs the line, and keeps the keyboard for the next one. The up and down arrows step
 * through the lines run before, and back to the line being written. Every console in the
 * editor shares those lines, and each keeps its own line being written.
 **/
class ConsoleEditor final {
 public:
    /**
     * The kind of a console.
     **/
    static constexpr std::string_view KIND = "console";

    /**
     * What runs a line.
     **/
    using Run = std::function<void(const std::string& line)>;

    ConsoleEditor(const boost::shared_ptr<script::Journal>& journal, const Run& run);

    /**
     * Draw one console into an area's region.
     * @param editor the console's id, which keeps its own line and scroll
     **/
    void draw(v3d::ui::Immediate* ui, std::size_t editor, const glm::vec4& region);

    /**
     * @return whether a console's line holds the keyboard, as it was last drawn
     **/
    bool typing() const noexcept;

    /**
     * Step through the lines run before, in the console whose line holds the keyboard: -1 to an
     * earlier one, 1 to a later one, and past the latest back to the line being written.
     **/
    void recall(int step);

    /**
     * @return the line a console is writing
     **/
    std::string line(std::size_t editor) const;

 private:
    /**
     * What one console keeps.
     **/
    struct State final {
        std::string line;
        std::string draft;          /**< the line being written, while an earlier one is recalled **/
        std::optional<std::size_t> recalled;  /**< which line run before is shown, if one is **/
        bool keep = false;          /**< the line takes the keyboard when it is next drawn **/
        float back = 0.0F;          /**< how many rows the list is scrolled back from its foot **/
        std::uint64_t seen = 0;     /**< the journal's revision when the console was last drawn **/
    };

    /**
     * One row of the list: its text and the colour it is set in.
     **/
    struct Row final {
        std::string text;
        glm::vec4 colour;
    };

    /**
     * @return the journal's entries as rows, oldest first
     **/
    std::vector<Row> rows(const v3d::ui::paint::Dressing& dressing) const;

    /**
     * Draw the list into the room above the line.
     **/
    void list(v3d::ui::Immediate* ui, State* state, float height) const;

    /**
     * Run a console's line, and keep the keyboard for the next.
     **/
    void enter(State* state);

    boost::shared_ptr<script::Journal> journal_;
    Run run_;
    // the lines run, oldest first, which every console steps through
    std::vector<std::string> history_;
    std::map<std::size_t, State> states_;
    // the console whose line held the keyboard when it was last drawn
    std::optional<std::size_t> typing_;
};

};  // namespace v3d::editor::control
