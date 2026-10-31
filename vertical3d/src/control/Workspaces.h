/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>

#include <cstddef>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <boost/json.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 * The named workspaces, each an area layout, and what each project was last arranged as.
 *
 * The editor's data gives the workspaces a new user starts with. A user who keeps or imports a
 * workspace has a set of their own from then on. The settings store that set, and it replaces
 * the shipped set. A workspace is named by its layout's name, and
 * two workspaces never share one.
 *
 * Each project is recorded by its directory's path: the workspace it was last shown in, and
 * the arrangement it had in each workspace it was shown in. A record is kept for as long as its
 * directory exists.
 *
 * The documents this reads and writes:
 *
 * - a set of workspaces, as the editor's data and an exported file hold it, is an object whose
 *   "workspaces" is an array of layouts, each with a name, as AreaLayout reads them
 * - the projects, as the settings hold them, is an object from each project's path to its
 *   record: its "workspace", and its "layouts", an object from a workspace's name to the
 *   arrangement it had
 **/
class Workspaces final {
 public:
    /**
     * The settings keys this is stored under: the user's own set, the projects' records, and the
     * workspace last shown.
     **/
    static constexpr std::string_view SAVED = "workspaces";
    static constexpr std::string_view PROJECTS = "projects";
    static constexpr std::string_view LAST = "workspace";

    /**
     * What the settings hold, either of which may be null: the user's own set, and the
     * projects' records.
     **/
    struct Stored final {
        const boost::json::value* saved = nullptr;
        const boost::json::value* projects = nullptr;
    };

    /**
     * A project, by its directory's path, and one of the workspaces it is shown in.
     **/
    struct Shown final {
        std::string project;
        std::string workspace;
    };

    /**
     * What a project was last arranged as.
     **/
    struct Record final {
        std::string workspace;                              /**< the workspace it was last shown in **/
        std::map<std::string, boost::json::object> layouts;   /**< its arrangement in each workspace it was shown in, by name **/
    };

    explicit Workspaces(const boost::shared_ptr<v3d::log::Logger>& logger);

    /**
     * Read the set the editor ships with, and start from it. A layout that cannot be read, or
     * that has no name or the name of one before it, is left out.
     *
     * @return whether at least one workspace was read
     **/
    bool load(const boost::json::object& shipped);

    /**
     * Take what the settings hold. Anything of the wrong shape is left out.
     **/
    void restore(const Stored& stored);

    /**
     * @return the workspaces, the user's own set when there is one and the shipped set otherwise
     **/
    const std::vector<boost::json::object>& all() const noexcept;

    /**
     * @return the index in all() of the workspace with a name, or all().size() when none has it
     **/
    std::size_t find(std::string_view name) const;

    /**
     * Keep a layout as a workspace of its name, replacing one of that name or adding one. The
     * user has a set of their own from then on.
     *
     * @return whether the layout has a name and could be read
     **/
    bool keep(const boost::json::object& layout);

    /**
     * Put a workspace back as the editor ships it, when it ships one of that name.
     * @return whether it did
     **/
    bool reset(std::string_view name);

    /**
     * @return the user's own set, as the settings store it, or nothing when the user has none
     **/
    std::optional<boost::json::value> saved() const;

    /**
     * Record what a project is arranged as in the workspace it shows.
     **/
    void remember(const Shown& shown, const boost::json::object& layout);

    /**
     * Forget a project's arrangement in one workspace, so it shows the workspace as it is kept.
     **/
    void forget(const Shown& shown);

    /**
     * @return what a project was last arranged as, or nothing when it has no record
     **/
    std::optional<Record> recall(const std::string& project) const;

    /**
     * Drop the record of each project whose directory no longer exists.
     * @return how many were dropped
     **/
    std::size_t prune(const std::function<bool(const std::string& project)>& exists);

    /**
     * @return the projects' records, as the settings store them
     **/
    boost::json::value projects() const;

    /**
     * @return a file that holds one workspace, as import() reads it
     **/
    static boost::json::object exported(const boost::json::object& layout);

    /**
     * Keep each workspace a file holds, as keep() does.
     * @return the names of the workspaces kept, in the file's order
     **/
    std::vector<std::string> imported(const boost::json::object& file);

 private:
    /**
     * @return the workspaces a set holds that can be read and have names of their own
     **/
    std::vector<boost::json::object> read(const boost::json::value& set) const;

    /**
     * @return whether a layout can be read and has a name
     **/
    bool readable(const boost::json::object& layout) const;

    boost::shared_ptr<v3d::log::Logger> logger_;
    std::vector<boost::json::object> shipped_;
    std::vector<boost::json::object> all_;
    bool own_ = false;   // whether all_ is the user's own set
    std::map<std::string, Record> projects_;
};

};  // namespace v3d::editor
