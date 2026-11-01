/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "EditCommands.h"

#include <api/type/camera/Profile.h>
#include <vertical3d/src/app/CommandTable.h>
#include <vertical3d/src/app/Context.h>
#include <vertical3d/src/document/Document.h>
#include <vertical3d/src/scene/Camera.h>
#include <vertical3d/src/scene/CreateItem.h>
#include <vertical3d/src/scene/Light.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/scene/Timeline.h>
#include <vertical3d/src/selection/SelectMask.h>

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
    table->press("timeline::set", [&context]() {
        context.report(context.document->key(context.document->scene()->selection(), context.document->scene()->timeline().frame));
    });
    table->press("timeline::delete_keys", [&context]() {
        if (context.timeline) {
            context.timeline->deleteChosen();
        }
    });
    table->press("timeline::auto_key", [&context]() {
        context.report(context.document->autoKey(!context.document->scene()->timeline().autoKey));
    });
    table->press("edit::undo", [&context]() { context.report(context.document->undo()); });
    table->press("edit::redo", [&context]() { context.report(context.document->redo()); });
}

/**
 **/
void EditCommands::registerCreate(CommandTable* table) const {
    const Context& context = context_;
    for (const auto& [name, primitive] : std::vector<std::pair<const char*, document::Document::Primitive>>{
        { "cube", document::Document::Primitive::Cube }, { "plane", document::Document::Primitive::Plane },
        { "cylinder", document::Document::Primitive::Cylinder }, { "cone", document::Document::Primitive::Cone } }) {
        table->press(std::string("create::poly::") + name, [&context, primitive]() {
            context.report(context.document->createPrimitive(primitive));
        });
    }
    for (const auto& [name, projection] : std::vector<std::pair<const char*, scene::Camera::Projection>>{
        { "create::perspective::camera", scene::Camera::Projection::Perspective },
        { "create::ortho::camera", scene::Camera::Projection::Orthographic } }) {
        table->press(name, [&context, projection]() {
            context.atView("a camera is made", [&context, projection](const v3d::type::camera::Profile& at) {
                context.report(context.document->createCamera(at, projection));
            });
        });
    }
    for (const auto& [name, type] : std::vector<std::pair<const char*, scene::Light::Type>>{
        { "create::light::point", scene::Light::Type::Point }, { "create::light::distant", scene::Light::Type::Distant },
        { "create::light::spot", scene::Light::Type::Spot } }) {
        table->press(name, [&context, type]() {
            context.atView("a light is made", [&context, type](const v3d::type::camera::Profile& at) {
                context.report(context.document->createLight(at, type));
            });
        });
    }
    for (const auto& [name, type] : std::vector<std::pair<const char*, scene::CurveType>>{
        { "create::curve::poly", scene::CurveType::Poly }, { "create::curve::bezier", scene::CurveType::Bezier },
        { "create::curve::catmull_rom", scene::CurveType::CatmullRom }, { "create::curve::nurbs", scene::CurveType::Nurbs } }) {
        table->press(name, [&context, type]() { context.report(context.document->createCurve(type)); });
    }
    table->press("create::group", [&context]() { context.report(context.document->group(context.document->scene()->selection())); });
}

/**
 **/
void EditCommands::registerModelling(CommandTable* table) const {
    const Context& context = context_;
    table->press("transform::poly::split_edge", [&context]() {
        context.masked(selection::SelectMask::Edge, "split edge", [&context]() { return context.document->splitEdges(); });
    });
    table->press("transform::poly::split_face", [&context]() {
        context.masked(selection::SelectMask::Vertex, "split face", [&context]() { return context.document->splitFace(); });
    });
    table->press("transform::poly::extrude_face", [&context]() {
        context.masked(selection::SelectMask::Face, "extrude face", [&context]() { return context.document->extrudeFaces(); });
    });
    table->press("transform::poly::inset_face", [&context]() {
        context.masked(selection::SelectMask::Face, "inset face", [&context]() { return context.document->insetFaces(); });
    });
    table->press("transform::poly::merge_vertices", [&context]() {
        context.masked(selection::SelectMask::Vertex, "merge vertices", [&context]() { return context.document->mergeVertices(); });
    });
}

};  // namespace v3d::editor::app
