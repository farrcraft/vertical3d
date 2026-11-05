/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Scene.h"

#include <api/animation/Curve.h>
#include <api/brep/BRep.h>
#include <vertical3d/src/scene/item/Camera.h>
#include <vertical3d/src/scene/item/Components.h>
#include <vertical3d/src/scene/item/Curve.h>
#include <vertical3d/src/scene/item/Group.h>
#include <vertical3d/src/scene/item/Item.h>
#include <vertical3d/src/scene/item/Light.h>
#include <vertical3d/src/scene/item/Mesh.h>

#include "Evaluation.h"
#include "Properties.h"
#include "RenderSettings.h"
#include "Timeline.h"
#include "Units.h"
#include "Visitor.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace v3d::editor::scene {

/**
 **/
void Scene::walk(const Walk& walk, const boost::shared_ptr<item::Item>& item) {
    const item::Item::Shown& shown = item->shown();
    if ((walk.showing == Showing::Views && !shown.views) || (walk.showing == Showing::Renders && !shown.renders)) {
        return;
    }
    Visitor* visitor = walk.visitor;
    const Evaluation& evaluation = walk.evaluation;
    const Placed placed = evaluation.placed(item->id());
    switch (item->kind()) {
        case item::Item::Kind::Mesh:
            visitor->visit(boost::static_pointer_cast<item::Mesh>(item), placed);
            break;
        case item::Item::Kind::Camera:
            visitor->visit(boost::static_pointer_cast<item::Camera>(item), placed, evaluation.pose(*item).lens);
            break;
        case item::Item::Kind::Light:
            visitor->visit(boost::static_pointer_cast<item::Light>(item), placed, evaluation.pose(*item).emission);
            break;
        case item::Item::Kind::Group:
            visitor->visit(boost::static_pointer_cast<item::Group>(item), placed);
            break;
        case item::Item::Kind::Curve:
            visitor->visit(boost::static_pointer_cast<item::Curve>(item), placed);
            break;
    }
    const auto beneath = walk.tree.find(item->documentId());
    if (beneath != walk.tree.end()) {
        for (const boost::shared_ptr<item::Item>& child : beneath->second) {
            Scene::walk(walk, child);
        }
    }
    visitor->leave(item);
}

/**
 **/
unsigned int Scene::add(const boost::shared_ptr<item::Item>& item) {
    if (!item) {
        return 0;
    }
    if (item->documentId() == 0) {
        item->documentId(nextDocumentId_++);
    }
    const std::string name = available(item->name(), *item);
    if (name != item->name()) {
        item->name(name);
    }
    items_.push_back(item);
    settle(*item);
    return item->id();
}

/**
 **/
bool Scene::remove(unsigned int id) {
    const boost::shared_ptr<item::Item> top = item(id);
    if (!top) {
        return false;
    }
    // every link has both ends in the scene, so the items beneath are found by following the
    // links down from the top one
    std::unordered_set<std::int64_t> gone{ top->documentId() };
    std::vector<std::int64_t> open{ top->documentId() };
    while (!open.empty()) {
        const std::int64_t above = open.back();
        open.pop_back();
        for (const auto& [child, parent] : parents_) {
            if (parent == above && gone.insert(child).second) {
                open.push_back(child);
            }
        }
    }
    for (const boost::shared_ptr<item::Item>& each : items_) {
        if (gone.contains(each->documentId())) {
            components_.clear(each->id());
        }
    }
    std::erase_if(items_, [&gone](const boost::shared_ptr<item::Item>& each) { return gone.contains(each->documentId()); });
    std::erase_if(parents_, [&gone](const auto& link) { return gone.contains(link.first); });
    std::erase_if(curves_, [&gone](const auto& owned) { return gone.contains(owned.first); });
    std::erase_if(settled_, [&gone](const auto& held) { return gone.contains(held.first); });
    return true;
}

/**
 **/
