/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Outliner.h"

#include <api/ui/Image.h>
#include <api/ui/Immediate.h>
#include <vertical3d/src/control/Text.h>
#include <vertical3d/src/scene/item/Item.h>
#include <vertical3d/src/scene/Scene.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <set>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor::control {

/**
 **/
std::string Outliner::label(const scene::item::Item& item) {
    return item.name().empty() ? scene::item::name(item.kind()) : item.name();
}

/**
 **/
Outliner::Mark Outliner::mark(scene::item::Item::Kind kind) {
    switch (kind) {
        case scene::item::Item::Kind::Mesh:
            return { .icon = "icon:item-mesh", .colour = MESH };
        case scene::item::Item::Kind::Camera:
            return { .icon = "icon:item-camera", .colour = CAMERA };
        case scene::item::Item::Kind::Light:
            return { .icon = "icon:item-light", .colour = LIGHT };
        case scene::item::Item::Kind::Group:
            return { .icon = "icon:item-group", .colour = GROUP };
        case scene::item::Item::Kind::Curve:
            return { .icon = "icon:item-curve", .colour = CURVE };
    }
    return {};
}

/**
 **/
Outliner::Mark Outliner::sceneMark() {
    return { .icon = "icon:item-scene", .colour = GROUP };
}

/**
 **/
void Outliner::icons(const Icons& resolve) {
    icons_ = resolve;
}

/**
 **/
void Outliner::eye(const Eye& clicked) {
    eye_ = clicked;
}

/**
 **/
v3d::ui::Image Outliner::image(const Mark& mark) const {
    return icons_ ? icons_(mark.icon) : v3d::ui::Image();
}

/**
 **/
std::vector<Outliner::Row> Outliner::rows(const scene::Scene& scene, const std::string& filter, const std::set<std::int64_t>& folded) {
    const std::vector<std::string> wanted = words(filter);
    const Walk walk{ .scene = scene, .words = wanted, .folded = folded };
    std::vector<Row> made;
    for (const boost::shared_ptr<scene::item::Item>& item : scene.children(0)) {
        gather(walk, item, 0, &made);
    }
    return made;
}

/**
 **/
void Outliner::draw(v3d::ui::Immediate* ui, scene::Scene* scene, std::size_t editor, const glm::vec4& region, bool adding) {
    State& state = states_[editor];
    const std::string id = "outliner " + std::to_string(editor);
    ui->region(id, { .position = glm::vec2(region.x, region.y), .size = glm::vec2(region.z, region.w) }, 1.0F);
    // two outliners draw the same labels, so each draws under its own id
    ui->pushId(id);
    ui->textField("Filter", &state.filter,
        v3d::ui::Immediate::FieldLook{ .icon = icons_ ? icons_("icon:search") : v3d::ui::Image(), .labelled = false, .limit = 0 });
    ui->separator();
    // the Scene row folds every item and selects nothing
    bool sceneOpen = !state.sceneFolded;
    const Mark scened = sceneMark();
    ui->treeRow("Scene", { .depth = 0, .open = &sceneOpen, .selected = false, .icon = image(scened), .iconTint = scened.colour });
    state.sceneFolded = !sceneOpen;
    const std::vector<Row> shown = sceneOpen ? rows(*scene, state.filter, state.folded) : std::vector<Row>();
    for (const Row& row : shown) {
        ui->pushId(static_cast<int>(row.item->documentId()));
        bool open = row.open;
        const Mark marked = mark(row.item->kind());
        bool eyed = false;
        const v3d::ui::Image eye = image({ .icon = row.item->shown().views ? "icon:eye" : "icon:eye-closed", .colour = glm::vec4(1.0F) });
        const bool clicked = ui->treeRow(label(*row.item), { .depth = row.depth + 1, .open = row.parent ? &open : nullptr,
            .selected = row.item->selected(), .icon = image(marked), .iconTint = marked.colour,
            .dimmed = !scene->shown(row.item->id()).views, .end = eye, .endClicked = &eyed });
        if (eyed && eye_) {
            eye_(row.item);
        }
        if (open != row.open) {
            if (open) {
                state.folded.erase(row.item->documentId());
            } else {
                state.folded.insert(row.item->documentId());
            }
        }
        if (clicked) {
            if (!adding) {
                scene->deselect();
            }
            row.item->selected(!adding || !row.item->selected());
        }
        ui->popId();
    }
    ui->popId();
    ui->endRegion();
}

/**
 **/
void Outliner::keep(const std::vector<std::size_t>& editors) {
    std::erase_if(states_, [&](const auto& entry) { return std::ranges::find(editors, entry.first) == editors.end(); });
}

/**
 **/
bool Outliner::gather(const Walk& walk, const boost::shared_ptr<scene::item::Item>& item, unsigned int depth, std::vector<Row>* into) {
    const scene::Items children = walk.scene.children(item->id());
    std::vector<Row> below;
    bool any = false;
    for (const boost::shared_ptr<scene::item::Item>& child : children) {
        any = gather(walk, child, depth + 1, &below) || any;
    }
    const std::string named = lower(label(*item));
    const bool matches = std::ranges::all_of(walk.words, [&](const std::string& word) { return named.find(word) != std::string::npos; });
    if (!matches && !any) {
        return false;
    }

    // a filtered tree shows each item it keeps where it sits, with nothing folded
    const bool filtering = !walk.words.empty();
    Row row{ .item = item, .depth = depth };
    row.parent = !filtering && !children.empty();
    row.open = filtering ? any : !walk.folded.contains(item->documentId());
    into->push_back(row);
    if (row.open) {
        into->insert(into->end(), below.begin(), below.end());
    }
    return true;
}

};  // namespace v3d::editor::control
