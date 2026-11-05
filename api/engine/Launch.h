/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace v3d::engine {

/**
 * What an app's command line asks of the engine, and the arguments the engine leaves to the app.
 *
 * argv[0] gives the program's name. The engine reads three options. `--version` prints the app's
 * name and version, and the app does not start. `--capture <file>` writes one presented frame to
 * a png file and then quits. `--capture-after <frames>` names that frame, counted from 1 at the
 * first frame the engine renders. Every other argument is kept in rest, in order, for the app to
 * read.
 **/
struct Launch final {
    /** The frame written when the command line names none. **/
    static constexpr unsigned int CAPTURE_AFTER = 60;

    /** Whether the command line asks only for the app's version. **/
    bool version{ false };
    /** The png file a frame is written to, or empty when no capture is asked for. **/
    std::string capture{};
    /** The frame written to capture, counted from 1. **/
    unsigned int captureAfter { CAPTURE_AFTER };
    /**
     * The name the executable was started as, without its directory or extension. It names the
     * directory under data/ that holds the app's own assets. Empty when no command line was
     * read, and the engine then reads data/ alone.
     **/
    std::string program{};
    /** The arguments the engine does not read, in the order given. **/
    std::vector<std::string> rest{};

    /**
     * Read a command line.
     *
     * @param argc the number of arguments, argv[0] included
     * @param argv the arguments. argv[0] is the executable, and gives only the program's name
     * @return what the command line asked for
     * @throws std::invalid_argument when `--capture` has no file after it, or `--capture-after`
     *         is not followed by a whole number from 1 up
     **/
    static Launch parse(int argc, const char* const* argv);

    /**
     * @param frame a frame's number, counted from 1
     * @return whether that frame is the one to capture
     **/
    bool due(unsigned int frame) const noexcept;

    /**
     * The value given after an option the app reads, such as the editor's `--script`.
     *
     * @param name the option, with its dashes
     * @return the argument after the first name in rest, or nothing when name is not there or is
     *         the last argument
     **/
    std::optional<std::string> option(std::string_view name) const;
};

};  // namespace v3d::engine
