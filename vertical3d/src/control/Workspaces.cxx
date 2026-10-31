/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Workspaces.h"

#include <api/asset/Json.h>
#include <api/log/Logger.h>
#include <vertical3d/src/view/AreaLayout.h>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <boost/json.hpp>

namespace v3d::editor {

/**
 **/
Workspaces::Workspaces(const boost::shared_ptr<v3d::log::Logger>& logger) :
    logger_(logger) {
}

/**
 **/
bool Workspaces::readable(const boost::json::object& layout) const {
    if (nameOf(layout).empty()) {
        logger_->get()->warn("A workspace has no name, so it is left out");
        return false;
    }
    AreaLayout trial(logger_);
    return trial.load(layout);
}

/**
 **/
std::vector<boost::json::object> Workspaces::read(const boost::json::value& set) const {
    std::vector<boost::json::object> found;
    const boost::json::array* layouts = set.is_object() ? v3d::asset::readArray(set.as_object(), SAVED) : nullptr;
    if (layouts == nullptr) {
        return found;
    }
    for (const boost::json::value& entry : *layouts) {
        if (!entry.is_object() || !readable(entry.as_object())) {
            continue;
        }
        const std::string name = nameOf(entry.as_object());
        const bool taken = std::ranges::any_of(found, [&name](const boost::json::object& held) { return nameOf(held) == name; });
        if (taken) {
            logger_->get()->warn("Two workspaces are named {}, so the second is left out", name);
            continue;
        }
        found.push_back(entry.as_object());
    }
    return found;
}

/**
 **/
bool Workspaces::load(const boost::json::object& shipped) {
    shipped_ = read(shipped);
    all_ = shipped_;
    own_ = false;
    return !all_.empty();
}

/**
 **/
void Workspaces::restore(const Stored& stored) {
    if (stored.saved != nullptr) {
        boost::json::object set;
        set[SAVED] = *stored.saved;
        std::vector<boost::json::object> own = read(set);
        if (!own.empty()) {
            all_ = std::move(own);
            own_ = true;
        }
    }
    projects_.clear();
    if (stored.projects == nullptr || !stored.projects->is_object()) {
        return;
    }
    for (const auto& [path, entry] : stored.projects->as_object()) {
        if (!entry.is_object()) {
            continue;
        }
        Record record;
        record.workspace = v3d::asset::readString(entry.as_object(), "workspace").value_or(std::string());
        if (const boost::json::object* layouts = v3d::asset::readObject(entry.as_object(), "layouts")) {
            for (const auto& [name, layout] : *layouts) {
                if (layout.is_object() && readable(layout.as_object())) {
                    record.layouts[std::string(name)] = layout.as_object();
                }
            }
        }
        if (!record.workspace.empty()) {
            projects_[std::string(path)] = std::move(record);
        }
    }
}

/**
 **/
const std::vector<boost::json::object>& Workspaces::all() const noexcept {
    return all_;
}

/**
 **/
std::size_t Workspaces::find(std::string_view name) const {
    const auto found = std::ranges::find_if(all_, [name](const boost::json::object& layout) { return nameOf(layout) == name; });
    return static_cast<std::size_t>(found - all_.begin());
}

/**
 **/
bool Workspaces::keep(const boost::json::object& layout) {
    if (!readable(layout)) {
        return false;
    }
    const std::size_t at = find(nameOf(layout));
    if (at < all_.size()) {
        all_[at] = layout;
    } else {
        all_.push_back(layout);
    }
    own_ = true;
    return true;
}

/**
 **/
bool Workspaces::reset(std::string_view name) {
    const auto shipped = std::ranges::find_if(shipped_, [name](const boost::json::object& layout) { return nameOf(layout) == name; });
    const std::size_t at = find(name);
    if (shipped == shipped_.end() || at >= all_.size()) {
        return false;
    }
    all_[at] = *shipped;
    return true;
}

/**
 **/
std::optional<boost::json::value> Workspaces::saved() const {
    if (!own_) {
        return std::nullopt;
    }
    boost::json::array set;
    for (const boost::json::object& layout : all_) {
        set.emplace_back(layout);
    }
    return boost::json::value(std::move(set));
}

/**
 **/
void Workspaces::remember(const Shown& shown, const boost::json::object& layout) {
    Record& record = projects_[shown.project];
    record.workspace = shown.workspace;
    record.layouts[shown.workspace] = layout;
}

/**
 **/
void Workspaces::forget(const Shown& shown) {
    const auto found = projects_.find(shown.project);
    if (found != projects_.end()) {
        found->second.layouts.erase(shown.workspace);
    }
}

/**
 **/
std::optional<Workspaces::Record> Workspaces::recall(const std::string& project) const {
    const auto found = projects_.find(project);
    if (found == projects_.end()) {
        return std::nullopt;
    }
    return found->second;
}

/**
 **/
std::size_t Workspaces::prune(const std::function<bool(const std::string& project)>& exists) {
    return std::erase_if(projects_, [&exists](const auto& entry) { return !exists(entry.first); });
}

/**
 **/
boost::json::value Workspaces::projects() const {
    boost::json::object all;
    for (const auto& [path, record] : projects_) {
        boost::json::object layouts;
        for (const auto& [name, layout] : record.layouts) {
            layouts[name] = layout;
        }
        all[path] = boost::json::object{ { "workspace", record.workspace }, { "layouts", std::move(layouts) } };
    }
    return all;
}

/**
 **/
boost::json::object Workspaces::exported(const boost::json::object& layout) {
    return boost::json::object{ { SAVED, boost::json::array{ layout } } };
}

/**
 **/
std::vector<std::string> Workspaces::imported(const boost::json::object& file) {
    std::vector<std::string> names;
    for (const boost::json::object& layout : read(file)) {
        if (keep(layout)) {
            names.push_back(nameOf(layout));
        }
    }
    return names;
}

/**
 **/
std::string Workspaces::nameOf(const boost::json::object& layout) {
    return v3d::asset::readString(layout, "name").value_or(std::string());
}

};  // namespace v3d::editor