bool Scene::parent(unsigned int child, unsigned int parent) {
    const boost::shared_ptr<item::Item> below = item(child);
    if (!below) {
        return false;
    }
    if (parent == 0) {
        parents_.erase(below->documentId());
        return true;
    }
    const boost::shared_ptr<item::Item> above = item(parent);
    if (!above || above == below) {
        return false;
    }
    const Items over = ancestors(parent);
    if (std::find(over.begin(), over.end(), below) != over.end()) {
        return false;
    }
    parents_[below->documentId()] = above->documentId();
    return true;
}

/**
 **/
boost::shared_ptr<item::Item> Scene::parent(unsigned int child) const {
    const boost::shared_ptr<item::Item> below = item(child);
    if (!below) {
        return boost::shared_ptr<item::Item>();
    }
    const auto link = parents_.find(below->documentId());
    if (link == parents_.end()) {
        return boost::shared_ptr<item::Item>();
    }
    return document(link->second);
}

/**
 **/
Items Scene::children(unsigned int id) const {
    std::int64_t above = 0;
    if (id != 0) {
        const boost::shared_ptr<item::Item> found = item(id);
        if (!found) {
            return Items();
        }
        above = found->documentId();
    }
    Items beneath;
    for (const boost::shared_ptr<item::Item>& each : items_) {
        const auto link = parents_.find(each->documentId());
        if ((link == parents_.end() ? 0 : link->second) == above) {
            beneath.push_back(each);
        }
    }
    return beneath;
}

/**
 **/
Items Scene::ancestors(unsigned int id) const {
    Items above;
    for (boost::shared_ptr<item::Item> at = parent(id); at; at = parent(at->id())) {
        above.push_back(at);
    }
    return above;
}

/**
 **/
Items Scene::uppermost(const Items& items) const {
    Items top;
    for (const boost::shared_ptr<item::Item>& each : items) {
        if (!each) {
            continue;
        }
        const Items over = ancestors(each->id());
        const bool under = std::any_of(over.begin(), over.end(),
            [&items](const boost::shared_ptr<item::Item>& above) { return std::find(items.begin(), items.end(), above) != items.end(); });
        if (!under) {
            top.push_back(each);
        }
    }
    return top;
}

/**
 **/
Curves Scene::curves(unsigned int id) const {
    const boost::shared_ptr<item::Item> found = item(id);
    if (!found) {
        return Curves();
    }
    const auto owned = curves_.find(found->documentId());
    return owned == curves_.end() ? Curves() : owned->second;
}

/**
 **/
bool Scene::curve(unsigned int id, const std::string& path, const v3d::animation::Curve& curve) {
    const boost::shared_ptr<item::Item> found = item(id);
    if (!found || !declares(found->kind(), path)) {
        return false;
    }
    if (curve.empty()) {
        const auto owned = curves_.find(found->documentId());
        if (owned != curves_.end()) {
            owned->second.erase(path);
            if (owned->second.empty()) {
                curves_.erase(owned);
            }
        }
        return true;
    }
    curves_[found->documentId()][path] = curve;
    return true;
}

/**
 **/
std::vector<float> Scene::keyedFrames(const Items& items) const {
    std::vector<float> frames;
    for (const boost::shared_ptr<item::Item>& each : items) {
        if (!each || item(each->id()) != each) {
            continue;
        }
        const auto owned = curves_.find(each->documentId());
        if (owned == curves_.end()) {
            continue;
        }
        for (const auto& [path, curve] : owned->second) {
            for (const v3d::animation::Curve::Key& key : curve.keys()) {
                frames.push_back(key.frame);
            }
        }
    }
    std::sort(frames.begin(), frames.end());
    frames.erase(std::unique(frames.begin(), frames.end(),
        [](float first, float second) { return second - first <= v3d::animation::Curve::SAME_FRAME; }), frames.end());
    return frames;
}

/**
 **/
void Scene::frame(int frame) {
    timeline_.frame = frame;
    for (const boost::shared_ptr<item::Item>& each : items_) {
        settle(*each);
    }
}

/**
 **/
const Timeline& Scene::timeline() const noexcept {
    return timeline_;
}

/**
 **/
void Scene::timeline(const Timeline& timeline) noexcept {
    timeline_ = timeline;
}

/**
 **/
const RenderSettings& Scene::renderSettings() const noexcept {
    return renderSettings_;
}

