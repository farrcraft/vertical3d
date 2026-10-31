/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/Immediate.h>
#include <vertical3d/src/command/Edit.h>
#include <vertical3d/src/command/Stack.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Properties.h>
#include <vertical3d/src/scene/Scene.h>

#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <string_view>

#include <boost/shared_ptr.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor::control {

/**
 * The properties editor: an editor that shows the selected item's name, placement and settings,
 * and edits them.
 *
 * It shows one item, when exactly one is selected, and otherwise says how many are. Each number
 * a curve can drive is shown as the current frame shows it, so a keyed number follows playback.
 * A camera adds its projection and a light its type. A curve item adds whether a render draws
 * it, and its width.
 *
 * An edit is written into the item as it happens, so the views follow a drag of a number. When
 * the edit ends it is recorded in the history as one change, which undo takes back whole. An
 * edit to a keyed number is pending until the frame is set, as a manipulator's is. A lens, an
 * emission or a style an edit would make invalid is not written.
 **/
class PropertiesEditor final {
 public:
    /**
     * The kind of a properties editor.
     **/
    static constexpr std::string_view KIND = "properties";

    /**
     * What a number is called in the editor, and how far a drag of one pixel moves it.
     **/
    struct Field final {
        std::string group;   /**< the heading it is shown under, such as "Translation" **/
        std::string label;   /**< what it is called under the heading, such as "X" **/
        float speed = 0.0F;
    };

    /**
     * @return how a property path is shown and dragged
     **/
    static Field field(const std::string& path);

    /**
     * @param history where an edit is recorded when it ends
     **/
    explicit PropertiesEditor(const boost::shared_ptr<command::Stack>& history);

    /**
     * Draw one properties editor into a region of the window, and write and record what it
     * edits.
     *
     * @param editor the editor's id, which keeps its widgets apart from another's
     **/
    void draw(v3d::ui::Immediate* ui, scene::Scene* scene, std::size_t editor, const glm::vec4& region);

 private:
    /**
     * How far a pixel of drag moves a number with no speed of its own, and a curve item's width.
     **/
    static constexpr float DEFAULT_SPEED = 0.01F;
    static constexpr float WIDTH_SPEED = 0.001F;

    /**
     * How each property path is shown, and how far a pixel of drag moves it. A placement moves a
     * hundredth of a unit, and an angle half a degree.
     **/
    static const std::map<std::string, Field>& fields();

    /**
     * @return a heading in lower case, as the name of the change an edit under it makes
     **/
    static std::string lower(const std::string& text);

    /**
     * @return whether values can be written into an item: a camera's lens, a light's emission and a
     *         curve item's style are each valid, and the other kinds' are not written
     **/
    static bool valid(const scene::Item& item, const command::Edit::Values& values);

    /**
     * An edit that has begun and not ended: the item, and its values when it began.
     **/
    struct Pending final {
        boost::shared_ptr<scene::Item> item;
        command::Edit::Values before;
    };

    /**
     * Draw the numbers a curve can drive, and write what is dragged or typed.
     **/
    void numbers(v3d::ui::Immediate* ui, const boost::shared_ptr<scene::Item>& item, const scene::Pose& shown);

    /**
     * Draw what a kind adds that is not a number: a camera's projection, a light's type, and a
     * curve item's style.
     **/
    void settings(v3d::ui::Immediate* ui, const boost::shared_ptr<scene::Item>& item);

    /**
     * Remember an item's values before the first change of an edit. An edit of another item
     * that has not ended is recorded first.
     **/
    void begin(const boost::shared_ptr<scene::Item>& item);

    /**
     * Record the edit that has ended, if it changed anything.
     **/
    void finish(const std::string& what);

    boost::shared_ptr<command::Stack> history_;
    std::optional<Pending> pending_;
    // the item the angles were last shown for, and the angles
    boost::shared_ptr<scene::Item> turned_;
    glm::vec3 angles_{ 0.0F };
};

};  // namespace v3d::editor::control
