/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "SceneStore.h"

#include <api/database/Binder.h>
#include <api/database/Columns.h>
#include <api/database/Database.h>
#include <api/database/Statement.h>
#include <api/database/Transaction.h>
#include <api/log/Logger.h>
#include <api/animation/Curve.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Properties.h>
#include <vertical3d/src/scene/RenderSettings.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/scene/Timeline.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <map>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Bytes.h"
#include "Codec.h"

#include <boost/filesystem.hpp>
#include <boost/json.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::editor {

namespace {

// blobs are written as the machine holds them, which is the order the format defines
/**
 * The tables of a scene document at SCHEMA_VERSION.
 *
 * An item's kind is not checked by the table, so a new kind needs no new schema. Its payload
 * must be JSON. A buffer belongs to one item and goes when the item does. A parent is checked
 * when a write commits rather than row by row, because a changed row is deleted and written
 * again while its children still name it.
 **/
const char* const SCHEMA =
"CREATE TABLE meta ("
"  key TEXT PRIMARY KEY NOT NULL,"
"  value ANY NOT NULL) STRICT;"
"CREATE TABLE items ("
"  id INTEGER PRIMARY KEY,"
"  position INTEGER NOT NULL,"
"  kind TEXT NOT NULL,"
"  type_version INTEGER NOT NULL,"
"  tx REAL NOT NULL, ty REAL NOT NULL, tz REAL NOT NULL,"
"  rx REAL NOT NULL, ry REAL NOT NULL, rz REAL NOT NULL, rw REAL NOT NULL,"
"  sx REAL NOT NULL, sy REAL NOT NULL, sz REAL NOT NULL,"
"  payload TEXT NOT NULL CHECK (json_valid(payload)),"
"  parent INTEGER REFERENCES items(id) DEFERRABLE INITIALLY DEFERRED) STRICT;"
"CREATE TABLE buffers ("
"  item INTEGER NOT NULL REFERENCES items(id) ON DELETE CASCADE,"
"  role TEXT NOT NULL,"
"  count INTEGER NOT NULL CHECK (count >= 0),"
"  data BLOB NOT NULL,"
"  PRIMARY KEY (item, role)) STRICT;"
"CREATE TABLE curves ("
"  owner INTEGER NOT NULL REFERENCES items(id) DEFERRABLE INITIALLY DEFERRED,"
"  property TEXT NOT NULL,"
"  type_version INTEGER NOT NULL,"
"  count INTEGER NOT NULL CHECK (count >= 0),"
"  keys BLOB NOT NULL,"
"  PRIMARY KEY (owner, property)) STRICT;";

/**
 * What takes a document's tables from version 1 to version 2: the parent column, which
 * version 1 does not have.
 **/
const char* const MIGRATE_FROM_1 =
"ALTER TABLE items ADD COLUMN parent INTEGER REFERENCES items(id) DEFERRABLE INITIALLY DEFERRED;";

/**
 * What takes a document's tables from version 2 to version 3: the curves table.
 **/
const char* const MIGRATE_FROM_2 =
"CREATE TABLE curves ("
"  owner INTEGER NOT NULL REFERENCES items(id) DEFERRABLE INITIALLY DEFERRED,"
"  property TEXT NOT NULL,"
"  type_version INTEGER NOT NULL,"
"  count INTEGER NOT NULL CHECK (count >= 0),"
"  keys BLOB NOT NULL,"
"  PRIMARY KEY (owner, property)) STRICT;";

/**
 * The size of one key of a curve: its frame, value, in slope and out slope, then its
 * interpolation and whether it is automatic, a byte each, and two bytes of padding.
 **/
constexpr std::size_t KEY_SIZE = (4 * sizeof(float)) + 4;

using v3d::database::Binder;
using v3d::database::Columns;

/**
 * Read the next three columns of a row as a vector.
 **/
glm::vec3 vector(Columns* row) {
    const auto x = static_cast<float>(row->real());
    const auto y = static_cast<float>(row->real());
    const auto z = static_cast<float>(row->real());
    return glm::vec3(x, y, z);
}

/**
 * A reference to an item's row, which is NULL for 0.
 **/
std::optional<std::int64_t> reference(std::int64_t id) {
    return id == 0 ? std::nullopt : std::optional<std::int64_t>(id);
}

/**
 * Whether a row's kind is at the version this build writes.
 *
 * An older version is taken forward here one step at a time. Every kind is at its first
 * version, so a version below it is not one any build wrote, and is refused.
 **/
bool current(const std::string& kind, std::int64_t version, std::int64_t latest, std::string* problem) {
    if (version > latest) {
        *problem = "its " + kind + " was written by a later build";
        return false;
    }
    if (version < latest) {
        *problem = "its " + kind + " has no version this build can read";
        return false;
    }
    return true;
}

/**
 * Check a document's header against this build.
 *
 * @param fresh set when the database is empty and has no header at all, so it can be made
 *        into a document. Null when that is not allowed
 * @param version set to the version of the document's tables
 **/
bool header(v3d::database::Database* database, bool* fresh, std::int32_t* version, std::string* problem) {
    std::int32_t application = 0;
    if (!database->applicationId(&application) || !database->userVersion(version)) {
        *problem = database->error();
        return false;
    }
    if (application == 0 && *version == 0 && fresh != nullptr) {
        v3d::database::Statement tables = database->prepare("SELECT count(*) FROM sqlite_schema");
        if (tables.step() == v3d::database::Statement::Step::Row && tables.integer(0) == 0) {
            *fresh = true;
            return true;
        }
    }
    if (application != SceneStore::APPLICATION_ID) {
        *problem = "it is not a scene document";
        return false;
    }
    if (*version > SceneStore::SCHEMA_VERSION) {
        *problem = "it was written by a later build";
        return false;
    }
    if (*version < 1) {
        *problem = "it has no schema version this build can read";
        return false;
    }
    return true;
}

/**
 * Take a document's tables forward to SCHEMA_VERSION, one version at a time.
 **/
bool migrate(v3d::database::Database* database, std::int32_t version) {
    if (version >= SceneStore::SCHEMA_VERSION) {
        return true;
    }
    return (version > 1 || database->execute(MIGRATE_FROM_1)) && (version > 2 || database->execute(MIGRATE_FROM_2)) &&
        database->userVersion(SceneStore::SCHEMA_VERSION);
}

/**
 * The item one row of the items table describes, without its id or its placement. The blobs
 * it read are added to read.
 **/
boost::shared_ptr<Item> readKind(std::int64_t id, const std::string& kind, std::int64_t version,
    const boost::json::object& payload, v3d::database::Statement* buffers, std::size_t* read, std::string* problem) {
    const Codec* codec = codecNamed(kind);
    if (codec == nullptr) {
        *problem = "its kind, " + kind + ", is not one this build reads";
        return nullptr;
    }
    if (!current(kind, version, codec->version(), problem)) {
        return nullptr;
    }
    Blobs blobs;
    if (codec->buffered()) {
        if (!buffers->reset() || !buffers->bindInteger(1, id)) {
            *problem = buffers->error();
            return nullptr;
        }
        while (buffers->step() == v3d::database::Statement::Step::Row) {
            Columns row(*buffers);
            const std::string role = row.text();
            const std::int64_t count = row.integer();
            const std::span<const std::byte> data = row.blob();
            blobs[role] = Blob{ .count = count, .data = std::vector<std::byte>(data.begin(), data.end()) };
        }
    }
    *read += blobs.size();
    return codec->read(payload, blobs, problem);
}

/**
 * The item a row of the items table describes, whole.
 **/
boost::shared_ptr<Item> readItem(const v3d::database::Statement& rows, v3d::database::Statement* buffers,
    std::int64_t* parent, std::size_t* read, std::string* problem) {
    Columns row(rows);
    const std::int64_t id = row.integer();
    const std::string kind = row.text();
    const std::int64_t version = row.integer();
    const glm::vec3 translation = vector(&row);
    const glm::vec3 axis = vector(&row);
    const auto w = static_cast<float>(row.real());
    const glm::vec3 scale = vector(&row);
    const std::string text = row.text();
    *parent = row.integer();

    if (id <= 0 || *parent < 0 || !finite(translation) || !finite(axis) || !std::isfinite(w) || !finite(scale)) {
        *problem = "its id or its placement is not valid";
        return nullptr;
    }
    boost::system::error_code error;
    const boost::json::value parsed = boost::json::parse(text, error);
    if (error || !parsed.is_object()) {
        *problem = "its payload is not an object";
        return nullptr;
    }
    boost::shared_ptr<Item> item = readKind(id, kind, version, parsed.as_object(), buffers, read, problem);
    if (!item) {
        return nullptr;
    }
    item->documentId(id);
    item->translation(translation);
    item->rotation(glm::quat(w, axis.x, axis.y, axis.z));
    item->scale(scale);
    return item;
}

/**
 * Every item one document holds, read before any of it reaches the scene.
 **/
struct Loaded {
    struct Animated {
        std::int64_t owner = 0;
        std::string path;
        v3d::animation::Curve curve;
    };