/**
 **/
void Scene::renderSettings(const RenderSettings& settings) noexcept {
    renderSettings_ = settings;
}

/**
 **/
Length Scene::length() const noexcept {
    return length_;
}

/**
 **/
void Scene::length(Length unit) noexcept {
    length_ = unit;
}

/**
 **/
Evaluation Scene::evaluate() const {
    return evaluated(static_cast<float>(timeline_.frame), true);
}

/**
 **/
Evaluation Scene::evaluate(float frame) const {
    return evaluated(frame, false);
}

/**
 **/
Evaluation Scene::evaluated(float frame, bool pending) const {
    Evaluation evaluation;
    std::unordered_map<std::int64_t, const item::Item*> documents;
    for (const boost::shared_ptr<item::Item>& each : items_) {
        documents[each->documentId()] = each.get();
        Pose pose = scene::pose(*each);
        const auto owned = curves_.find(each->documentId());
        if (owned != curves_.end()) {
            const std::set<std::string> kept = pending ? edited(*each) : std::set<std::string>();
            std::map<std::string, float> values;
            for (const auto& [path, curve] : owned->second) {
                if (!kept.contains(path)) {
                    values[path] = curve.sample(frame);
                }
            }
            pose = posed(each->kind(), pose, values);
        }
        evaluation.pose(each->id(), pose);
    }
    const auto above = [this, &documents](const item::Item& below) -> const item::Item* {
        const auto link = parents_.find(below.documentId());
        return link == parents_.end() ? nullptr : documents.at(link->second);
    };

    std::unordered_set<unsigned int> placed;
    std::vector<const item::Item*> chain;
    for (const boost::shared_ptr<item::Item>& each : items_) {
        // the item and those of its ancestors not yet placed, placed from the top down so
        // that each parent's world matrix is ready before its child's
        chain.clear();
        for (const item::Item* at = each.get(); at != nullptr && !placed.contains(at->id()); at = above(*at)) {
            chain.push_back(at);
        }
        for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
            const item::Item* parent = above(**it);
            Placed where;
            where.parent = parent == nullptr ? glm::mat4(1.0F) : evaluation.placed(parent->id()).world;
            where.local = evaluation.pose(**it).placement.matrix();
            where.world = where.parent * where.local;
            evaluation.place((*it)->id(), where);
            placed.insert((*it)->id());
        }
    }
    return evaluation;
}

/**
 **/
boost::shared_ptr<item::Item> Scene::item(unsigned int id) const {
    auto it = std::find_if(items_.begin(), items_.end(),
        [id](const boost::shared_ptr<item::Item>& item) { return item->id() == id; });
    if (it == items_.end()) {
        return boost::shared_ptr<item::Item>();
    }
    return *it;
}

/**
 **/
boost::shared_ptr<item::Item> Scene::named(const std::string& name) const {
    const auto found = std::find_if(items_.begin(), items_.end(),
        [&name](const boost::shared_ptr<item::Item>& item) { return item->name() == name; });
    return found == items_.end() ? boost::shared_ptr<item::Item>() : *found;
}

/**
 **/
std::string Scene::available(const std::string& wanted, const item::Item& item) const {
    std::string name = wanted.empty() ? item::name(item.kind()) : wanted;
    std::unordered_set<std::string> taken;
    for (const boost::shared_ptr<item::Item>& other : items_) {
        if (other.get() != &item) {
            taken.insert(other->name());
        }
    }
    if (!taken.contains(name)) {
        return name;
    }
    std::string stem = name;
    const std::size_t dot = name.rfind('.');
    if (dot != std::string::npos && name.size() - dot > 3 &&
        std::all_of(name.begin() + static_cast<std::ptrdiff_t>(dot) + 1, name.end(), [](char letter) { return letter >= '0' && letter <= '9'; })) {
        stem = name.substr(0, dot);
    }
    for (std::size_t number = 1;; ++number) {
        std::string digits = std::to_string(number);
        if (digits.size() < 3) {
            digits.insert(0, 3 - digits.size(), '0');
        }
        std::string candidate = stem;
        candidate += '.';
        candidate += digits;
        if (!taken.contains(candidate)) {
            return candidate;
        }
    }
}

