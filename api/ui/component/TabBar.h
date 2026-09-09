/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/type/Bound2D.h>
#include <api/ui/Component.h>

#include <vector>

#include <boost/shared_ptr.hpp>

#include "TabPage.h"

namespace v3d::ui::component {

/**
 * A strip of tabs over the page one of them chose.
 *
 * The pages are the bar's children and one of them is chosen, so the bar owns which page
 * is up the way a select list owns which row is chosen. Only the chosen page is drawn,
 * and only its children are laid out - a page that is not up has no box, and nothing in
 * it can be picked.
 *
 * Where each tab ended up is written by whatever drew the strip, so a bar that has never
 * been drawn cannot say which tab a point is on, per ADR-0019.
 *
 * The strip, the tabs and the rule under them are the "tabs" style class the component
 * names, per ADR-0020.
 **/
class TabBar : public Component {
 public:
    /**
     * What selected() answers when the bar holds no pages.
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
     * @return the page that is up, or null when the bar holds none
     **/
    boost::shared_ptr<TabPage> page() const;

    /**
     * Where each tab was drawn, in the order the pages are held. Written by the draw, and
     * what at() answers a point with.
     **/
    void tabs(const std::vector<v3d::type::Bound2D>& boxes);
    const std::vector<v3d::type::Bound2D>& tabs() const noexcept;

    /**
     * @return the tab a point is on, or none when it is on the strip's empty part or
     *      outside it
     **/
    int at(const glm::vec2& point) const;

 private:
    std::vector<v3d::type::Bound2D> tabs_;
    int selected_;
};

};  // namespace v3d::ui::component
