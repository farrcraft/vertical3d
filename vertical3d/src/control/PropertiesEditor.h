/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/Immediate.h>
#include <vertical3d/src/command/Edit.h>
#include <vertical3d/src/command/Stack.h>
#include <vertical3d/src/scene/item/Item.h>
#include <vertical3d/src/scene/Properties.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/scene/Units.h>

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
 * the edit ends, or its item stops being the one shown, it is recorded in the history as one
 * change, which undo takes back whole. While auto key is on, the numbers it changed are keyed at
 * the current frame in the same change. Otherwise an edit to a keyed number is pending until the
 * frame is set, as a manipulator's is. A lens, an emission or a style an edit would make invalid
 * is not written.
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
     * @return what a property path's value is shown with. A distance takes the scene's unit of
     *         length, a lens's angle or a rotation a degree sign, and a light's cone "rad". A
     *         scale, an intensity or a colour takes nothing
     **/
    static std::string unit(const std::string& path, scene::Length length);

    /**
     * Draw one properties editor into a region of the window, and write and record what it
     * edits.
     *
     * With nothing selected it shows the scene's own settings: its unit of length. With one
     * item selected it shows tabs. Item holds the item's name and every number and setting it
     * has. Mesh, for a mesh, counts its vertices and faces. Keys lists the item's curves. A group
     * of numbers along x, y and z is one row of three, each led by its axis's letter in the axis's
     * colour.
     *
     * @param editor the editor's id, which keeps its widgets apart from another's
     **/
    void draw(v3d::ui::Immediate* ui, const boost::shared_ptr<scene::Scene>& scene, std::size_t editor, const glm::vec4& region);

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
     * @return whether values can be written into an item: a camera's lens, a light's emission and a
     *         curve item's style are each valid, and the other kinds' are not written
     **/
    static bool valid(const scene::item::Item& item, const command::Edit::Values& values);

    /**
     * An edit that has begun and not ended: the item, and its values when it began.
     **/
    struct Pending final {
        boost::shared_ptr<scene::Scene> scene;  /**< the scene the item is in, where auto key sets its key **/
        boost::shared_ptr<scene::item::Item> item;
        command::Edit::Values before;
        std::string what;   /**< the name of the change, such as "rename" **/
    };

    /**
     * Draw the numbers a curve can drive, and write what is dragged or typed.
     **/
    void numbers(v3d::ui::Immediate* ui, const boost::shared_ptr<scene::item::Item>& item, const scene::Pose& shown, scene::Length length);

    /**
     * Draw one number, and write it into the item when it is dragged or typed.
     *
     * @param now every number as shown, with the rotation's angles as they were last shown
     **/
    void number(v3d::ui::Immediate* ui, const boost::shared_ptr<scene::item::Item>& item, const scene::Pose& shown,
        const std::map<std::string, float>& now, const std::string& path, const v3d::ui::Immediate::NumberLook& look);

    /**
     * Draw the scene's own settings, and record a change to them.
     **/
    void sceneSettings(v3d::ui::Immediate* ui, const boost::shared_ptr<scene::Scene>& scene);

    /**
     * Draw a mesh's counts of vertices and faces.
     **/
    static void meshCounts(v3d::ui::Immediate* ui, const scene::item::Item& item);

    /**
     * Draw each curve an item has: the property it drives and how many keys it holds.
     **/
    static void keys(v3d::ui::Immediate* ui, const scene::Scene& scene, const scene::item::Item& item);

    /**
     * Draw what a kind adds that is not a number: a camera's projection, a light's type, and a
     * curve item's style.
     **/
    void settings(v3d::ui::Immediate* ui, const boost::shared_ptr<scene::item::Item>& item);

    /**
     * Remember an item's values before the first change of an edit, and what the change is
     * called. An edit of another item that has not ended is recorded first.
     **/
    void begin(const boost::shared_ptr<scene::item::Item>& item, const std::string& what);

    /**
     * Record the edit that has ended, if it changed anything.
     **/
    void finish();

    boost::shared_ptr<command::Stack> history_;
    // the scene last drawn, which a new edit is made in
    boost::shared_ptr<scene::Scene> scene_;
    std::optional<Pending> pending_;
    // the item the angles were last shown for, and the angles
    boost::shared_ptr<scene::item::Item> turned_;
    glm::vec3 angles_{ 0.0F };
};

};  // namespace v3d::editor::control
