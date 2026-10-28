/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Manifest.h"

#include <api/asset/kind/Json.h>
#include <api/log/Logger.h>

#include "Json.h"
#include "Manager.h"

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace v3d::asset {
/**
 **/
Manifest::Manifest(const boost::shared_ptr<v3d::log::Logger>& logger) :
    logger_(logger) {
}

/**
 **/
bool Manifest::load(Manager& assets, std::string_view list) {
    const boost::shared_ptr<kind::Json> manifest = assets.load<kind::Json>(list);
    if (!manifest) {
        return false;
    }
    auto const doc = manifest->document();
    // boost::json's at() and as_*() throw, so every call below follows a contains() or a type
    // check. A list this function cannot read returns false, never an exception.
    if (!doc.contains("configs")) {
        logger_->get()->error("{} has no configs", list);
        return false;
    }
    auto const configs = doc.at("configs");
    if (!configs.is_array()) {
        logger_->get()->error("The configs in {} are not an array", list);
        return false;
    }
    // collected here and stored only once every entry has loaded, so a failed load stores
    // nothing
    std::unordered_map<std::string, boost::shared_ptr<kind::Json> > read;
    auto const items = configs.as_array();
    const auto* it = items.begin();
    for (; it != items.end(); ++it) {
        if (!it->is_object()) {
            logger_->get()->error("An entry in {} is not an object", list);
            return false;
        }
        auto const entry = it->as_object();
        if (!entry.contains("type") || !entry.contains("file")) {
            logger_->get()->error("An entry in {} needs both a type and a file", list);
            return false;
        }
        const std::optional<std::string> type = readString(entry, "type");
        const std::optional<std::string> file = readString(entry, "file");
        if (!type || !file) {
            logger_->get()->error("An entry in {} gives a type or a file that is not a string", list);
            return false;
        }
        if (type->empty()) {
            logger_->get()->error("The entry in {} for {} has an empty type", list, *file);
            return false;
        }
        // the manager returns null for a file that is missing, is not JSON, or has an
        // extension no loader handles, and logs which
        const boost::shared_ptr<kind::Json> asset = assets.load<kind::Json>(*file);
        if (!asset) {
            logger_->get()->error("{} names {}, which could not be loaded", list, *file);
            return false;
        }
        read[*type] = asset;
    }
    documents_ = std::move(read);
    return true;
}

/**
 **/
boost::shared_ptr<kind::Json> Manifest::get(std::string_view kind) const {
    auto entry = documents_.find(std::string(kind));
    if (entry == documents_.end()) {
        return boost::shared_ptr<kind::Json>();
    }
    return entry->second;
}

};  // namespace v3d::asset
