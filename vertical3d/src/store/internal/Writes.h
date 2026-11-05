/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/database/Database.h>
#include <api/database/Statement.h>
#include <vertical3d/src/store/internal/Inserts.h>

#include <initializer_list>
#include <string>
#include <utility>

namespace v3d::editor::store::internal {

/**
 * The statements a write runs.
 **/
struct Writes {
    explicit Writes(v3d::database::Database* database) :
        inserts{
            .item = database->prepare(
                "INSERT INTO items (id, position, kind, type_version, tx, ty, tz, rx, ry, rz, rw, sx, sy, sz, payload, parent, name, views, renders) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"),
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

};  // namespace v3d::editor::store::internal
