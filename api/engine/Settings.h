/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "../log/Logger.h"

#include <boost/json.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::engine {

/**
 * What a player changed, in a document under userPath().
 *
 * An overlay rather than a copy: only a setting somebody touched is stored, so deleting the
 * file is a reset and a setting nobody touched keeps tracking whatever the build ships rather
 * than a snapshot of it taken the first time a menu was opened. A binding added in a later
 * build therefore reaches a player who already has a settings document.
 *
 * The schema is the app's. This knows a key, a value and a version, the way config::Config
 * does not know what a camera profile means. Applying a setting is elsewhere - a binding is
 * Engine::rebind(), a window size is Window::request().
 *
 * Written whole or not at all, per ADR-0041.
 **/
class Settings final {
 public:
    /**
     * The version written into every document, and the highest one this build will write to.
     **/
    static const int VERSION;

    /**
     * @param org the organization the app belongs to, as userPath() takes it
     * @param app what this app is called, and never changed once it has been chosen -
     *        changing it orphans every existing player's settings
     **/
    Settings(const std::string& org, const std::string& app,
        const boost::shared_ptr<v3d::log::Logger>& logger);

    /**
     * Read the document, replacing whatever is held.
     *
     * A missing file is the common case and not a failure: nothing is held and every read
     * gives its caller's default. A document this build does not understand is treated the
     * same way, and a document from a later version additionally makes this read-only, so a
     * player who downgrades keeps the settings the newer build wrote.
     *
     * @return whether a document was read
     **/
    bool load();

    /**
     * Write what is held, per ADR-0041.
     *
     * Call this when something changes rather than when the app exits: there is no exit path
     * that reliably runs, and a crash after a rebinding should not lose the rebinding.
     *
     * @return whether the document was written, which is false when load() found one from a
     *         later version
     **/
    bool save();

    /**
     * @return the document this reads and writes, whether or not it exists, or an empty
     *         string when the platform could not say where a user's files go
     **/
    const std::string& path() const noexcept;

    /**
     * @return false when load() found a document from a version this build does not know, in
     *         which case nothing here is written back
     **/
    bool writable() const noexcept;

    /**
     * The typed reads. Each gives back the fallback when the key is absent or holds
     * something of another kind, so a hand-edited document cannot make an app read a string
     * as a volume.
     **/
    std::string text(const std::string& key, const std::string& fallback) const;

    /**
     **/
    double number(const std::string& key, double fallback) const;

    /**
     **/
    int integer(const std::string& key, int fallback) const;

    /**
     **/
    bool flag(const std::string& key, bool fallback) const;

    /**
     * Store one setting, in the kind the matching read gives back.
     *
     * The const char* overload is not redundant: without it a string literal binds to the
     * bool one, because a pointer converts to bool by a standard conversion and to a
     * std::string only by a user defined one.
     **/
    void set(const std::string& key, const std::string& value);

    /**
     **/
    void set(const std::string& key, const char* value);

    /**
     **/
    void set(const std::string& key, double value);

    /**
     **/
    void set(const std::string& key, int value);

    /**
     **/
    void set(const std::string& key, bool value);

    /**
     * Forget one setting, so it tracks the shipped default again.
     **/
    void clear(const std::string& key);

 private:
    /**
     * @return the stored value for a key, or null when there is none
     **/
    const boost::json::value* find(const std::string& key) const;

    boost::shared_ptr<v3d::log::Logger> logger_;
    std::string directory_;
    std::string path_;
    // every entry read, including one this build does not know: a key belonging to a
    // feature added later survives a save by a build that has never heard of it
    boost::json::object entries_;
    bool writable_ = true;
};

};  // namespace v3d::engine