    Items items;
    std::vector<Animated> curves;
    Timeline timeline;
    RenderSettings render;
    // each child's parent, both by document id
    std::map<std::int64_t, std::int64_t> parents;
    std::int64_t next = 1;
};

/**
 * Whether every parent is an item the document holds, and no item is its own ancestor.
 **/
bool trees(const Loaded& loaded) {
    std::map<std::int64_t, bool> held;
    for (const boost::shared_ptr<Item>& item : loaded.items) {
        held[item->documentId()] = true;
    }
    for (const auto& [child, parent] : loaded.parents) {
        if (!held.contains(parent)) {
            return false;
        }
        // a walk up from an item that takes more steps than there are items has gone round
        std::size_t steps = 0;
        for (auto above = loaded.parents.find(child); above != loaded.parents.end();
            above = loaded.parents.find(above->second)) {
            if (++steps > loaded.items.size()) {
                return false;
            }
        }
    }
    return true;
}

bool load(v3d::database::Database* database, std::int32_t version, Loaded* loaded, std::string* problem) {
    // a document from before items had parents has none to read
    const std::string parent = version >= 2 ? "parent" : "NULL";
    v3d::database::Statement rows = database->prepare(
        "SELECT id, kind, type_version, tx, ty, tz, rx, ry, rz, rw, sx, sy, sz, payload, " + parent +
        " FROM items ORDER BY position, id");
    v3d::database::Statement buffers = database->prepare("SELECT role, count, data FROM buffers WHERE item = ?1");
    if (!rows.valid() || !buffers.valid()) {
        *problem = database->error();
        return false;
    }

    std::int64_t highest = 0;
    std::size_t blobs = 0;
    v3d::database::Statement::Step step = rows.step();
    for (; step == v3d::database::Statement::Step::Row; step = rows.step()) {
        std::string fault;
        std::int64_t above = 0;
        const boost::shared_ptr<Item> item = readItem(rows, &buffers, &above, &blobs, &fault);
        if (!item) {
            *problem = "item " + std::to_string(rows.integer(0)) + ": " + fault;
            return false;
        }
        loaded->items.push_back(item);
        if (above != 0) {
            loaded->parents[item->documentId()] = above;
        }
        highest = std::max(highest, item->documentId());
    }
    if (step == v3d::database::Statement::Step::Failed) {
        *problem = rows.error();
        return false;
    }
    if (!trees(*loaded)) {
        *problem = "its parents do not form a set of trees";
        return false;
    }

    // a blob no item's codec read is data this build does not read
    v3d::database::Statement count = database->prepare("SELECT count(*) FROM buffers");
    if (count.step() != v3d::database::Statement::Step::Row || static_cast<std::size_t>(count.integer(0)) != blobs) {
        *problem = "it holds blobs that belong to no item";
        return false;
    }

    v3d::database::Statement next = database->prepare("SELECT value FROM meta WHERE key = 'next_id'");
    if (next.step() != v3d::database::Statement::Step::Row || next.integer(0) <= highest) {
        *problem = "its next id is missing or is not above every id it holds";
        return false;
    }
    loaded->next = next.integer(0);
    return true;
}

/**
 * A curve's keys, from its blob.
 **/
bool decodeKeys(std::span<const std::byte> bytes, std::int64_t count, v3d::animation::Curve* curve,
    std::string* problem) {
    if (count < 0 || static_cast<std::size_t>(count) * KEY_SIZE != bytes.size()) {
        *problem = "its keys do not fill their blob exactly";
        return false;
    }
    Cursor cursor(bytes);
    std::vector<v3d::animation::Curve::Key> keys;
    for (std::int64_t index = 0; index < count; index++) {
        v3d::animation::Curve::Key key;
        key.frame = cursor.next<float>();
        key.value = cursor.next<float>();
        key.in = cursor.next<float>();
        key.out = cursor.next<float>();
        const auto interpolation = cursor.next<std::uint8_t>();
        const auto automatic = cursor.next<std::uint8_t>();
        static_cast<void>(cursor.next<std::uint16_t>());
        if (interpolation > static_cast<std::uint8_t>(v3d::animation::Curve::Interpolation::Bezier) || automatic > 1) {
            *problem = "a key's interpolation or its slopes are not ones this build reads";
            return false;
        }
        key.interpolation = static_cast<v3d::animation::Curve::Interpolation>(interpolation);
        key.automatic = automatic == 1;
        keys.push_back(key);
    }
    try {
        *curve = v3d::animation::Curve(keys);
    } catch (const std::invalid_argument&) {
        *problem = "its keys are not finite or do not rise in frame";
        return false;
    }
    return true;
}

/**
 * A curve's keys, as its blob.
 **/
std::vector<std::byte> encodeKeys(const v3d::animation::Curve& curve) {
    std::vector<std::byte> bytes;
    for (const v3d::animation::Curve::Key& key : curve.keys()) {
        put(&bytes, key.frame);
        put(&bytes, key.value);
        put(&bytes, key.in);
        put(&bytes, key.out);
        put(&bytes, static_cast<std::uint8_t>(key.interpolation));
        put(&bytes, static_cast<std::uint8_t>(key.automatic ? 1 : 0));
        put(&bytes, static_cast<std::uint16_t>(0));
    }
    return bytes;
}

/**
 * Every curve a document holds, once its items are read.
 **/
bool loadAnimation(v3d::database::Database* database, std::int32_t version, Loaded* loaded, std::string* problem) {
    std::map<std::int64_t, Item::Kind> kinds;
    for (const boost::shared_ptr<Item>& item : loaded->items) {
        kinds[item->documentId()] = item->kind();
    }
    // a document from before curves has none to read
    if (version >= 3) {
        v3d::database::Statement rows = database->prepare(
            "SELECT owner, property, type_version, count, keys FROM curves ORDER BY owner, property");
        if (!rows.valid()) {
            *problem = database->error();
            return false;
        }
        v3d::database::Statement::Step step = rows.step();
        for (; step == v3d::database::Statement::Step::Row; step = rows.step()) {
            Columns row(rows);
            Loaded::Animated animated;
            animated.owner = row.integer();
            animated.path = row.text();
            const std::int64_t curveVersion = row.integer();
            const std::int64_t count = row.integer();
            const std::span<const std::byte> keys = row.blob();
            std::string fault;
            const auto owner = kinds.find(animated.owner);
            if (owner == kinds.end() || !declares(owner->second, animated.path)) {
                fault = "it drives a property its owner does not have";
            } else if (current("curve", curveVersion, SceneStore::CURVE_VERSION, &fault) &&
                decodeKeys(keys, count, &animated.curve, &fault) && animated.curve.empty()) {
                fault = "it has no keys";
            }
            if (!fault.empty()) {
                *problem = "the curve " + animated.path + " of item " + std::to_string(animated.owner) + ": " + fault;
                return false;
            }
            loaded->curves.push_back(animated);
        }
        if (step == v3d::database::Statement::Step::Failed) {
            *problem = rows.error();
            return false;
        }
    }

    return true;
}

/**
 * One row of the meta table that holds a part of the timeline or the render settings.
 **/
bool readSetting(const v3d::database::Statement& meta, Loaded* loaded, std::string* problem) {
    const std::string key = meta.text(0);
    if (key == "rate") {
        loaded->timeline.rate = static_cast<float>(meta.real(1));
    } else if (key == "start") {
        loaded->timeline.start = static_cast<int>(meta.integer(1));
    } else if (key == "end") {
        loaded->timeline.end = static_cast<int>(meta.integer(1));
    } else if (key == "frame") {
        loaded->timeline.frame = static_cast<int>(meta.integer(1));
    } else if (key == "render_hider") {
        const std::string hider = meta.text(1);
        if (hider != "hidden" && hider != "raytrace") {
            *problem = "its render settings name a hider this build does not have";
            return false;
        }
        loaded->render.hider = hider == "raytrace" ? RenderSettings::Hider::RayTrace : RenderSettings::Hider::Hidden;
    } else {
        // a negative or oversized number is held just out of range, so valid() refuses it
        const auto number = static_cast<unsigned int>(
            std::clamp<std::int64_t>(meta.integer(1), 0, RenderSettings::LARGEST_SIDE + 1));
        if (key == "render_width") {
            loaded->render.size.x = number;
        } else if (key == "render_height") {
            loaded->render.size.y = number;
        } else {
            loaded->render.samples = number;
        }
    }
    return true;
}

/**
 * A document's timeline and render settings, from its meta table. Each part a document does
 * not hold keeps its default.
 **/
bool loadSettings(v3d::database::Database* database, Loaded* loaded, std::string* problem) {
    v3d::database::Statement meta = database->prepare(
        "SELECT key, value FROM meta WHERE key IN ('rate', 'start', 'end', 'frame', "
        "'render_width', 'render_height', 'render_samples', 'render_hider')");
    if (!meta.valid()) {
        *problem = database->error();
        return false;
    }
    while (meta.step() == v3d::database::Statement::Step::Row) {
        if (!readSetting(meta, loaded, problem)) {
            return false;
        }
    }
    if (!loaded->timeline.valid()) {
        *problem = "its timeline's rate, range or frame is not valid";
        return false;
    }
    if (!loaded->render.valid()) {
        *problem = "its render settings' size or samples are not valid";
        return false;
    }
    return true;
}

/**
 * The two inserts a write runs for each item.
 **/
struct Inserts {
    v3d::database::Statement item;
    v3d::database::Statement buffer;
};

/**
 * Write one item's row, and its blobs.
 **/
bool storeItem(Inserts* inserts, const boost::shared_ptr<Item>& item, std::int64_t position, std::int64_t parent) {
    const Codec& codec = codecOf(item->kind());
    const std::string payload = boost::json::serialize(codec.payload(*item));
    const glm::vec3 translation = item->translation();
    const glm::quat rotation = item->rotation();
    const glm::vec3 scale = item->scale();
    if (!Binder(&inserts->item).integer(item->documentId()).integer(position).text(std::string(codec.name()))
        .integer(codec.version())
        .reals({ translation.x, translation.y, translation.z, rotation.x, rotation.y, rotation.z, rotation.w })
        .reals({ scale.x, scale.y, scale.z }).text(payload).integer(reference(parent)).run()) {
        return false;
    }
    return std::ranges::all_of(codec.buffers(*item), [&](const Buffer& buffer) {
        return Binder(&inserts->buffer).integer(item->documentId()).text(buffer.role)
            .integer(static_cast<std::int64_t>(buffer.count)).blob(buffer.data).run();
    });
}

/**
 * The statements a write runs.
 **/
struct Writes {
    explicit Writes(v3d::database::Database* database) :
        inserts{
            .item = database->prepare(
                "INSERT INTO items (id, position, kind, type_version, tx, ty, tz, rx, ry, rz, rw, sx, sy, sz, payload, parent) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"),
            .buffer = database->prepare("INSERT INTO buffers (item, role, count, data) VALUES (?, ?, ?, ?)") },
        remove(database->prepare("DELETE FROM items WHERE id = ?")),
        move(database->prepare("UPDATE items SET position = ?, parent = ? WHERE id = ?")),
        next(database->prepare("INSERT OR REPLACE INTO meta (key, value) VALUES ('next_id', ?)")) {
    }