/**
 **/
boost::shared_ptr<item::Mesh> Scene::mesh(unsigned int id) const {
    return as<item::Mesh>(item(id));
}

/**
 **/
Items Scene::selection() const {
    Items selected;
    std::copy_if(items_.begin(), items_.end(), std::back_inserter(selected),
        [](const boost::shared_ptr<item::Item>& item) { return item->selected(); });
    return selected;
}

/**
 **/
std::size_t Scene::count() const noexcept {
    return items_.size();
}

/**
 **/
void Scene::clear() noexcept {
    items_.clear();
    parents_.clear();
    curves_.clear();
    settled_.clear();
    components_.clear();
    timeline_ = Timeline();
    renderSettings_ = RenderSettings();
    nextDocumentId_ = 1;
}

/**
 **/
std::int64_t Scene::nextDocumentId() const noexcept {
    return nextDocumentId_;
}

/**
 **/
void Scene::nextDocumentId(std::int64_t id) noexcept {
    nextDocumentId_ = id;
}

/**
 **/
const Items& Scene::items() const noexcept {
    return items_;
}

/**
 **/
item::Item::Shown Scene::shown(unsigned int id) const {
    const boost::shared_ptr<item::Item> found = item(id);
    if (!found) {
        return { .views = false, .renders = false };
    }
    item::Item::Shown shown = found->shown();
    for (const boost::shared_ptr<item::Item>& above : ancestors(id)) {
        shown.views = shown.views && above->shown().views;
        shown.renders = shown.renders && above->shown().renders;
    }
    return shown;
}

/**
 **/
void Scene::accept(Visitor* visitor, Showing showing) const {
    accept(visitor, evaluate(), showing);
}

/**
 **/
void Scene::accept(Visitor* visitor, const Evaluation& evaluation, Showing showing) const {
    if (visitor == nullptr) {
        return;
    }
    Tree tree;
    for (const boost::shared_ptr<item::Item>& each : items_) {
        const auto link = parents_.find(each->documentId());
        tree[link == parents_.end() ? 0 : link->second].push_back(each);
    }
    const auto roots = tree.find(0);
    if (roots == tree.end()) {
        return;
    }
    const Walk walking{ .visitor = visitor, .evaluation = evaluation, .tree = tree, .showing = showing };
    for (const boost::shared_ptr<item::Item>& each : roots->second) {
        walk(walking, each);
    }
}

/**
 **/
boost::shared_ptr<item::Item> Scene::document(std::int64_t id) const {
    auto it = std::find_if(items_.begin(), items_.end(),
        [id](const boost::shared_ptr<item::Item>& each) { return each->documentId() == id; });
    if (it == items_.end()) {
        return boost::shared_ptr<item::Item>();
    }
    return *it;
}

/**
 **/
std::set<std::string> Scene::edited(const item::Item& item) const {
    const auto settled = settled_.find(item.documentId());
    if (settled == settled_.end()) {
        return std::set<std::string>();
    }
    return changed(settled->second, values(item, glm::vec3(0.0F)));
}

/**
 **/
void Scene::settle(const item::Item& item) {
    settled_[item.documentId()] = values(item, glm::vec3(0.0F));
}

/**
 **/
void Scene::deselect() noexcept {
    for (const boost::shared_ptr<item::Item>& item : items_) {
        item->selected(false);
    }
    deselectComponents();
}

/**
 **/
void Scene::deselectComponents() noexcept {
    components_.clear();
}

/**
 **/
void Scene::deselectComponents(const v3d::brep::BRep* geometry) noexcept {
    for (const boost::shared_ptr<item::Item>& item : items_) {
        const boost::shared_ptr<item::Mesh> mesh = as<item::Mesh>(item);
        if (mesh && mesh->geometry().get() == geometry) {
            components_.clear(mesh->id());
        }
    }
}

/**
 **/
item::Components& Scene::components() noexcept {
    return components_;
}

/**
 **/
const item::Components& Scene::components() const noexcept {
    return components_;
}

};  // namespace v3d::editor::scene
