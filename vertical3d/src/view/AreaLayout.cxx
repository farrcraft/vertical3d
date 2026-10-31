/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "AreaLayout.h"

#include <api/asset/Json.h>
#include <api/log/Logger.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace v3d::editor {

namespace {

/**
 * How many steps a snapped divider takes across its split.
 **/
constexpr float TENTHS = 10.0F;

/**
 * Take one editor out of an area, and keep the area showing the one it showed, or the one
 * before it when that is the one taken.
 **/
AreaLayout::Editor take(AreaLayout::Area* area, std::size_t editor) {
    AreaLayout::Editor taken = area->editors[editor];
    area->editors.erase(area->editors.begin() + static_cast<std::ptrdiff_t>(editor));
    if (editor < area->chosen || (editor == area->chosen && area->chosen > 0)) {
        area->chosen--;
    }
    return taken;
}

/**
 * @return a region shrunk by a margin on every side, and never to less than nothing
 **/
glm::vec4 inset(const glm::vec4& region, float margin) {
    return glm::vec4(region.x + margin, region.y + margin,
        std::max(0.0F, region.z - (2.0F * margin)), std::max(0.0F, region.w - (2.0F * margin)));
}

bool contains(const glm::vec4& region, float x, float y) noexcept {
    return x >= region.x && x < region.x + region.z && y >= region.y && y < region.y + region.w;
}

};  // namespace

/**
 **/
const AreaLayout::Editor& AreaLayout::Area::editor() const {
    return editors.at(chosen);
}

/**
 **/
AreaLayout::AreaLayout(const boost::shared_ptr<v3d::log::Logger>& logger) :
    logger_(logger) {
}

/**
 **/
bool AreaLayout::load(const boost::json::object& layout) {
    std::string name;
    if (layout.contains("name")) {
        const std::optional<std::string> named = v3d::asset::readString(layout, "name");
        if (!named) {
            logger_->get()->error("A layout's name is a string");
            return false;
        }
        name = *named;
    }
    const boost::json::object* root = v3d::asset::readObject(layout, "root");
    if (root == nullptr) {
        logger_->get()->error("A layout needs a root node");
        return false;
    }

    Node tree;
    std::vector<Area> areas;
    if (!loadNode(*root, &tree, &areas)) {
        return false;
    }
    release();
    maximised_.reset();
    name_ = name;
    root_ = std::move(tree);
    areas_ = std::move(areas);
    place();
    return true;
}

/**
 **/
bool AreaLayout::loadNode(const boost::json::object& entry, Node* into, std::vector<Area>* areas) {
    if (entry.contains("share")) {
        const std::optional<double> share = v3d::asset::readNumber(entry, "share");
        if (!share || !std::isfinite(*share) || *share <= 0.0) {
            logger_->get()->error("A layout node's share is a number above zero");
            return false;
        }
        into->share = static_cast<float>(*share);
    }

    // an area lists its editors; anything else is a split
    if (entry.contains("editors")) {
        Area area;
        if (!loadArea(entry, &area)) {
            return false;
        }
        into->area = areas->size();
        areas->push_back(std::move(area));
        return true;
    }

    const boost::json::array* children = v3d::asset::readArray(entry, "children");
    if (children == nullptr) {
        logger_->get()->error("A layout node is either an area or a split with children");
        return false;
    }
    // place() treats a node with no children as an area, so an empty split would take one
    if (children->empty()) {
        logger_->get()->error("A layout split has no children");
        return false;
    }
    // horizontal puts its children side by side; vertical stacks them
    const std::optional<std::string> split = v3d::asset::readString(entry, "split");
    if (entry.contains("split") && !split) {
        logger_->get()->error("A layout node's split is horizontal or vertical");
        return false;
    }
    into->vertical = (split.value_or("vertical") != "horizontal");

    for (const boost::json::value& child : *children) {
        if (!child.is_object()) {
            logger_->get()->error("Unrecognized layout node");
            return false;
        }
        Node node;
        if (!loadNode(child.as_object(), &node, areas)) {
            return false;
        }
        into->children.push_back(std::move(node));
    }
    return true;
}

