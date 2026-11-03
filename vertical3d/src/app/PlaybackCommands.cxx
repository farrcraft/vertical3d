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
    table->script("timeline::playback::play", [&context]() { context.playback->play(); });
    table->script("timeline::playback::stop", [&context]() { context.playback->stop(); });
    table->script("timeline::playback::next", [&context]() { context.playback->step(1); });
    table->script("timeline::playback::previous", [&context]() { context.playback->step(-1); });
    table->script("timeline::playback::first", [&context]() { context.playback->first(); });
    table->script("timeline::playback::last", [&context]() { context.playback->last(); });
    table->script("timeline::playback::next_key", [&context]() { context.playback->nextKey(context.document->scene()->selection(), true); });
    table->script("timeline::playback::previous_key", [&context]() {
        context.playback->nextKey(context.document->scene()->selection(), false);
    });
    table->script("timeline::playback::first_key", [&context]() { context.playback->firstKey(context.document->scene()->selection()); });
    table->script("timeline::playback::last_key", [&context]() { context.playback->lastKey(context.document->scene()->selection()); });
}

};  // namespace v3d::editor::app