    bool valid() const {
        return inserts.item.valid() && inserts.buffer.valid() && remove.valid() && move.valid() && next.valid();
    }

    /**
     * The reason a statement failed, from whichever did.
     **/
    std::string error() const {
        for (const v3d::database::Statement* statement : { &inserts.item, &inserts.buffer, &remove, &move, &next }) {
            if (!statement->error().empty()) {
                return statement->error();
            }
        }
        return std::string();
    }

    Inserts inserts;
    v3d::database::Statement remove;
    v3d::database::Statement move;
    v3d::database::Statement next;
};

/**
 * Write the scene's changes since the items saved, or every item when nothing is saved, and the
 * next id. A deleted row takes its blobs with it.
 *
 * @param saved the items as the document holds them, which a whole write has emptied
 * @param after the items as the document will hold them once this commits
 **/
bool store(v3d::database::Database* database, const Scene& scene, const std::map<std::int64_t, SceneStore::Saved>& saved,
    std::map<std::int64_t, SceneStore::Saved>* after, SceneStore::Written* written, std::string* problem) {
    Writes writes(database);
    if (!writes.valid()) {
        *problem = database->error();
        return false;
    }

    std::int64_t position = 0;
    for (const boost::shared_ptr<Item>& item : scene.items()) {
        const std::int64_t id = item->documentId();
        const boost::shared_ptr<Item> above = scene.parent(item->id());
        const std::int64_t parent = above ? above->documentId() : 0;
        const auto found = saved.find(id);
        const bool known = found != saved.end();
        bool ran = true;
        if (known && found->second.item.lock() == item && found->second.revision == item->revision()) {
            if (found->second.position != position || found->second.parent != parent) {
                ran = Binder(&writes.move).integer(position).integer(reference(parent)).integer(id).run();
                written->moved++;
            }
        } else {
            ran = (!known || Binder(&writes.remove).integer(id).run()) && storeItem(&writes.inserts, item, position, parent);
            written->rows++;
        }
        if (!ran) {
            *problem = writes.error();
            return false;
        }
        (*after)[id] = SceneStore::Saved{ .item = item, .revision = item->revision(), .position = position, .parent = parent };
        position++;
    }

    for (const auto& [id, kept] : saved) {
        if (!after->contains(id)) {
            if (!Binder(&writes.remove).integer(id).run()) {
                *problem = writes.error();
                return false;
            }
            written->removed++;
        }
    }

    if (!Binder(&writes.next).integer(scene.nextDocumentId()).run()) {
        *problem = writes.error();
        return false;
    }
    return true;
}

/**
 * Write the curves that changed since the curves saved, or every curve when none is saved, and
 * the timeline. A curve no item has any more is deleted.
 *
 * @param after the curves as the document will hold them once this commits
 **/
bool storeAnimation(v3d::database::Database* database, const Scene& scene, const SceneStore::SavedCurves& saved,
    SceneStore::SavedCurves* after, SceneStore::Written* written, std::string* problem) {
    v3d::database::Statement upsert = database->prepare(
        "INSERT OR REPLACE INTO curves (owner, property, type_version, count, keys) VALUES (?, ?, ?, ?, ?)");
    v3d::database::Statement drop = database->prepare("DELETE FROM curves WHERE owner = ? AND property = ?");
    v3d::database::Statement meta = database->prepare("INSERT OR REPLACE INTO meta (key, value) VALUES (?, ?)");
    if (!upsert.valid() || !drop.valid() || !meta.valid()) {
        *problem = database->error();
        return false;
    }
    const auto failed = [&](const v3d::database::Statement& statement) {
        *problem = statement.error();
        return false;
    };

    for (const boost::shared_ptr<Item>& item : scene.items()) {
        for (const auto& [path, curve] : scene.curves(item->id())) {
            const auto key = std::make_pair(item->documentId(), path);
            const auto found = saved.find(key);
            if (found == saved.end() || !(found->second == curve)) {
                if (!Binder(&upsert).integer(item->documentId()).text(path).integer(SceneStore::CURVE_VERSION)
                    .integer(static_cast<std::int64_t>(curve.keys().size())).blob(encodeKeys(curve)).run()) {
                    return failed(upsert);
                }
                written->curves++;
            }
            (*after)[key] = curve;
        }
    }
    for (const auto& [key, curve] : saved) {
        if (!after->contains(key)) {
            if (!Binder(&drop).integer(key.first).text(key.second).run()) {
                return failed(drop);
            }
            written->curves++;
        }
    }

    const Timeline& timeline = scene.timeline();
    const RenderSettings& render = scene.renderSettings();
    const bool wrote = Binder(&meta).text("rate").real(timeline.rate).run() &&
        Binder(&meta).text("start").integer(timeline.start).run() &&
        Binder(&meta).text("end").integer(timeline.end).run() &&
        Binder(&meta).text("frame").integer(timeline.frame).run() &&
        Binder(&meta).text("render_width").integer(render.size.x).run() &&
        Binder(&meta).text("render_height").integer(render.size.y).run() &&
        Binder(&meta).text("render_samples").integer(render.samples).run() &&
        Binder(&meta).text("render_hider").text(render.hider == RenderSettings::Hider::RayTrace ? "raytrace" : "hidden").run();
    return wrote || failed(meta);
}

};  // namespace

