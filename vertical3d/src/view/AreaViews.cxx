/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "AreaViews.h"

#include <cstddef>
#include <map>
#include <set>
#include <vector>

#include "AreaLayout.h"
#include "ViewPort.h"

namespace v3d::editor {

/**
 **/
AreaViews::AreaViews(const boost::shared_ptr<AreaLayout>& layout, const Make& make) :
    layout_(layout),
    make_(make) {
}

/**
 **/
bool AreaViews::sync() {
    bool made = true;
    std::set<std::size_t> held;
    for (const AreaLayout::Area& area : layout_->areas()) {
        for (const AreaLayout::Editor& editor : area.editors) {
            if (editor.kind != KIND) {
                continue;
            }
            held.insert(editor.id);
            if (views_.contains(editor.id)) {
                continue;
            }
            const boost::shared_ptr<ViewPort> view = make_ ? make_(editor) : nullptr;
            if (!view) {
                made = false;
                continue;
            }
            views_[editor.id] = view;
        }
    }
    std::erase_if(views_, [&held](const auto& entry) { return !held.contains(entry.first); });

    shown_.clear();
    for (const AreaLayout::Area& area : layout_->areas()) {
        const auto found = views_.find(area.editor().id);
        // an area hidden behind a maximised one has no room to draw in
        if (found == views_.end() || area.content.z <= 0.0F || area.content.w <= 0.0F) {
            continue;
        }
        found->second->resize(area.content);
        shown_.push_back(found->second);
    }
    return made;
}

/**
 **/
const std::vector<boost::shared_ptr<ViewPort>>& AreaViews::shown() const noexcept {
    return shown_;
}

/**
 **/
boost::shared_ptr<ViewPort> AreaViews::of(std::size_t area) const {
    if (area >= layout_->areas().size()) {
        return nullptr;
    }
    return editor(layout_->areas()[area].editor().id);
}

/**
 **/
boost::shared_ptr<ViewPort> AreaViews::editor(std::size_t id) const {
    const auto found = views_.find(id);
    return found == views_.end() ? nullptr : found->second;
}

/**
 **/
const boost::shared_ptr<AreaLayout>& AreaViews::layout() const noexcept {
    return layout_;
}

};  // namespace v3d::editor