/**
 **/
bool AreaLayout::loadArea(const boost::json::object& entry, Area* into) {
    const boost::json::array* editors = v3d::asset::readArray(entry, "editors");
    if (editors == nullptr || editors->empty()) {
        logger_->get()->error("A layout area holds a list of one or more editors");
        return false;
    }
    into->id = nextArea_++;
    for (const boost::json::value& value : *editors) {
        const std::optional<std::string> kind = value.is_object() ?
            v3d::asset::readString(value.as_object(), "kind") : std::nullopt;
        if (!kind || kind->empty()) {
            logger_->get()->error("A layout editor is an object that names its kind");
            return false;
        }
        into->editors.push_back({ .id = nextEditor_++, .kind = *kind, .state = value.as_object() });
    }
    if (entry.contains("chosen")) {
        const std::optional<int> chosen = v3d::asset::readInt(entry, "chosen");
        if (!chosen || *chosen < 0 || static_cast<std::size_t>(*chosen) >= into->editors.size()) {
            logger_->get()->error("A layout area's chosen editor is one of its editors");
            return false;
        }
        into->chosen = static_cast<std::size_t>(*chosen);
    }
    return true;
}

/**
 **/
boost::json::object AreaLayout::save() const {
    boost::json::object layout;
    if (!name_.empty()) {
        layout["name"] = name_;
    }
    layout["root"] = saveNode(root_);
    return layout;
}

/**
 **/
boost::json::object AreaLayout::saveNode(const Node& node) const {
    boost::json::object entry;
    entry["share"] = node.share;
    if (node.children.empty()) {
        const Area& area = areas_[node.area];
        boost::json::array editors;
        for (const Editor& editor : area.editors) {
            boost::json::object state = editor.state;
            state["kind"] = editor.kind;
            editors.emplace_back(std::move(state));
        }
        entry["editors"] = std::move(editors);
        entry["chosen"] = area.chosen;
        return entry;
    }
    entry["split"] = node.vertical ? "vertical" : "horizontal";
    boost::json::array children;
    for (const Node& child : node.children) {
        children.emplace_back(saveNode(child));
    }
    entry["children"] = std::move(children);
    return entry;
}

/**
 **/
const std::string& AreaLayout::name() const noexcept {
    return name_;
}

/**
 **/
void AreaLayout::metrics(const Metrics& metrics) {
    metrics_ = metrics;
    place();
}

/**
 **/
const AreaLayout::Metrics& AreaLayout::metrics() const noexcept {
    return metrics_;
}

/**
 **/
void AreaLayout::resize(const glm::vec4& region) {
    region_ = glm::vec4(region.x, region.y, std::max(0.0F, region.z), std::max(0.0F, region.w));
    place();
}

/**
 **/
void AreaLayout::place() {
    const glm::vec4 room = inset(region_, metrics_.gap);
    place(&root_, room);
    if (!maximised_) {
        return;
    }
    for (Area& area : areas_) {
        area.region = area.id == *maximised_ ? room : glm::vec4(0.0F);
    }
}

/**
 **/
void AreaLayout::place(Node* node, const glm::vec4& region) {
    node->region = region;
    if (node->children.empty()) {
        if (node->area < areas_.size()) {
            areas_[node->area].region = region;
        }
        return;
    }

    const std::size_t count = node->children.size();
    float shares = 0.0F;
    for (const Node& child : node->children) {
        shares += child.share;
    }
    const float gaps = metrics_.gap * static_cast<float>(count - 1);
    const float total = node->vertical ? region.w : region.z;
    const float room = std::max(0.0F, total - gaps);
    float offset = node->vertical ? region.y : region.x;
    const float end = offset + total;

    // the last child takes what rounding each share down to a whole pixel left over, so the
    // children and their gaps cover the whole region
    for (std::size_t index = 0; index < count; index++) {
        const bool last = (index + 1 == count);
        const float extent = std::max(0.0F, last ? (end - offset) : std::floor(room * node->children[index].share / shares));
        glm::vec4 child = region;
        if (node->vertical) {
            child.y = offset;
            child.w = extent;
        } else {
            child.x = offset;
            child.z = extent;
        }
        place(&node->children[index], child);
        offset += extent + metrics_.gap;
    }
}