/**
 **/
SceneStore::SceneStore(const boost::shared_ptr<v3d::log::Logger>& logger) :
    logger_(logger) {
}

/**
 **/
bool SceneStore::read(const std::string& path, Scene* scene) {
    if (scene == nullptr) {
        return false;
    }
    v3d::database::Database database;
    if (!database.open(path, v3d::database::Database::Mode::ReadOnly)) {
        logger_->get()->error("No scene to read at {}: {}", path, database.error());
        return false;
    }

    std::string problem;
    Loaded loaded;
    std::int32_t version = 0;
    if (!header(&database, nullptr, &version, &problem) || !load(&database, version, &loaded, &problem) ||
        !loadAnimation(&database, version, &loaded, &problem) || !loadSettings(&database, &loaded, &problem)) {
        logger_->get()->error("Cannot read the scene {}: {}", path, problem);
        return false;
    }

    // the scene is replaced only once the whole document is understood
    scene->clear();
    std::map<std::int64_t, unsigned int> ids;
    for (const boost::shared_ptr<Item>& item : loaded.items) {
        ids[item->documentId()] = scene->add(item);
    }
    for (const auto& [child, parent] : loaded.parents) {
        scene->parent(ids.at(child), ids.at(parent));
    }
    curves_.clear();
    for (const Loaded::Animated& animated : loaded.curves) {
        scene->curve(ids.at(animated.owner), animated.path, animated.curve);
        curves_[std::make_pair(animated.owner, animated.path)] = animated.curve;
    }
    scene->nextDocumentId(loaded.next);
    scene->timeline(loaded.timeline);
    scene->renderSettings(loaded.render);
    scene->frame(loaded.timeline.frame);

    // the document holds what was just read, so the next write to it starts from here
    path_ = path;
    saved_.clear();
    std::int64_t position = 0;
    for (const boost::shared_ptr<Item>& item : loaded.items) {
        const auto above = loaded.parents.find(item->documentId());
        saved_[item->documentId()] = Saved{ .item = item, .revision = item->revision(), .position = position++,
            .parent = above == loaded.parents.end() ? 0 : above->second };
    }
    logger_->get()->info("read {} items from {}", scene->count(), path);
    return true;
}

