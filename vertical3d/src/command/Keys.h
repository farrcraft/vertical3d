/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/animation/Curve.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/scene/Timeline.h>

#include <cstddef>
#include <string>
#include <vector>

#include "Command.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor::command {

/**
 * Keys already set, moved along the frames or deleted.
 *
 * A key here is a FrameKey: every key one item has on one frame, across all its curves. A move
 * keeps each key's value, interpolation and slopes, and a moved key replaces one it lands on.
 * A curve left with no keys is removed.
 *
 * Everything is worked out when the command is made, from the scene as it stands then. Undo and
 * redo put each curve back as it was or as the change left it.
 **/
class Keys final : public Command {
 public:
    /**
     * Move keys along by an offset in frames.
     * @param keys what to move. A key no curve of its item has is skipped
     **/
    static boost::shared_ptr<Keys> shift(const boost::shared_ptr<scene::Scene>& scene, const std::vector<scene::FrameKey>& keys, float offset);

    /**
     * Delete keys.
     * @param keys what to delete. A key no curve of its item has is skipped
     **/
    static boost::shared_ptr<Keys> remove(const boost::shared_ptr<scene::Scene>& scene, const std::vector<scene::FrameKey>& keys);

    void undo() override;
    void redo() override;
    std::string name() const override;

    /**
     * @return how many of the keys asked for were found. The change acts on those
     **/
    std::size_t count() const noexcept;

    /**
     * @return whether there is anything to change. A command with nothing does nothing
     **/
    bool empty() const noexcept;

 private:
    /** One curve the change touches, before and after. An empty curve is no curve. **/
    struct Change {
        boost::shared_ptr<scene::Item> item;
        std::string path;
        v3d::animation::Curve before;
        v3d::animation::Curve after;
    };

    /**
     * What to do to one curve: its keys on the frames given are moved or deleted.
     * @return whether the curve changed
     **/
    using Edit = bool (*)(v3d::animation::Curve* curve, const std::vector<float>& frames, float offset);

    /**
     * Work out the change an edit makes to every curve of the keys' items.
     **/
    Keys(const boost::shared_ptr<scene::Scene>& scene, const std::vector<scene::FrameKey>& keys, const Edit& edit, float offset);

    /**
     * Give every changed curve the curve one end of the change has.
     **/
    void apply(bool after);

    boost::shared_ptr<scene::Scene> scene_;
    std::vector<Change> changes_;
    std::size_t count_ = 0;
    std::string what_;
};

};  // namespace v3d::editor::command
