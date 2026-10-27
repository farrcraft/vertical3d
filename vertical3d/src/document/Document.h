/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <api/log/Logger.h>
#include <api/type/camera/Profile.h>
#include <vertical3d/src/command/CommandStack.h>
#include <vertical3d/src/scene/Camera.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Light.h>
#include <vertical3d/src/scene/Project.h>
#include <vertical3d/src/scene/RenderSettings.h>
#include <vertical3d/src/scene/Scene.h>

#include <cstdint>
#include <string>
#include <vector>

#include <boost/filesystem/path.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 * What an operation did, for whoever asked for it to say.
 **/
struct Outcome final {
    bool done = false;    /**< whether it did what it was asked, rather than refusing **/
    std::string message;  /**< what to tell the user, whether it changed or not **/
};

/**
 * The document the editor edits: the scene, its history, and the project it is kept in, with
 * the saved position, the autosave and the recovery that go with them.
 *
 * Every change a user can ask for is an operation here. An operation applies itself, records
 * itself in the history, and says what it did. What it acts on is named in its arguments or
 * in its name, so a menu, a binding and a console that call one get the same change. An
 * operation that cannot run changes nothing and says why.
 *
 * A gesture is not an operation, because it changes the document as the cursor moves. A tool
 * records a gesture in history() when it ends.
 **/
class Document final {
 public:
    /** The primitives createPrimitive() makes. **/
    enum class Primitive : std::uint8_t { Cube, Plane, Cylinder, Cone };

    /**
     * @param project the directory of the project the document starts as, which is not read
     **/
    Document(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::filesystem::path& project);

    const boost::shared_ptr<Scene>& scene() const noexcept;
    const boost::shared_ptr<CommandStack>& history() const noexcept;
    const boost::shared_ptr<Project>& project() const noexcept;

    /**
     * @return whether the document has changes no save kept
     **/
    bool dirty() const noexcept;

    /**
     * Put a new item into the scene.
     * @param name what the history calls the change
     **/
    Outcome create(const boost::shared_ptr<Item>& item, const std::string& name);
    Outcome createPrimitive(Primitive primitive);

    /**
     * Make a camera or a light where a view is, facing the way it looks.
     **/
    Outcome createCamera(const v3d::type::camera::Profile& at, Camera::Projection projection);
    Outcome createLight(const v3d::type::camera::Profile& at, Light::Type type);

    /**
     * Gather items beneath a new group.
     **/
    Outcome group(const Items& items);

    /**
     * Key every property of each item at a frame.
     **/
    Outcome key(const Items& items, int frame);

    /**
     * Take new render settings. The same settings change nothing.
     **/
    Outcome settings(const RenderSettings& settings);

    /**
     * The modelling operations, on the scene's selected parts: split the selected edges at their
     * middles, split a face between its two selected vertices, merge the selected vertices,
     * and extrude or inset the selected faces.
     **/
    Outcome splitEdges();
    Outcome splitFace();
    Outcome mergeVertices();
    Outcome extrudeFaces();
    Outcome insetFaces();

    /**
     * Step the history one command back or forward.
     **/
    Outcome undo();
    Outcome redo();

    /**
     * Make a frame the current one, held within the timeline's range, and pose the scene at it.
     * A frame is not a change to the document, so it is not recorded.
     **/
    void goTo(int frame);

    /**
     * Read a project over the document. The history describes the scene it replaces, so it is
     * cleared, and the new document counts as saved.
     * @param manifest the project's manifest
     **/
    bool open(const boost::filesystem::path& manifest);

    /**
     * Write the document to its project, or to a new one it is then kept in.
     **/
    bool save();
    bool saveAs(const boost::filesystem::path& directory);

    /**
     * Autosave the document while it has changes, once every autosaveInterval milliseconds.
     * @param delta milliseconds since the last call
     **/
    void tick(unsigned int delta);

    /**
     * How often an autosave runs while the document has changes, in milliseconds.
     **/
    static constexpr unsigned int autosaveInterval = 60000;

    /**
     * @return whether the project's autosave holds changes no save kept
     **/
    bool recoverable() const;

    /**
     * Read the autosave over the document. What it holds is not what the project holds, so the
     * document is dirty after it.
     **/
    bool recover();

    /**
     * Throw the autosave away, so it is not offered again.
     **/
    void discardRecovery();

    /**
     * The editor is closing. A document with nothing unsaved leaves nothing to recover. One with
     * changes keeps its autosave, so they are offered back at the next start.
     **/
    void close();

    /**
     * Write the scene as RIB for the offline renderer, seen from a camera.
     * @param picture the picture the RIB names, which rendering it writes
     **/
    Outcome exportRib(const boost::filesystem::path& file, const v3d::type::camera::Profile& from,
        const std::string& picture);

 private:
    boost::shared_ptr<Scene> scene_;
    boost::shared_ptr<CommandStack> history_;
    boost::shared_ptr<Project> project_;
    // milliseconds since the last autosave
    unsigned int sinceAutosave_;
};

};  // namespace v3d::editor
