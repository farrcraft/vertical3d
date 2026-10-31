/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vertical3d/src/app/CommandTable.h>
#include <vertical3d/src/app/Context.h>

namespace v3d::editor {

/**
 * The timeline's playback commands: play and stop, a step either way, the first and last
 * frames, and the selection's keys. Playback carries each one out.
 **/
class PlaybackCommands final {
 public:
    explicit PlaybackCommands(const Context& context);

    /**
     * Register every command.
     **/
    void registerCommands(CommandTable* table) const;

 private:
    const Context& context_;
};

};  // namespace v3d::editor
