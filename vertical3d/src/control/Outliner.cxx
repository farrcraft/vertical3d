/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Outliner.h"

#include <api/ui/Immediate.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Scene.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor {

/**
 **/
std::string Outliner::label(const Item& item) {
    return item.name().empty() ? kind(item.kind()) : item.name();
}

/**
 **/
std::string Outliner::kind(Item::Kind kind) {
    switch (kind) {
        case Item::Kind::Mesh:
            return "Mesh";
        case Item::Kind::Camera:
            return "Camera";
        case Item::Kind::Light:
            return "Light";
        case Item::Kind::Group:
            return "Group";
        case Item::Kind::Curve:
            return "Curve";
    }
    return std::string();
}

/**
 **/
std::vector<Outliner::Row> Outliner::rows(const Scene& scene, const std::string& filter, const std::set<std::int64_t>& folded) {
    const std::vector<std::string> wanted = words(filter);
    const Walk walk{ .scene = scene, .words = wanted, .folded = folded };
    std::vector<Row> made;
    for (const boost::shared_ptr<Item>& item : scene.children(0)) {
        gather(walk, item, 0, &made);
    }
    return made;
}

/**
 **/
void Outliner::draw(v3d::ui::Immediate* ui, Scene* scene, std::size_t editor, const glm::vec4& region, bool adding) {
    State& state = states_[editor];
    const std::string id = "outliner " + std::to_string(editor);
    ui->region(id, { .position = glm::vec2(region.x, region.y), .size = glm::vec2(region.z, region.w) }, 1.0F);
    // two outliners draw the same labels, so each draws under its own id
    ui->pushId(id);
    ui->textField("Filter", &state.filter);
    ui->separator();
    for (const Row& row : rows(*scene, state.filter, state.folded)) {
        ui->pushId(static_cast<int>(row.item->documentId()));
        bool open = row.open;
        const bool clicked = ui->treeRow(label(*row.item),
            { .depth = row.depth, .open = row.parent ? &open : nullptr, .selected = row.item->selected() });
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
std::string Outliner::lower(const std::string& text) {
    std::string made = text;
    std::ranges::transform(made, made.begin(), [](char letter) {
        return static_cast<char>(std::tolower(static_cast<unsigned char>(letter)));
    });
    return made;
}

/**
 **/
std::vector<std::string> Outliner::words(const std::string& filter) {
    std::vector<std::string> made;
    std::istringstream split(lower(filter));
    for (std::string word; split >> word;) {
        made.push_back(word);
    }
    return made;
}

/**
 **/
bool Outliner::gather(const Walk& walk, const boost::shared_ptr<Item>& item, unsigned int depth, std::vector<Row>* into) {
    const Items children = walk.scene.children(item->id());
    std::vector<Row> below;
    bool any = false;
    for (const boost::shared_ptr<Item>& child : children) {
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

};  // namespace v3d::editor