/**
 **/
bool SceneStore::write(const std::string& path, const Scene& scene) {
    boost::system::error_code error;
    const bool existed = boost::filesystem::exists(path, error);

    std::string problem;
    bool written = false;
    Written what;
    std::map<std::int64_t, Saved> after;
    SavedCurves animated;
    {
        v3d::database::Database database;
        bool fresh = false;
        std::int32_t version = 0;
        if (!database.open(path, v3d::database::Database::Mode::Create)) {
            problem = database.error();
        } else if (header(&database, &fresh, &version, &problem)) {
            // only the document this store last wrote or read can take just the changes.
            // Anything else, a document another store wrote included, is written whole
            what.whole = fresh || !existed || path != path_;
            const std::map<std::int64_t, Saved> none;
            v3d::database::Transaction transaction(&database);
            written = transaction.active() &&
                (fresh ? database.execute(SCHEMA) && database.applicationId(APPLICATION_ID) &&
                    database.userVersion(SCHEMA_VERSION) : migrate(&database, version)) &&
                (!what.whole || database.execute("DELETE FROM curves; DELETE FROM buffers; DELETE FROM items; DELETE FROM meta;")) &&
                store(&database, scene, what.whole ? none : saved_, &after, &what, &problem) &&
                storeAnimation(&database, scene, what.whole ? SavedCurves() : curves_, &animated, &what, &problem) &&
                transaction.commit();
            if (!written && problem.empty()) {
                problem = database.error();
            }
        }
    }

    if (!written) {
        // a document that this write made is not left behind empty, and a document that may
        // now be other than the store remembers is written whole next time
        if (!existed) {
            boost::filesystem::remove(path, error);
        }
        path_.clear();
        saved_.clear();
        curves_.clear();
        logger_->get()->error("Cannot write the scene {}: {}", path, problem);
        return false;
    }
    path_ = path;
    saved_ = std::move(after);
    curves_ = std::move(animated);
    written_ = what;
    logger_->get()->info("wrote {} of {} items to {}", what.rows, scene.count(), path);
    return true;
}

/**
 **/
const SceneStore::Written& SceneStore::written() const noexcept {
    return written_;
}

};  // namespace v3d::editor
