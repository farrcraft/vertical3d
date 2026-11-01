/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "PlaybackCommands.h"

#include <vertical3d/src/app/CommandTable.h>
#include <vertical3d/src/app/Context.h>
#include <vertical3d/src/scene/Scene.h>

namespace v3d::editor::app {

/**
 **/
PlaybackCommands::PlaybackCommands(const Context& context) :
    context_(context) {
}

/**
 **/
void PlaybackCommands::registerCommands(CommandTable* table) const {
    const Context& context = context_;
    table->press("timeline::playback::play", [&context]() { context.playback->play(); });
    table->press("timeline::playback::stop", [&context]() { context.playback->stop(); });
    table->press("timeline::playback::next", [&context]() { context.playback->step(1); });
    table->press("timeline::playback::previous", [&context]() { context.playback->step(-1); });
    table->press("timeline::playback::first", [&context]() { context.playback->first(); });
    table->press("timeline::playback::last", [&context]() { context.playback->last(); });
    table->press("timeline::playback::next_key", [&context]() { context.playback->nextKey(context.document->scene()->selection(), true); });
    table->press("timeline::playback::previous_key", [&context]() {
        context.playback->nextKey(context.document->scene()->selection(), false);
    });
    table->press("timeline::playback::first_key", [&context]() { context.playback->firstKey(context.document->scene()->selection()); });
    table->press("timeline::playback::last_key", [&context]() { context.playback->lastKey(context.document->scene()->selection()); });
}

};  // namespace v3d::editor::app
