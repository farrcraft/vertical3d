/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include "Objects.h"

#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>

#include <set>
#include <string>

namespace v3d::moya::ri::internal {

Objects::Objects(scene::Scene * scene) : scene_(scene) {
}

scene::Handle Objects::make(const std::string & kind, const std::string & name) {
    const scene::Handle made = scene_->create(kind, name);
    taken_[kind].insert(name);
    made_.push_back(made);
    return made;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the kind, then the name, as in create()
std::string Objects::unique(const std::string & kind, const std::string & base) {
    std::string name = base;
    const std::set<std::string> & taken = taken_[kind];
    for (unsigned int again = 2; taken.contains(name); ++again) {
        name = base + " " + std::to_string(again);
    }
    return name;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the kind, then the request, then the name
std::string Objects::name(const std::string & kind, const std::string & request, const std::string & given) {
    return unique(kind, given.empty() ? request + " " + std::to_string(++counts_[kind + " " + request]) : given);
}

void Objects::clear() {
    for (const scene::Handle & made : made_) {
        scene_->remove(made);
    }
    made_.clear();
    taken_.clear();
    counts_.clear();
}

};  // namespace v3d::moya::ri::internal
