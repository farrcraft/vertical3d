/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>
#include <functional>
#include <map>
#include <string_view>
#include <vector>

#include "AreaLayout.h"
#include "ViewPort.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor::view {

/**
 * The viewports of an area layout: one for each viewport editor the layout holds, made when the
 * editor first appears and dropped when it goes.
 *
 * A viewport lasts as long as its editor, whether or not its area shows it, so a view keeps its
 * camera and its display while another tab is chosen. The views shown are the viewports their
 * areas show, each given where its area's editor draws, under any strip of tabs.
 **/
class AreaViews final {
 public:
    /**
     * The kind of a viewport editor.
     **/
    static constexpr std::string_view KIND = "viewport";

    /**
     * Makes the viewport for an editor, or returns null when its state cannot be shown.
     **/
    typedef std::function<boost::shared_ptr<ViewPort>(const AreaLayout::Editor& editor)> Make;

    AreaViews(const boost::shared_ptr<AreaLayout>& layout, const Make& make);

    /**
     * Make a viewport for each viewport editor that has none, drop those of editors that are
     * gone, and give each view shown where its area's editor draws. Call it after any change to
     * the layout.
     *
     * @return whether every viewport editor has a viewport
     **/
    bool sync();

    /**
     * @return the views shown, in the order of their areas
     **/
    const std::vector<boost::shared_ptr<ViewPort>>& shown() const noexcept;

    /**
     * @return the view an area shows, or null when it shows another kind of editor
     **/
    boost::shared_ptr<ViewPort> of(std::size_t area) const;

    /**
     * @return the view of an editor, or null when it has none
     **/
    boost::shared_ptr<ViewPort> editor(std::size_t id) const;

    const boost::shared_ptr<AreaLayout>& layout() const noexcept;

 private:
    boost::shared_ptr<AreaLayout> layout_;
    Make make_;
    // keyed by the editor's id
    std::map<std::size_t, boost::shared_ptr<ViewPort>> views_;
    std::vector<boost::shared_ptr<ViewPort>> shown_;
};

};  // namespace v3d::editor::view
