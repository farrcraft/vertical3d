/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/type/geometry/Bound2D.h>
#include <api/ui/Component.h>

#include <vector>

#include <boost/shared_ptr.hpp>

#include "TabPage.h"

namespace v3d::ui::component {

/**
 * A strip of tabs over the page one of them chose.
 *
 * The pages are the bar's children. The bar holds which page is chosen, the way a select
 * list holds which row is chosen. Only the chosen page is drawn, and only its children are
 * laid out. A page that is not chosen gets no new box, and nothing in it can be picked.
 *
 * The renderer records each tab's box through tabs() when it draws the strip. Until the
 * bar has been drawn, at() finds no tab.
 *
 * The strip, the tabs and the rule under them are the "tabs" style class the component
 * names.
 **/
class TabBar : public Component {
 public:
    /**
     * What selected() returns when no page is chosen: the bar holds none, or an index outside
     * them was chosen.
     **/
    static const int none = -1;

    TabBar();
    ~TabBar() = default;

    /**
     * @return the pages the bar holds, which are the children that are pages
     **/
    std::vector<boost::shared_ptr<TabPage>> pages() const;

    /**
     * Choose a page by index. An index outside the pages chooses none.
     **/
    void selected(int index);
    int selected() const noexcept;

    /**
     * @return the page that is up, or null when no page is chosen
     **/
    boost::shared_ptr<TabPage> page() const;

    /**
     * Where each tab was drawn, in the order the pages are held. The renderer writes these
     * as it draws, and at() tests a point against them.
     **/
    void tabs(const std::vector<v3d::type::geometry::Bound2D>& boxes);
    const std::vector<v3d::type::geometry::Bound2D>& tabs() const noexcept;

    /**
     * @return the tab a point is on, or none when it is on the strip's empty part or
     *      outside it
     **/
    int at(const glm::vec2& point) const;

 private:
    std::vector<v3d::type::geometry::Bound2D> tabs_;
    int selected_;
};

};  // namespace v3d::ui::component