/**
 **/
const std::vector<AreaLayout::Area>& AreaLayout::areas() const noexcept {
    return areas_;
}

/**
 **/
std::size_t AreaLayout::find(std::size_t id) const noexcept {
    const auto found = std::ranges::find(areas_, id, &Area::id);
    return static_cast<std::size_t>(found - areas_.begin());
}

/**
 **/
std::size_t AreaLayout::areaAt(float x, float y) const noexcept {
    const auto found = std::ranges::find_if(areas_, [x, y](const Area& area) { return contains(area.region, x, y); });
    return static_cast<std::size_t>(found - areas_.begin());
}

/**
 **/
std::vector<glm::vec4> AreaLayout::gaps() const {
    const glm::vec4 room = inset(region_, metrics_.gap);
    const float right = room.x + room.z;
    const float bottom = room.y + room.w;
    std::vector<glm::vec4> found = {
        glm::vec4(region_.x, region_.y, region_.z, room.y - region_.y),
        glm::vec4(region_.x, bottom, region_.z, region_.y + region_.w - bottom),
        glm::vec4(region_.x, room.y, room.x - region_.x, room.w),
        glm::vec4(right, room.y, region_.x + region_.z - right, room.w),
    };
    if (!maximised_) {
        gapsIn(root_, &found);
    }
    return found;
}

/**
 **/
void AreaLayout::gapsIn(const Node& node, std::vector<glm::vec4>* into) {
    for (std::size_t index = 0; index + 1 < node.children.size(); index++) {
        into->push_back(gapAfter(node, index).region);
    }
    for (const Node& child : node.children) {
        gapsIn(child, into);
    }
}

/**
 **/
float AreaLayout::least(const Node& node, bool vertical) const {
    if (node.children.empty()) {
        return metrics_.least;
    }
    float sum = 0.0F;
    float most = 0.0F;
    for (const Node& child : node.children) {
        const float needs = least(child, vertical);
        sum += needs;
        most = std::max(most, needs);
    }
    if (node.vertical != vertical) {
        return most;
    }
    return sum + (metrics_.gap * static_cast<float>(node.children.size() - 1));
}

/**
 **/
AreaLayout::Divider AreaLayout::gapAfter(const Node& split, std::size_t index) {
    const glm::vec4& before = split.children[index].region;
    const glm::vec4& after = split.children[index + 1].region;
    Divider divider;
    divider.across = split.vertical;
    if (split.vertical) {
        const float top = before.y + before.w;
        divider.region = glm::vec4(split.region.x, top, split.region.z, after.y - top);
    } else {
        const float left = before.x + before.z;
        divider.region = glm::vec4(left, split.region.y, after.x - left, split.region.w);
    }
    return divider;
}

/**
 **/
std::optional<AreaLayout::Found> AreaLayout::dividerIn(const Node& node, float x, float y, std::vector<std::size_t>* path) const {
    if (node.children.empty()) {
        return std::nullopt;
    }
    const glm::vec4& region = node.region;
    const float reach = metrics_.reach;
    if (x < region.x - reach || x > region.x + region.z + reach || y < region.y - reach || y > region.y + region.w + reach) {
        return std::nullopt;
    }
    // a divider is the gap where one child ends and the next begins, along the split's axis
    for (std::size_t index = 0; index + 1 < node.children.size(); index++) {
        const glm::vec4 gap = gapAfter(node, index).region;
        const bool hit = node.vertical ?
            (y >= gap.y - reach && y <= gap.y + gap.w + reach && x >= gap.x && x < gap.x + gap.z) :
            (x >= gap.x - reach && x <= gap.x + gap.z + reach && y >= gap.y && y < gap.y + gap.w);
        if (hit) {
            return Found{ .path = *path, .after = index };
        }
    }
    for (std::size_t index = 0; index < node.children.size(); index++) {
        path->push_back(index);
        std::optional<Found> found = dividerIn(node.children[index], x, y, path);
        path->pop_back();
        if (found) {
            return found;
        }
    }
    return std::nullopt;
}

