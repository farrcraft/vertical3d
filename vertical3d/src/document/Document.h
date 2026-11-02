/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <api/animation/Curve.h>
#include <api/log/Logger.h>
#include <api/type/camera/Profile.h>
#include <vertical3d/src/command/Stack.h>
#include <vertical3d/src/command/Topology.h>
#include <vertical3d/src/scene/item/Camera.h>
#include <vertical3d/src/scene/item/CreateItem.h>
#include <vertical3d/src/scene/item/Item.h>
#include <vertical3d/src/scene/item/Light.h>
#include <vertical3d/src/scene/RenderSettings.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/scene/Timeline.h>
#include <vertical3d/src/store/Project.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <boost/filesystem/path.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::editor::document {

/**
 * What an operation did, for whoever asked for it to say.
 **/
struct Outcome final {
    bool done = false;    /**< whether it did what it was asked, rather than refusing **/
    std::string message;  /**< what to tell the user, whether it changed or not **/
    bool undoable = false;  /**< whether it is a change undo takes back **/
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

    const boost::shared_ptr<scene::Scene>& scene() const noexcept;
    const boost::shared_ptr<command::Stack>& history() const noexcept;
    const boost::shared_ptr<store::Project>& project() const noexcept;

    /**
     * @return whether the document has changes no save kept
     **/
    bool dirty() const noexcept;

    /**
     * Put a new item into the scene.
     * @param name what the history calls the change
     **/
    Outcome create(const boost::shared_ptr<scene::item::Item>& item, const std::string& name);
    Outcome createPrimitive(Primitive primitive);

    /**
     * Make a camera or a light where a view is, facing the way it looks.
     **/
    Outcome createCamera(const v3d::type::camera::Profile& at, scene::item::Camera::Projection projection);
    Outcome createLight(const v3d::type::camera::Profile& at, scene::item::Light::Type type);

    /**
     * Make one curve of a type at the origin.
     **/
    Outcome createCurve(scene::item::CurveType type);

    /**
     * Gather items beneath a new group.
     **/
    Outcome group(const scene::Items& items);

    /**
     * Read a glTF file (.gltf, .glb) or a USD stage (.usd, .usda, .usdc, .usdz), and put its
     * scene into the document as one change. Its nodes become items, with their hierarchy and
     * their keys, and the items at the top become the selection. What the file holds that the
     * scene cannot is logged as a warning.
     **/
    Outcome importScene(const boost::filesystem::path& file);

    /**
     * Key every property of each item at a frame.
     **/
    Outcome key(const scene::Items& items, int frame);

    /**
     * Turn auto key on or off. While it is on, an edit to a number a curve can drive keys that
     * number at the current frame, in the same change. Setting it as it is changes nothing.
     **/
    Outcome autoKey(bool on);

    /**
     * Take a new frame rate and range. The frame stays where it is, held within the new range.
     * The same rate and range change nothing, and a rate that is not a positive number or a
     * range that runs backwards is refused.
     **/
    Outcome timing(float rate, int start, int end);

    /**
     * Move keys along by whole frames, or delete them. A key is every key one item has on one
     * frame. A key no curve has is skipped, and with none left nothing changes.
     **/
    Outcome moveKeys(const std::vector<scene::FrameKey>& keys, int offset);
    Outcome deleteKeys(const std::vector<scene::FrameKey>& keys);

    /**
     * Give one of an item's curves new keys, as a graph's drag leaves it. A path the item has no
     * curve on, or a curve the same as the one it has, changes nothing.
     **/
    Outcome reshape(unsigned int item, const std::string& path, const v3d::animation::Curve& curve);

    /**
     * Take new render settings. The same settings change nothing.
     **/
    Outcome settings(const scene::RenderSettings& settings);

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
     * Make a frame the current one, held within the timeline's range. Every pending edit to a
     * keyed property is dropped. A frame is not a change to the document, so it is not
     * recorded.
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
    /**
     * What a modelling operation says when it changed the mesh, and when there was nothing to
     * change.
     **/
    struct Said final {
        std::string done;
        std::string refused;
    };

    /**
     * @return a count of items, singular for one: "1 item", "3 items"
     **/
    static std::string items(std::size_t count);

    /**
     * @return a count of keys, singular for one: "1 key", "3 keys"
     **/
    static std::string keys(std::size_t count);

    /**
     * @return a kind's name with each word begun in capitals, as a new item is named: "point light"
     *         is "Point Light"
     **/
    static std::string titled(const std::string& kind);

    /**
     * Record a modelling operation's change, or say why there was none. A modelling operation has
     * changed the mesh by the time it hands its record back.
     **/
    static Outcome record(command::Stack* history, const boost::shared_ptr<command::Topology>& command, const Said& said);

    boost::shared_ptr<v3d::log::Logger> logger_;
    boost::shared_ptr<scene::Scene> scene_;
    boost::shared_ptr<command::Stack> history_;
    boost::shared_ptr<store::Project> project_;
    // milliseconds since the last autosave
    unsigned int sinceAutosave_;
};

};  // namespace v3d::editor::document
