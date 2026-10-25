/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Scene.h"

#include "Camera.h"
#include "Evaluation.h"
#include "Group.h"
#include "Item.h"
#include "Light.h"
#include "Mesh.h"
#include "SceneVisitor.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/mat4x4.hpp>

namespace v3d::editor {

namespace {

/**
 * The items beneath each parent, named by its document id, in the scene's order. The items
 * with no parent are under 0.
 **/
using Tree = std::unordered_map<std::int64_t, Items>;

/**
 * Visit one item through the visit() for its kind, then everything beneath it.
 **/
void walk(SceneVisitor* visitor, const boost::shared_ptr<Item>& item, const Evaluation& evaluation, const Tree& tree) {
    const Placed placed = evaluation.placed(item->id());
    switch (item->kind()) {
        case Item::Kind::Mesh:
            visitor->visit(boost::static_pointer_cast<Mesh>(item), placed);
            break;
        case Item::Kind::Camera:
            visitor->visit(boost::static_pointer_cast<Camera>(item), placed);
            break;
        case Item::Kind::Light:
            visitor->visit(boost::static_pointer_cast<Light>(item), placed);
            break;
        case Item::Kind::Group:
            visitor->visit(boost::static_pointer_cast<Group>(item), placed);
            break;
    }
    const auto beneath = tree.find(item->documentId());
    if (beneath != tree.end()) {
        for (const boost::shared_ptr<Item>& child : beneath->second) {
            walk(visitor, child, evaluation, tree);
        }
    }
    visitor->leave(item);
}

};  // namespace

/**
 **/
unsigned int Scene::add(const boost::shared_ptr<Item>& item) {
    if (!item) {
        return 0;
    }
    if (item->documentId() == 0) {
        item->documentId(nextDocumentId_++);
    }
    items_.push_back(item);
    return item->id();
}

/**
 **/
bool Scene::remove(unsigned int id) {
    const boost::shared_ptr<Item> top = item(id);
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
    std::erase_if(items_, [&gone](const boost::shared_ptr<Item>& each) { return gone.contains(each->documentId()); });
    std::erase_if(parents_, [&gone](const auto& link) { return gone.contains(link.first); });
    return true;
}

/**
 **/
bool Scene::parent(unsigned int child, unsigned int parent) {
    const boost::shared_ptr<Item> below = item(child);
    if (!below) {
        return false;
    }
    if (parent == 0) {
        parents_.erase(below->documentId());
        return true;
    }
    const boost::shared_ptr<Item> above = item(parent);
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
boost::shared_ptr<Item> Scene::parent(unsigned int child) const {
    const boost::shared_ptr<Item> below = item(child);
    if (!below) {
        return boost::shared_ptr<Item>();
    }
    const auto link = parents_.find(below->documentId());
    if (link == parents_.end()) {
        return boost::shared_ptr<Item>();
    }
    return document(link->second);
}

/**
 **/
Items Scene::children(unsigned int id) const {
    std::int64_t above = 0;
    if (id != 0) {
        const boost::shared_ptr<Item> found = item(id);
        if (!found) {
            return Items();
        }
        above = found->documentId();
    }
    Items beneath;
    for (const boost::shared_ptr<Item>& each : items_) {
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
    for (boost::shared_ptr<Item> at = parent(id); at; at = parent(at->id())) {
        above.push_back(at);
    }
    return above;
}

/**
 **/
Items Scene::uppermost(const Items& items) const {
    Items top;
    for (const boost::shared_ptr<Item>& each : items) {
        if (!each) {
            continue;
        }
        const Items over = ancestors(each->id());
        const bool under = std::any_of(over.begin(), over.end(),
            [&items](const boost::shared_ptr<Item>& above) { return std::find(items.begin(), items.end(), above) != items.end(); });
        if (!under) {
            top.push_back(each);
        }
    }
    return top;
}

/**
 **/
Evaluation Scene::evaluate() const {
    std::unordered_map<std::int64_t, const Item*> documents;
    for (const boost::shared_ptr<Item>& each : items_) {
        documents[each->documentId()] = each.get();
    }
    const auto above = [this, &documents](const Item& below) -> const Item* {
        const auto link = parents_.find(below.documentId());
        return link == parents_.end() ? nullptr : documents.at(link->second);
    };

    Evaluation evaluation;
    std::unordered_set<unsigned int> placed;
    std::vector<const Item*> chain;
    for (const boost::shared_ptr<Item>& each : items_) {
        // the item and those of its ancestors not yet placed, placed from the top down so
        // that each parent's world matrix is ready before its child's
        chain.clear();
        for (const Item* at = each.get(); at != nullptr && !placed.contains(at->id()); at = above(*at)) {
            chain.push_back(at);
        }
        for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
            const Item* parent = above(**it);
            Placed where;
            where.parent = parent == nullptr ? glm::mat4(1.0F) : evaluation.placed(parent->id()).world;
            where.local = (*it)->matrix();
            where.world = where.parent * where.local;
            evaluation.place((*it)->id(), where);
            placed.insert((*it)->id());
        }
    }
    return evaluation;
}

/**
 **/
boost::shared_ptr<Item> Scene::item(unsigned int id) const {
    auto it = std::find_if(items_.begin(), items_.end(),
        [id](const boost::shared_ptr<Item>& item) { return item->id() == id; });
    if (it == items_.end()) {
        return boost::shared_ptr<Item>();
    }
    return *it;
}

/**
 **/
boost::shared_ptr<Mesh> Scene::mesh(unsigned int id) const {
    return as<Mesh>(item(id));
}

/**
 **/
Items Scene::selection() const {
    Items selected;
    std::copy_if(items_.begin(), items_.end(), std::back_inserter(selected),
        [](const boost::shared_ptr<Item>& item) { return item->selected(); });
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
void Scene::accept(SceneVisitor* visitor) const {
    if (visitor == nullptr) {
        return;
    }
    Tree tree;
    for (const boost::shared_ptr<Item>& each : items_) {
        const auto link = parents_.find(each->documentId());
        tree[link == parents_.end() ? 0 : link->second].push_back(each);
    }
    const auto roots = tree.find(0);
    if (roots == tree.end()) {
        return;
    }
    const Evaluation evaluation = evaluate();
    for (const boost::shared_ptr<Item>& each : roots->second) {
        walk(visitor, each, evaluation, tree);
    }
}

/**
 **/
boost::shared_ptr<Item> Scene::document(std::int64_t id) const {
    auto it = std::find_if(items_.begin(), items_.end(),
        [id](const boost::shared_ptr<Item>& each) { return each->documentId() == id; });
    if (it == items_.end()) {
        return boost::shared_ptr<Item>();
    }
    return *it;
}

/**
 **/
void Scene::deselect() noexcept {
    for (const boost::shared_ptr<Item>& item : items_) {
        item->selected(false);
    }
    deselectComponents();
}

/**
 **/
void Scene::deselectComponents() noexcept {
    for (const boost::shared_ptr<Item>& item : items_) {
        const boost::shared_ptr<Mesh> mesh = as<Mesh>(item);
        if (mesh) {
            mesh->geometry()->deselectComponents();
        }
    }
}

};  // namespace v3d::editor