/**
 **/
std::optional<AreaLayout::Found> AreaLayout::dividerAt(float x, float y) const {
    if (maximised_) {
        return std::nullopt;
    }
    std::vector<std::size_t> path;
    return dividerIn(root_, x, y, &path);
}

/**
 **/
AreaLayout::Node* AreaLayout::walk(const std::vector<std::size_t>& path) {
    Node* node = &root_;
    for (const std::size_t index : path) {
        node = &node->children[index];
    }
    return node;
}

/**
 **/
std::optional<AreaLayout::Divider> AreaLayout::divider(float x, float y) const {
    const std::optional<Found> found = dividerAt(x, y);
    if (!found) {
        return std::nullopt;
    }
    const Node* node = &root_;
    for (const std::size_t index : found->path) {
        node = &node->children[index];
    }
    return gapAfter(*node, found->after);
}

/**
 **/
bool AreaLayout::grab(float x, float y) {
    release();
    const std::optional<Found> found = dividerAt(x, y);
    if (!found) {
        return false;
    }
    held_ = walk(found->path);
    heldAfter_ = found->after;
    heldShares_.clear();
    for (const Node& child : held_->children) {
        heldShares_.push_back(child.share);
    }
    return true;
}

/**
 **/
std::optional<AreaLayout::Divider> AreaLayout::held() const {
    if (held_ == nullptr) {
        return std::nullopt;
    }
    return gapAfter(*held_, heldAfter_);
}

/**
 **/
bool AreaLayout::grabbed() const noexcept {
    return held_ != nullptr;
}

/**
 **/
void AreaLayout::drag(float x, float y, bool snap) {
    if (held_ == nullptr) {
        return;
    }
    Node& first = held_->children[heldAfter_];
    Node& second = held_->children[heldAfter_ + 1];
    const bool vertical = held_->vertical;
    const float start = vertical ? first.region.y : first.region.x;
    const float end = vertical ? second.region.y + second.region.w : second.region.x + second.region.z;
    const float gap = metrics_.gap;

    float centre = vertical ? y : x;
    if (snap) {
        const float origin = vertical ? held_->region.y : held_->region.x;
        const float extent = vertical ? held_->region.w : held_->region.z;
        if (extent > 0.0F) {
            centre = origin + (std::round((centre - origin) / extent * TENTHS) / TENTHS * extent);
        }
    }
    const float lowest = start + least(first, vertical);
    const float highest = end - gap - least(second, vertical);
    if (highest < lowest) {
        return;
    }

    // the two keep the weight they had between them, divided where the divider now is
    const float boundary = std::clamp(centre - (gap / 2.0F), lowest, highest);
    const float before = boundary - start;
    const float after = end - gap - boundary;
    const float weight = first.share + second.share;
    first.share = weight * before / (before + after);
    second.share = weight - first.share;
    place();
}

/**
 **/
void AreaLayout::release() noexcept {
    held_ = nullptr;
    heldAfter_ = 0;
    heldShares_.clear();
}

/**
 **/
void AreaLayout::cancel() {
    if (held_ != nullptr && heldShares_.size() == held_->children.size()) {
        for (std::size_t index = 0; index < heldShares_.size(); index++) {
            held_->children[index].share = heldShares_[index];
        }
        place();
    }
    release();
}

/**
 **/
bool AreaLayout::even(float x, float y) {
    const std::optional<Found> found = dividerAt(x, y);
    if (!found) {
        return false;
    }
    Node* split = walk(found->path);
    Node& first = split->children[found->after];
    Node& second = split->children[found->after + 1];
    const float half = (first.share + second.share) / 2.0F;
    first.share = half;
    second.share = half;
    place();
    return true;
}

/**
 **/
void AreaLayout::maximise(std::size_t area) {
    if (area >= areas_.size()) {
        return;
    }
    release();
    if (maximised_ == areas_[area].id) {
        maximised_.reset();
    } else {
        maximised_ = areas_[area].id;
    }
    place();
}

