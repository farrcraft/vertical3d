/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/Image.h>
#include <api/ui/Immediate.h>
#include <vertical3d/src/document/Document.h>
#include <vertical3d/src/scene/item/Item.h>
#include <vertical3d/src/scene/Timeline.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Playback.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor::control {

/**
 * The timeline: an editor that plays and steps through the frames, edits the frame, the range
 * and the rate, and shows the selected items' keys along the frames.
 *
 * Its controls are one row, which goes on to a second where the area is too narrow. Five icon
 * buttons go to the first frame, the previous key, play or
 * stop, the next key and the last frame, each with a tooltip that names what it does and its key.
 * A field for the frame, a pair for the first and last frames of the range, a menu of rates and a
 * check for auto key follow them. The ruler labels the range's first frame, and every multiple of
 * its step that has room after it. A change to the range is recorded when its drag or its typing
 * ends, and a change of rate when it is chosen.
 *
 * Under a ruler of frame numbers, each selected item has a row with its keys as diamonds, the
 * chosen ones in the selection's amber. A key is every key the item has on one frame. The
 * playhead is drawn across the ruler and the rows in the accent.
 *
 * A press on the ruler, or on a row away from its keys, chooses no key and moves the frame to
 * where it is, and a drag scrubs. A press on a key chooses it, unless it is chosen already, and
 * a drag moves every chosen key along by whole frames when it is let go. The chosen keys are
 * kept while their items are selected.
 **/
class TimelineEditor final {
 public:
    /**
     * The kind of a timeline editor.
     **/
    static constexpr std::string_view KIND = "timeline";

    /**
     * What is told what an edit the timeline asked for did.
     **/
    using Report = std::function<void(const document::Outcome& outcome)>;

    /**
     * Where frames lie along a track. The range's first frame is centred half a frame in from
     * the left, and its last half a frame in from the right.
     **/
    struct Span final {
        float left = 0.0F;
        float right = 0.0F;
        int start = 1;
        int end = 1;

        /**
         * @return how far along a frame is
         **/
        float at(float frame) const;

        /**
         * @return the frame a point along the track is at, which need not be whole
         **/
        float frame(float x) const;
    };

    TimelineEditor(const boost::shared_ptr<document::Document>& document, const boost::shared_ptr<Playback>& playback, const Report& report);

    /**
     * What gives the image an icon's source resolves to.
     **/
    using Icons = std::function<v3d::ui::Image(const std::string& source)>;

    /**
     * Resolve the transport's icons through this. With none, its buttons are drawn blank.
     **/
    void icons(const Icons& resolve);

    /**
     * What gives the key bound to a command, or nothing when none is.
     **/
    using Shortcuts = std::function<std::string(const std::string& command)>;

    /**
     * Find the keys the transport's tooltips name through this. With none, they name none.
     **/
    void shortcuts(const Shortcuts& find);

    /**
     * What a button of the transport does.
     **/
    enum class Action : std::uint8_t { First, PreviousKey, Play, Stop, NextKey, Last };

    /**
     * One button of the transport: what it does, its id, its icon, its tooltip and the command
     * whose key the tooltip names.
     **/
    struct Control final {
        Action action = Action::First;
        const char* id = "";
        const char* icon = "";
        const char* tooltip = "";
        const char* command = "";
    };

    /**
     * @return the transport's buttons, in order: first frame, previous key, play or stop, next
     *         key and last frame. Play is stop while the scene plays
     **/
    static std::vector<Control> controls(bool playing);

    /**
     * @return how the rate menu names a rate, such as "24 fps" or "29.97 fps"
     **/
    static std::string rateName(float rate);

    /**
     * Draw one timeline editor into a region of the window, and act on what it is asked.
     *
     * @param editor the editor's id, which keeps its fields and its drag apart from another's
     **/
    void draw(v3d::ui::Immediate* ui, std::size_t editor, const glm::vec4& region);

    /**
     * @return the chosen keys
     **/
    const std::vector<scene::FrameKey>& chosen() const noexcept;

    /**
     * Delete the chosen keys, and choose none.
     **/
    void deleteChosen();

 private:
    /**
     * The selection's amber, as the views draw selected geometry.
     **/
    static constexpr glm::vec4 CHOSEN{ 0.949F, 0.639F, 0.227F, 1.0F };

    /**
     * A key's half width, and how far past it a press still lands on it, against the line height.
     **/
    static constexpr float KEY = 0.3F;
    static constexpr float KEY_SLACK = 0.15F;

