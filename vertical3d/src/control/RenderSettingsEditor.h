/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/Immediate.h>
#include <vertical3d/src/document/Document.h>
#include <vertical3d/src/scene/RenderSettings.h>

#include <cstddef>
#include <functional>
#include <map>
#include <string_view>

#include <boost/shared_ptr.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor::control {

/**
 * The render settings: an editor of the document's width, height, samples and hider.
 *
 * Each number is dragged or typed, and a change is recorded when its drag or its typing ends. A
 * number that is not a whole one within its range is refused, and its field shows the document's
 * again. A choice of hider is recorded as it is made. Every change is one change in the history.
 **/
class RenderSettingsEditor final {
 public:
    /**
     * The kind of a render settings editor.
     **/
    static constexpr std::string_view KIND = "render-settings";

    /**
     * What is told what an edit the editor asked for did.
     **/
    using Report = std::function<void(const document::Outcome& outcome)>;

    RenderSettingsEditor(const boost::shared_ptr<document::Document>& document, const Report& report);

    /**
     * Draw one render settings editor into a region of the window, and act on what it is asked.
     *
     * @param editor the editor's id, which keeps its fields apart from another's
     **/
    void draw(v3d::ui::Immediate* ui, std::size_t editor, const glm::vec4& region);

 private:
    /**
     * What one editor keeps between frames: the numbers its fields show while one is being
     * changed.
     **/
    struct State final {
        float width = 0.0F;
        float height = 0.0F;
        float samples = 0.0F;
        bool editing = false;  /**< whether a number is being changed **/
    };

    /**
     * How far a pixel of drag moves a side and the samples.
     **/
    static constexpr float SIDE_SPEED = 1.0F;
    static constexpr float SAMPLE_SPEED = 0.1F;

    /**
     * Give the document the numbers a state holds, or say why they cannot be taken.
     **/
    void take(const State& state) const;

    /**
     * Give the document the settings, and say what that did.
     **/
    void apply(const scene::RenderSettings& settings) const;

    boost::shared_ptr<document::Document> document_;
    Report report_;
    std::map<std::size_t, State> states_;
};

};  // namespace v3d::editor::control