/**
 **/
std::optional<std::size_t> AreaLayout::maximised() const noexcept {
    if (!maximised_) {
        return std::nullopt;
    }
    return find(*maximised_);
}

/**
 **/
AreaLayout::Spot AreaLayout::spot(std::size_t area) {
    std::vector<Spot> pending{ Spot{ .split = nullptr, .at = 0, .node = &root_ } };
    while (!pending.empty()) {
        const Spot next = pending.back();
        pending.pop_back();
        if (next.node->children.empty()) {
            if (next.node->area == area) {
                return next;
            }
            continue;
        }
        for (std::size_t index = 0; index < next.node->children.size(); index++) {
            pending.push_back(Spot{ .split = next.node, .at = index, .node = &next.node->children[index] });
        }
    }
    return Spot{};
}

/**
 **/
std::optional<std::size_t> AreaLayout::divide(std::size_t area, Edge edge, const Editor& editor) {
    const Spot at = spot(area);
    if (at.node == nullptr) {
        return std::nullopt;
    }
    const bool vertical = (edge == Edge::Top || edge == Edge::Bottom);
    const bool before = (edge == Edge::Top || edge == Edge::Left);
    const glm::vec4& region = areas_[area].region;
    if ((vertical ? region.w : region.z) < (2.0F * metrics_.least) + metrics_.gap) {
        return std::nullopt;
    }

    Area made;
    made.id = nextArea_++;
    made.editors.push_back(editor);
    Node leaf;
    leaf.area = areas_.size();
    areas_.push_back(std::move(made));

    // a split that already runs this way takes the new area beside the old one, so the tree
    // gains no level, and the two halve the old one's share
    if (at.split != nullptr && at.split->vertical == vertical) {
        const float half = at.node->share / 2.0F;
        at.node->share = half;
        leaf.share = half;
        at.split->children.insert(at.split->children.begin() + static_cast<std::ptrdiff_t>(at.at + (before ? 0 : 1)), std::move(leaf));
        return areas_.back().id;
    }
    Node old = std::move(*at.node);
    const float share = old.share;
    old.share = 1.0F;
    *at.node = Node{};
    at.node->vertical = vertical;
    at.node->share = share;
    if (before) {
        at.node->children.push_back(std::move(leaf));
        at.node->children.push_back(std::move(old));
    } else {
        at.node->children.push_back(std::move(old));
        at.node->children.push_back(std::move(leaf));
    }
    return areas_.back().id;
}

/**
 **/
void AreaLayout::remove(std::size_t area, std::optional<std::size_t> heir) {
    const Spot at = spot(area);
    if (at.split == nullptr) {
        return;
    }
    const std::size_t next = heir.value_or(at.at > 0 ? at.at - 1 : at.at + 1);
    at.split->children[next].share += at.node->share;
    at.split->children.erase(at.split->children.begin() + static_cast<std::ptrdiff_t>(at.at));
}

/**
 **/
void AreaLayout::tidy(Node* node) {
    if (node->children.empty()) {
        return;
    }
    std::vector<Node> children;
    for (Node& child : node->children) {
        tidy(&child);
        // a split left with one child is that child, in the split's place
        while (child.children.size() == 1) {
            Node only = std::move(child.children.front());
            only.share = child.share;
            child = std::move(only);
        }
        if (!child.children.empty() && child.vertical == node->vertical) {
            float shares = 0.0F;
            for (const Node& inner : child.children) {
                shares += inner.share;
            }
            for (Node& inner : child.children) {
                inner.share *= child.share / shares;
                children.push_back(std::move(inner));
            }
            continue;
        }
        children.push_back(std::move(child));
    }
    node->children = std::move(children);
}

/**
 **/
void AreaLayout::collect(Node* node, std::vector<Area>* into) {
    if (node->children.empty()) {
        into->push_back(std::move(areas_[node->area]));
        node->area = into->size() - 1;
        return;
    }
    for (Node& child : node->children) {
        collect(&child, into);
    }
}

/**
 **/
