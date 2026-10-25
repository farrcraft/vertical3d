/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Project.h"

#include <api/asset/File.h>
#include <api/asset/Json.h>
#include <api/asset/Migration.h>
#include <api/asset/Writer.h>
#include <api/log/Logger.h>

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

#include "Scene.h"
#include "SceneStore.h"

#include <boost/filesystem.hpp>
#include <boost/json.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::editor {

namespace {

/**
 * What the manifest's "format" says, so a JSON file that is not a manifest is refused.
 **/
const char* const FORMAT = "v3d-project";

/**
 * The scene a new project has.
 **/
const char* const FIRST_SCENE = "scenes/main.v3d";

/**
 * Whether a path in the manifest names a file inside the project. An absolute path, or one
 * that climbs out with "..", would let a manifest write a scene anywhere.
 **/
bool inside(const std::string& entry) {
    const boost::filesystem::path path(entry);
    if (path.empty() || !path.is_relative() || path.has_root_name() || !path.has_filename()) {
        return false;
    }
    return std::none_of(path.begin(), path.end(),
        [](const boost::filesystem::path& part) { return part == ".." || part == "."; });
}

/**
 * What a manifest holds.
 **/
struct Manifest {
    std::string name;
    std::vector<std::string> scenes;
};

/**
 * Read a manifest.
 * @return nothing when it is not a manifest this build can read, with the problem logged
 **/
std::optional<Manifest> readManifest(const boost::filesystem::path& path, const boost::shared_ptr<v3d::log::Logger>& logger) {
    const std::optional<std::string> text = v3d::asset::readFile(path.string());
    if (!text || text->empty()) {
        logger->get()->error("No project manifest to read at {}", path.string());
        return std::nullopt;
    }
    boost::system::error_code error;
    const boost::json::value document = boost::json::parse(*text, error);
    if (error || !document.is_object()) {
        logger->get()->error("{} is not a project manifest: {}", path.string(), error.message());
        return std::nullopt;
    }

    // an older manifest is migrated one version at a time. The format has one version, so the
    // migration chain is empty
    boost::json::object root = document.as_object();
    switch (v3d::asset::readForward(&root, Project::VERSION, {})) {
        case v3d::asset::Reading::Current:
        case v3d::asset::Reading::Migrated:
            break;
        case v3d::asset::Reading::Newer:
            logger->get()->error("{} was written by a later build, which this one cannot read", path.string());
            return std::nullopt;
        case v3d::asset::Reading::Refused:
            logger->get()->error("{} has no version this build can read", path.string());
            return std::nullopt;
    }

    if (v3d::asset::readString(root, "format") != std::optional<std::string>(FORMAT)) {
        logger->get()->error("{} is not a project manifest", path.string());
        return std::nullopt;
    }
    Manifest manifest;
    const std::optional<std::string> name = v3d::asset::readString(root, "name");
    if (!name) {
        logger->get()->error("{} does not name its project", path.string());
        return std::nullopt;
    }
    manifest.name = *name;
    const boost::json::array* scenes = v3d::asset::readArray(root, "scenes");
    if (scenes == nullptr || scenes->empty()) {
        logger->get()->error("{} lists no scenes", path.string());
        return std::nullopt;
    }
    for (const boost::json::value& entry : *scenes) {
        const std::string scene = entry.is_string() ? std::string(entry.as_string()) : std::string();
        if (!inside(scene)) {
            logger->get()->error("{} lists a scene that is not a path inside the project", path.string());
            return std::nullopt;
        }
        manifest.scenes.push_back(scene);
    }
    return manifest;
}

};  // namespace

/**
 **/
const int Project::VERSION = 1;

/**
 **/
const char* const Project::MANIFEST = "project.json";

/**
 **/
const std::vector<std::string>& Project::layout() {
    static const std::vector<std::string> directories{ "scenes", "assets", "cache", "renders", "autosave" };
    return directories;
}

/**
 **/
Project::Project(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::filesystem::path& directory) :
    logger_(logger),
    store_(logger),
    recoveryStore_(logger),
    directory_(directory),
    name_("untitled"),
    scenes_{ FIRST_SCENE } {
}

/**
 **/
