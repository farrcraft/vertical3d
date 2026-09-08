/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Settings.h"

#include <string>

#include "Application.h"

#include "../asset/Json.h"
#include "../asset/Manager.h"
#include "../asset/Writer.h"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/system/error_code.hpp>

namespace v3d::engine {

namespace {

/**
 * The document within userPath(). One name for every app, because the directory is already
 * this app's.
 **/
const char* const DOCUMENT = "settings.json";

/**
 * The top level key the settings sit under. They are not at the root so that "version" is
 * the format's and an app can still have a setting called version.
 **/
const char* const ENTRIES = "settings";

/**
 **/
const char* const FORMAT = "version";

};  // namespace

/**
 **/
const int Settings::VERSION = 1;

/**
 **/
Settings::Settings(const std::string& org, const std::string& app,
    const boost::shared_ptr<v3d::log::Logger>& logger) :
    logger_(logger),
    directory_(userPath(org, app)) {
    if (!directory_.empty()) {
        path_ = (boost::filesystem::path(directory_) / DOCUMENT).string();
    }
}

/**
 **/
bool Settings::load() {
    entries_.clear();
    writable_ = true;
    if (path_.empty() || !boost::filesystem::exists(path_)) {
        return false;
    }

    // through the loader every other document is read with; it answers null for one it
    // could not parse
    v3d::asset::Manager assets(directory_, logger_);
    const boost::shared_ptr<v3d::asset::Json> document =
        boost::dynamic_pointer_cast<v3d::asset::Json>(assets.loadTypeFromExt(DOCUMENT));
    if (!document) {
        logger_->get()->error("{} is not a settings document - running on defaults", path_);
        return false;
    }

    const boost::json::object& root = document->document();
    boost::system::error_code error;
    const int version = root.contains(FORMAT) ? root.at(FORMAT).to_number<int>(error) : 0;
    if (!error && version > VERSION) {
        // a later build wrote it and knows what is in it; overwriting would cost that build
        // everything it stored
        logger_->get()->warn("{} is version {} and this build writes {} - running on defaults",
            path_, version, VERSION);
        writable_ = false;
        return false;
    }

    if (!root.contains(ENTRIES) || !root.at(ENTRIES).is_object()) {
        logger_->get()->error("{} holds no settings - running on defaults", path_);
        return false;
    }
    entries_ = root.at(ENTRIES).as_object();
    return true;
}

/**
 **/
bool Settings::save() {
    if (!writable_ || path_.empty()) {
        return false;
    }

    boost::json::object root;
    root[FORMAT] = VERSION;
    root[ENTRIES] = entries_;
    if (!v3d::asset::writeDocument(path_, root)) {
        logger_->get()->error("Failed writing settings to {}", path_);
        return false;
    }
    return true;
}

/**
 **/
const std::string& Settings::path() const noexcept {
    return path_;
}

/**
 **/
bool Settings::writable() const noexcept {
    return writable_;
}

/**
 **/
const boost::json::value* Settings::find(const std::string& key) const {
    const auto* const entry = entries_.find(key);
    return entry == entries_.end() ? nullptr : &entry->value();
}

/**
 **/
std::string Settings::text(const std::string& key, const std::string& fallback) const {
    const boost::json::value* value = find(key);
    if (value == nullptr || !value->is_string()) {
        return fallback;
    }
    return boost::json::value_to<std::string>(*value);
}

/**
 **/
double Settings::number(const std::string& key, double fallback) const {
    const boost::json::value* value = find(key);
    if (value == nullptr) {
        return fallback;
    }
    boost::system::error_code error;
    const double number = value->to_number<double>(error);
    return error ? fallback : number;
}

/**
 **/
int Settings::integer(const std::string& key, int fallback) const {
    const boost::json::value* value = find(key);
    if (value == nullptr) {
        return fallback;
    }
    // a number too large for an int, or one with a fraction, is a document somebody edited
    // into a shape this build cannot use rather than one to round
    boost::system::error_code error;
    const int number = value->to_number<int>(error);
    return error ? fallback : number;
}

/**
 **/
bool Settings::flag(const std::string& key, bool fallback) const {
    const boost::json::value* value = find(key);
    if (value == nullptr || !value->is_bool()) {
        return fallback;
    }
    return value->as_bool();
}

/**
 **/
void Settings::set(const std::string& key, const std::string& value) {
    entries_[key] = value;
}

/**
 **/
void Settings::set(const std::string& key, const char* value) {
    entries_[key] = value;
}

/**
 **/
void Settings::set(const std::string& key, double value) {
    entries_[key] = value;
}

/**
 **/
void Settings::set(const std::string& key, int value) {
    entries_[key] = value;
}

/**
 **/
void Settings::set(const std::string& key, bool value) {
    entries_[key] = value;
}

/**
 **/
void Settings::clear(const std::string& key) {
    entries_.erase(key);
}

};  // namespace v3d::engine