void AreaLayout::rebuild() {
    release();
    maximised_.reset();
    tidy(&root_);
    while (root_.children.size() == 1) {
        Node only = std::move(root_.children.front());
        root_ = std::move(only);
    }
    root_.share = 1.0F;
    std::vector<Area> areas;
    collect(&root_, &areas);
    areas_ = std::move(areas);
    place();
}

/**
 **/
void AreaLayout::restore() {
    release();
    if (maximised_) {
        maximised_.reset();
        place();
    }
}

/**
 **/
std::size_t AreaLayout::split(std::size_t area, Edge edge) {
    if (area >= areas_.size()) {
        return areas_.size();
    }
    restore();
    Editor copy = areas_[area].editor();
    copy.id = nextEditor_++;
    const std::optional<std::size_t> made = divide(area, edge, copy);
    if (!made) {
        return areas_.size();
    }
    rebuild();
    return find(*made);
}

/**
 **/
bool AreaLayout::join(std::size_t keep, std::size_t gone) {
    if (keep >= areas_.size() || gone >= areas_.size() || keep == gone) {
        return false;
    }
    const Spot kept = spot(keep);
    const Spot going = spot(gone);
    if (kept.split == nullptr || kept.split != going.split || (kept.at + 1 != going.at && going.at + 1 != kept.at)) {
        return false;
    }
    restore();
    remove(gone, kept.at);
    rebuild();
    return true;
}

/**
 **/
bool AreaLayout::swap(std::size_t first, std::size_t second) {
    if (first >= areas_.size() || second >= areas_.size()) {
        return false;
    }
    std::swap(areas_[first].editors, areas_[second].editors);
    std::swap(areas_[first].chosen, areas_[second].chosen);
    return true;
}

/**
 **/
std::optional<std::size_t> AreaLayout::add(std::size_t area, const boost::json::object& state) {
    const std::optional<std::string> kind = v3d::asset::readString(state, "kind");
    if (area >= areas_.size() || !kind || kind->empty()) {
        return std::nullopt;
    }
    const std::size_t id = nextEditor_++;
    areas_[area].editors.push_back({ .id = id, .kind = *kind, .state = state });
    areas_[area].chosen = areas_[area].editors.size() - 1;
    return id;
}

/**
 **/
bool AreaLayout::choose(std::size_t area, std::size_t editor) {
    if (area >= areas_.size() || editor >= areas_[area].editors.size()) {
        return false;
    }
    areas_[area].chosen = editor;
    return true;
}

/**
 **/
bool AreaLayout::close(std::size_t area, std::size_t editor) {
    if (area >= areas_.size() || editor >= areas_[area].editors.size() || areas_[area].editors.size() == 1) {
        return false;
    }
    take(&areas_[area], editor);
    return true;
}

/**
 **/
bool AreaLayout::move(std::size_t from, std::size_t editor, std::size_t to) {
    if (from >= areas_.size() || to >= areas_.size() || from == to || editor >= areas_[from].editors.size()) {
        return false;
    }
    Area& target = areas_[to];
    target.editors.push_back(take(&areas_[from], editor));
    target.chosen = target.editors.size() - 1;
    if (areas_[from].editors.empty()) {
        restore();
        remove(from);
        rebuild();
    }
    return true;
}

/**
 **/
bool AreaLayout::drop(std::size_t from, std::size_t editor, std::size_t to, Edge edge) {
    if (from >= areas_.size() || to >= areas_.size() || editor >= areas_[from].editors.size()) {
        return false;
    }
    if (from == to && areas_[from].editors.size() == 1) {
        return false;
    }
    restore();
    const glm::vec4& region = areas_[to].region;
    const bool vertical = (edge == Edge::Top || edge == Edge::Bottom);
    if ((vertical ? region.w : region.z) < (2.0F * metrics_.least) + metrics_.gap) {
        return false;
    }
    const Editor moved = take(&areas_[from], editor);
    const bool emptied = areas_[from].editors.empty();
    divide(to, edge, moved);
    if (emptied) {
        remove(from);
    }
    rebuild();
    return true;
}

};  // namespace v3d::editor