bool Project::open(const boost::filesystem::path& manifest, const boost::shared_ptr<Scene>& scene) {
    if (!scene) {
        return false;
    }
    const std::optional<Manifest> read = readManifest(manifest, logger_);
    if (!read) {
        return false;
    }
    const boost::filesystem::path directory = manifest.parent_path();
    if (!store_.read((directory / read->scenes.front()).string(), scene.get())) {
        return false;
    }
    directory_ = directory;
    name_ = read->name;
    scenes_ = read->scenes;
    logger_->get()->info("opened {} from {}", name_, directory_.string());
    return true;
}

/**
 **/
bool Project::save(const boost::shared_ptr<Scene>& scene) {
    if (!write(directory_, scene)) {
        return false;
    }
    // the document holds everything the recovery database did
    discardRecovery();
    return true;
}

/**
 **/
bool Project::saveAs(const boost::filesystem::path& directory, const boost::shared_ptr<Scene>& scene) {
    boost::system::error_code error;
    if (boost::filesystem::exists(directory, error)) {
        logger_->get()->error("A new project cannot go at {}, which is already there", directory.string());
        return false;
    }
    if (!write(directory, scene)) {
        return false;
    }
    discardRecovery();
    directory_ = directory;
    return true;
}

/**
 **/
bool Project::autosave(const boost::shared_ptr<Scene>& scene) {
    if (!scene) {
        return false;
    }
    const boost::filesystem::path path = recovery();
    boost::system::error_code error;
    boost::filesystem::create_directories(path.parent_path(), error);
    if (error) {
        logger_->get()->error("Cannot make {}: {}", path.parent_path().string(), error.message());
        return false;
    }
    return recoveryStore_.write(path.string(), *scene);
}

/**
 **/
boost::filesystem::path Project::recovery() const {
    return directory_ / "autosave" / boost::filesystem::path(scenes_.front()).filename();
}

/**
 **/
bool Project::recoverable() const {
    boost::system::error_code error;
    const boost::filesystem::path path = recovery();
    if (!boost::filesystem::is_regular_file(path, error)) {
        return false;
    }
    const boost::filesystem::path document = scene();
    if (!boost::filesystem::exists(document, error)) {
        return true;
    }
    return boost::filesystem::last_write_time(path, error) > boost::filesystem::last_write_time(document, error);
}

/**
 **/
bool Project::recover(const boost::shared_ptr<Scene>& scene) {
    if (!scene) {
        return false;
    }
    // read through the recovery store, so the next autosave goes on from what it holds
    if (!recoveryStore_.read(recovery().string(), scene.get())) {
        return false;
    }
    logger_->get()->info("recovered {} from {}", name_, recovery().string());
    return true;
}

/**
 **/
void Project::discardRecovery() {
    boost::system::error_code error;
    boost::filesystem::remove(recovery(), error);
}

/**
 **/
const SceneStore::Written& Project::autosaved() const noexcept {
    return recoveryStore_.written();
}

/**
 **/
const boost::filesystem::path& Project::directory() const noexcept {
    return directory_;
}

/**
 **/
boost::filesystem::path Project::scene() const {
    return directory_ / scenes_.front();
}

/**
 **/
const SceneStore::Written& Project::written() const noexcept {
    return store_.written();
}

/**
 **/
const std::string& Project::name() const noexcept {
    return name_;
}

/**
 **/
void Project::name(const std::string& title) {
    name_ = title;
}

/**
 **/
bool Project::write(const boost::filesystem::path& directory, const boost::shared_ptr<Scene>& scene) {
    if (!scene) {
        return false;
    }
    boost::system::error_code error;
    for (const std::string& part : layout()) {
        boost::filesystem::create_directories(directory / part, error);
        if (error) {
            logger_->get()->error("Cannot make {}: {}", (directory / part).string(), error.message());
            return false;
        }
    }
    const boost::filesystem::path scenePath = directory / scenes_.front();
    boost::filesystem::create_directories(scenePath.parent_path(), error);
    if (error || !store_.write(scenePath.string(), *scene)) {
        return false;
    }

    // written after the scene, so a manifest is never left naming a scene that was not written
    boost::json::array scenes;
    for (const std::string& entry : scenes_) {
        scenes.emplace_back(entry);
    }
    boost::json::object root;
    root["version"] = VERSION;
    root["format"] = FORMAT;
    root["name"] = name_;
    root["scenes"] = scenes;
    if (!v3d::asset::writeDocument(directory / MANIFEST, root)) {
        logger_->get()->error("Failed writing the project manifest in {}", directory.string());
        return false;
    }
    logger_->get()->info("saved {} to {}", name_, directory.string());
    return true;
}

};  // namespace v3d::editor
