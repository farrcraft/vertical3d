/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/animation/Curve.h>
#include <api/ui/Immediate.h>
#include <vertical3d/src/document/Document.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor::control {

/**
 * The graph editor: an editor that draws each curve of the selected items over a grid of frames
 * and values, and reshapes one key at a time.
 *
 * Above the graph, a ruler numbers the frames, and a press on it moves the frame there. The
 * playhead runs down the ruler and the graph at the current frame, in the accent. A curve whose
 * path ends in x, y or z is drawn in that axis's colour, and every other curve in the text's.
 *
 * A press on a key chooses it, and a drag moves it by whole frames and by any value. The chosen
 * key is drawn in the selection's amber with its handles, one toward each neighbour the curve is
 * a cubic to. A drag on a handle turns both of the key's slopes to point at it, and the key stops
 * being automatic. A move or a turn is one change, recorded when the drag is let go. A press away
 * from every key chooses none.
 *
 * The view moves as an orthographic viewport's does. While the truck move is held or latched, a
 * drag moves the view. While the zoom or dolly move is, a drag right shows fewer frames, and a
 * drag up fewer values, about where the drag began. The wheel shows less or more about the
 * cursor, a notch away from the reader showing less. Frame All fits the view to the selected
 * items' curves, as a new editor is fitted when it is first drawn.
 **/
class GraphEditor final {
 public:
    /**
     * The kind of a graph editor.
     **/
    static constexpr std::string_view KIND = "graph";

    /**
     * What is told what an edit the graph asked for did.
     **/
    using Report = std::function<void(const document::Outcome& outcome)>;

    /**
     * What a drag on the graph does to the view instead of to a key.
     **/
    enum class Navigate : std::uint8_t { None, Pan, Zoom };

    /**
     * One key of one curve.
     **/
    struct CurveKey final {
        unsigned int item = 0;  /**< the item's id in the scene **/
        std::string path;       /**< the property the curve drives **/
        float frame = 0.0F;

        bool operator==(const CurveKey& other) const = default;
    };

    /**
     * Where frames and values lie across the graph. Frames rise to the right and values rise up.
     **/
    struct View final {
        glm::vec2 min{ 0.0F };  /**< the graph's top left corner **/
        glm::vec2 max{ 0.0F };  /**< and its bottom right **/
        float frame = 0.0F;     /**< the frame at the left edge **/
        float value = 0.0F;     /**< the value at the bottom edge **/
        float frames = 1.0F;    /**< how many frames the width shows **/
        float values = 1.0F;    /**< how much value the height shows **/

        /**
         * @return where a frame and a value are drawn
         * @param place the frame in x and the value in y
         **/
        glm::vec2 at(const glm::vec2& place) const;

        /**
         * @return the frame and the value a point is at
         **/
        glm::vec2 point(const glm::vec2& at) const;
    };

    GraphEditor(const boost::shared_ptr<document::Document>& document, const Report& report);

    /**
     * Draw one graph editor into a region of the window, and act on what it is asked.
     *
     * @param editor the editor's id, which keeps its view and its drag apart from another's
     * @param navigate the camera move held or latched, which a drag on the graph performs
     **/
    void draw(v3d::ui::Immediate* ui, std::size_t editor, const glm::vec4& region, Navigate navigate);

    /**
     * @return where an editor last drew its frames and values, or nothing before it is drawn
     **/
    std::optional<View> view(std::size_t editor) const;

    /**
     * @return the chosen key, which every graph editor shares
     **/
    const std::optional<CurveKey>& chosen() const noexcept;

    /**
     * @return the colour a curve is drawn in: its axis's for a path ending in x, y or z, and the
     *         colour given for any other
     **/
    static glm::vec4 colour(const std::string& path, const glm::vec4& otherwise);

 private:
    /**
     * The selection's amber, as the views draw selected geometry.
     **/
    static constexpr glm::vec4 CHOSEN{ 0.949F, 0.639F, 0.227F, 1.0F };

    /**
     * A key's half width, a handle's, and how far from either a press still lands on it, against
     * the line height.
     **/
    static constexpr float KEY = 0.3F;
    static constexpr float HANDLE = 0.2F;
    static constexpr float REACH = 0.45F;

    /**
     * How much a notch of the wheel shows, and how fast a drag zooms, for each pixel.
     **/
    static constexpr float WHEEL_ZOOM = 1.25F;
    static constexpr float DRAG_ZOOM = 0.01F;

    /**
     * What a press on the graph or the ruler is doing while it is held.
     **/
    enum class Drag : std::uint8_t { None, Scrub, Pan, Zoom, Key, In, Out };

    /**
     * What one graph editor keeps between frames: its view, and its drag.
     **/
    struct State final {
        View view;
        bool fit = true;  /**< whether to fit the view to the curves when it is next drawn **/
        Drag drag = Drag::None;
        glm::vec2 last{ 0.0F };  /**< where the cursor was on the last frame of a pan or a zoom **/
        v3d::animation::Curve original;  /**< the chosen key's curve as it was when its drag began **/
        std::optional<v3d::animation::Curve> draft;  /**< and as the drag will leave it **/
        float landing = 0.0F;  /**< the frame a dragged key will land on **/
    };

    /**
     * Draw the ruler of frame numbers above the graph, and scrub on a press.
     **/
    void ruler(v3d::ui::Immediate* ui, const v3d::ui::Immediate::Surface& rule, State* state);

    /**
     * Draw each curve of the selected items, the chosen key's as its drag will leave it, and the
     * chosen key with its handles.
     **/
    void curves(v3d::ui::Immediate* ui, const State& state) const;

    /**
     * Draw the chosen key, where its drag will leave it, with its handles.
     **/
    void drawChosen(v3d::ui::Immediate* ui, const State& state) const;

    /**
     * Take a press on the graph: start moving the view, or take hold of a handle or a key.
     **/
    void press(const v3d::ui::Immediate::Surface& plot, float line, Navigate navigate, State* state);

    /**
     * Take hold of a handle of the chosen key, when the cursor is within reach of one.
     * @return whether one was taken
     **/
    bool grab(const glm::vec2& cursor, float reach, State* state) const;

    /**
     * Choose the key nearest the cursor within reach, and take hold of it, or choose none.
     **/
    void choose(const glm::vec2& cursor, float reach, State* state);

    /**
     * Take a held press on the graph: move the view, or reshape the chosen key's curve, and
     * record the reshaped curve when the press is let go.
     **/
    void follow(const v3d::ui::Immediate::Surface& plot, State* state);

    /**
     * Work out the chosen key's curve as the held key or handle leaves it.
     **/
    void reshape(const v3d::ui::Immediate::Surface& plot, State* state) const;

    /**
     * Record the reshaped curve, if the drag changed it, and end the drag.
     **/
    void commit(State* state);

    /**
     * Fit a view to the selected items' curves, or to the range when they have none.
     **/
    void fit(View* view) const;

    /**
     * @return the chosen key's curve, or nothing when no key is chosen
     **/
    std::optional<v3d::animation::Curve> chosenCurve() const;

    /**
     * Forget the chosen key when its item is no longer selected or its frame no longer keyed.
     **/
    void prune();

    boost::shared_ptr<document::Document> document_;
    Report report_;
    std::optional<CurveKey> chosen_;
    std::map<std::size_t, State> states_;
};

};  // namespace v3d::editor::control
