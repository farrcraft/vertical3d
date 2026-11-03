/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "EditCommands.h"

#include <api/type/camera/Profile.h>
#include <vertical3d/src/app/CommandTable.h>
#include <vertical3d/src/app/Context.h>
#include <vertical3d/src/document/Document.h>
#include <vertical3d/src/scene/item/Camera.h>
#include <vertical3d/src/scene/item/CreateItem.h>
#include <vertical3d/src/scene/item/Light.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/scene/Timeline.h>
#include <vertical3d/src/script/Arguments.h>
#include <vertical3d/src/selection/SelectMask.h>

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace v3d::editor::app {

/**
 **/
EditCommands::EditCommands(const Context& context) :
    context_(context) {
}

/**
 **/
void EditCommands::registerCommands(CommandTable* table) const {
    registerCreate(table);
    registerModelling(table);
    const Context& context = context_;
    table->script("timeline::set", { { "items", script::Type::Items }, { "frame", script::Type::Integer } }, [&context](script::Arguments* arguments) {
        const std::optional<scene::Items> items = context.items(arguments, "items", "set key");
        if (items) {
            const int frame = arguments->integer("frame", [&context]() { return context.document->scene()->timeline().frame; });
            context.report(context.document->key(*items, frame));
        }
    });
    table->script("timeline::delete_keys", [&context]() {
        if (context.timeline) {
            context.timeline->deleteChosen();
        }
    });
    table->script("timeline::auto_key", { { "on", script::Type::Boolean } }, [&context](script::Arguments* arguments) {
        context.report(context.document->autoKey(arguments->boolean("on", [&context]() { return !context.document->scene()->timeline().autoKey; })));
    });
    table->script("edit::undo", [&context]() { context.report(context.document->undo()); });
    table->script("edit::redo", [&context]() { context.report(context.document->redo()); });
}

/**
 **/
void EditCommands::registerCreate(CommandTable* table) const {
    const Context& context = context_;
    for (const auto& [name, primitive] : std::vector<std::pair<const char*, document::Document::Primitive>>{
        { "cube", document::Document::Primitive::Cube }, { "plane", document::Document::Primitive::Plane },
        { "cylinder", document::Document::Primitive::Cylinder }, { "cone", document::Document::Primitive::Cone } }) {
        table->script(std::string("create::poly::") + name, [&context, primitive]() {
            context.report(context.document->createPrimitive(primitive));
        });
    }
    for (const auto& [name, projection] : std::vector<std::pair<const char*, scene::item::Camera::Projection>>{
        { "create::perspective::camera", scene::item::Camera::Projection::Perspective },
        { "create::ortho::camera", scene::item::Camera::Projection::Orthographic } }) {
        table->script(name, [&context, projection]() {
            context.atView("a camera is made", [&context, projection](const v3d::type::camera::Profile& at) {
                context.report(context.document->createCamera(at, projection));
            });
        });
    }
    for (const auto& [name, type] : std::vector<std::pair<const char*, scene::item::Light::Type>>{
        { "create::light::point", scene::item::Light::Type::Point }, { "create::light::distant", scene::item::Light::Type::Distant },
        { "create::light::spot", scene::item::Light::Type::Spot } }) {
        table->script(name, [&context, type]() {
            context.atView("a light is made", [&context, type](const v3d::type::camera::Profile& at) {
                context.report(context.document->createLight(at, type));
            });
        });
    }
    for (const auto& [name, type] : std::vector<std::pair<const char*, scene::item::CurveType>>{
        { "create::curve::poly", scene::item::CurveType::Poly }, { "create::curve::bezier", scene::item::CurveType::Bezier },
        { "create::curve::catmull_rom", scene::item::CurveType::CatmullRom }, { "create::curve::nurbs", scene::item::CurveType::Nurbs } }) {
        table->script(name, [&context, type]() { context.report(context.document->createCurve(type)); });
    }
    table->script("create::group", { { "items", script::Type::Items } }, [&context](script::Arguments* arguments) {
        if (const std::optional<scene::Items> items = context.items(arguments, "items", "group")) {
            context.report(context.document->group(*items));
        }
    });
}

/**
 **/
void EditCommands::registerModelling(CommandTable* table) const {
    const Context& context = context_;
    // each modelling operation takes the parts it works on
    const std::vector<script::Parameter> parts{ { "parts", script::Type::Parts } };
    table->script("transform::poly::split_edge", parts, [&context](script::Arguments* arguments) {
        context.masked(arguments, selection::SelectMask::Edge, "split edge", [&context]() { return context.document->splitEdges(); });
    });
    table->script("transform::poly::split_face", parts, [&context](script::Arguments* arguments) {
        context.masked(arguments, selection::SelectMask::Vertex, "split face", [&context]() { return context.document->splitFace(); });
    });
    table->script("transform::poly::extrude_face", parts, [&context](script::Arguments* arguments) {
        context.masked(arguments, selection::SelectMask::Face, "extrude face", [&context]() { return context.document->extrudeFaces(); });
    });
    table->script("transform::poly::inset_face", parts, [&context](script::Arguments* arguments) {
        context.masked(arguments, selection::SelectMask::Face, "inset face", [&context]() { return context.document->insetFaces(); });
    });
    table->script("transform::poly::merge_vertices", parts, [&context](script::Arguments* arguments) {
        context.masked(arguments, selection::SelectMask::Vertex, "merge vertices", [&context]() { return context.document->mergeVertices(); });
    });
}

};  // namespace v3d::editor::app