    /**
     * The check that turns auto key on and off, and the labels of the frame, the range's two
     * ends and the rate.
     **/
    static constexpr const char* AUTO_KEY = "Auto Key";
    static constexpr const char* FRAME = "Frame";
    static constexpr const char* RANGE = "Range";
    static constexpr const char* RANGE_END = "to";
    static constexpr const char* RATE = "Rate";

    /**
     * The widest frame number a field of the frame or the range is sized for, with room for its
     * last figure.
     **/
    static constexpr const char* FIGURES = "000000";

    /**
     * The rates the menu offers.
     **/
    static constexpr std::array<float, 10> RATES = { 12.0F, 15.0F, 23.976F, 24.0F, 25.0F, 29.97F, 30.0F, 48.0F, 50.0F, 60.0F };

    /**
     * How far down the ruler a frame's tick starts, against the line height.
     **/
    static constexpr float TICK = 0.6F;

    /**
     * How wide the column of names before the tracks is, against the line height.
     **/
    static constexpr float NAMES = 6.0F;

    /**
     * How far a pixel of drag moves the range and the rate.
     **/
    static constexpr float FRAME_SPEED = 0.2F;

    /**
     * What a press on a track is doing while it is held.
     **/
    enum class Drag : std::uint8_t { None, Scrub, Move };

    /**
     * What one timeline editor keeps between frames: the range its fields show while one is
     * being changed, and its drag.
     **/
    struct State final {
        float start = 0.0F;
        float end = 0.0F;
        bool editing = false;  /**< whether a field of the range is being changed **/
        Drag drag = Drag::None;
        int offset = 0;  /**< how far a drag has moved the chosen keys so far **/
    };

    /**
     * How far along its row the controls have reached, and how wide the row is.
     **/
    struct Flow final {
        float room = 0.0F;
        float used = 0.0F;
    };

    /**
     * Place the next control of the row beside the last, or at the start of a new row where it
     * would pass the right edge.
     *
     * @param width how wide the control is
     * @param sized whether the control takes its width from the layer, as a field does, rather
     *        than sizing itself
     * @param after how much more has to fit beside it, so that two controls that belong
     *        together, such as the range's ends, stay on one row
     **/
    static void place(v3d::ui::Immediate* ui, Flow* flow, float width, bool sized, float after = 0.0F);

    /**
     * Draw the buttons that play and step, and act on a click.
     **/
    void transport(v3d::ui::Immediate* ui, Flow* flow);

    /**
     * Draw the fields of the frame and the range, the menu of rates and the check for auto key,
     * after the transport.
     **/
    void fields(v3d::ui::Immediate* ui, State* state, Flow* flow);

    /**
     * Draw the menu of rates, and set the rate chosen.
     **/
    void rates(v3d::ui::Immediate* ui, Flow* flow);

    /**
     * Draw the ruler of frame numbers, and scrub on a press.
     * @return the ruler's surface
     **/
    v3d::ui::Immediate::Surface ruler(v3d::ui::Immediate* ui, State* state);

    /**
     * Draw one item's row of keys, and act on a press on it.
     * @return the row's surface
     **/
    v3d::ui::Immediate::Surface row(v3d::ui::Immediate* ui, State* state, const boost::shared_ptr<scene::item::Item>& item);

    /**
     * Take a press on an item's row: choose the key it landed on and start moving the chosen
     * keys, or choose none and start scrubbing.
     * @param hit the frame of the key the press landed on, or nothing
     **/
    void press(unsigned int item, const std::optional<float>& hit, State* state);

    /**
     * Take a held press along a track: scrub, or measure how far the chosen keys are moved, and
     * move them when it is let go.
     **/
    void follow(const v3d::ui::Immediate::Surface& surface, const Span& span, State* state);

    /**
     * Set the frame, unless it is there already, since setting it drops the pending edits.
     **/
    void goTo(int frame) const;

    /**
     * @return the span of a track: from after the names to the surface's right edge, each end
     *         in by a key's half width so a key at either end of the range is drawn whole
     * @param line the height of a line, which the names and the keys are sized against
     **/
    Span span(const v3d::ui::Immediate::Surface& surface, float line) const;

    /**
     * @return whether a key is chosen
     **/
    bool isChosen(const scene::FrameKey& key) const;

    /**
     * Forget the chosen keys of items no longer selected, and of frames no longer keyed.
     **/
    void prune();

    boost::shared_ptr<document::Document> document_;
    boost::shared_ptr<Playback> playback_;
    Report report_;
    std::vector<scene::FrameKey> chosen_;
    std::map<std::size_t, State> states_;
    Icons icons_;
    Shortcuts shortcuts_;
};

};  // namespace v3d::editor::control
