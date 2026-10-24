/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
**/

#include "Controller.h"

#include <api/brep/BRep.h>
#include <api/config/CameraProfiles.h>
#include <api/config/Type.h>
#include <api/engine/Engine.h>
#include <api/engine/Statistics.h>
#include <api/event/Event.h>
#include <api/event/State.h>
#include <api/event/kind/MouseMotion.h>
#include <api/event/kind/WindowResize.h>
#include <api/render/realtime/Window.h>
#include <api/ui/Component.h>
#include <api/ui/Container.h>
#include <api/ui/Engine.h>
#include <api/ui/component/Button.h>
#include <api/ui/component/Toolbar.h>
#include <api/ui/component/menu/Item.h>
#include <api/ui/input/Cursor.h>
#include <api/ui/shell/FileChooser.h>
#include <api/ui/shell/Keyboard.h>
#include <api/ui/shell/Prompt.h>
#include <vertical3d/src/command/Command.h>
#include <vertical3d/src/command/CommandStack.h>
#include <vertical3d/src/command/CreateCommand.h>
#include <vertical3d/src/command/TopologyCommand.h>
#include <vertical3d/src/scene/CreatePoly.h>
#include <vertical3d/src/scene/Modelling.h>
#include <vertical3d/src/scene/Project.h>
#include <vertical3d/src/scene/RIBExportVisitor.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/tool/CameraControlTool.h>
#include <vertical3d/src/tool/SelectMask.h>
#include <vertical3d/src/tool/SelectTool.h>
#include <vertical3d/src/tool/TransformTool.h>
#include <vertical3d/src/view/ViewLayout.h>
#include <vertical3d/src/view/ViewPort.h>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <string>
#include <vector>
#include <fstream>

#include "Renderer.h"

#include <boost/make_shared.hpp>

namespace v3d::editor {

namespace {

/**
 * What the ui config calls the container the editor's own components are in, and the
 * menu bar within it. Every other component of that container is a toolbar.
 **/
const char* const uiContainer = "editor";
const char* const menuBar = "menu-bar";

};  // namespace

/**
 **/
Controller::Controller(const std::string& path) :
    v3d::engine::Engine(path),
    path_(path),
    projectPath_(boost::filesystem::path(path) / "project.json"),
    cursor_(0.0F, 0.0F),
    uiGrab_(false) {
    registerCommands();
}

/**
 **/
bool Controller::start() {
    if (!config()) {
        logger()->get()->error("The editor needs its config to know what to draw");
        return false;
    }

    scene_ = boost::make_shared<Scene>();
    project_ = boost::make_shared<Project>(logger());
    history_ = boost::make_shared<CommandStack>();

    profiles_ = boost::make_shared<v3d::config::CameraProfiles>(logger());
    const boost::json::object* cameras = document(v3d::config::Type::Camera);
    if (cameras == nullptr) {
        logger()->get()->error("The editor has no camera config, so its views would have no cameras");
        return false;
    }
    if (!profiles_->load(*cameras)) {
        return false;
    }

    layout_ = boost::make_shared<ViewLayout>(logger());
    if (!layout_->load(config()->get(v3d::config::Type::Layout))) {
        return false;
    }

    if (!buildViews()) {
        return false;
    }

    cameraTool_ = boost::make_shared<CameraControlTool>();
    selectTool_ = boost::make_shared<SelectTool>(scene_, logger());
    transformTool_ = boost::make_shared<TransformTool>(scene_, logger());
    transformTool_->commands(history_);

    motion_ = dispatcher()->sink<v3d::event::kind::MouseMotion>().connect<&Controller::handleMotion>(*this);
    resize_ = dispatcher()->sink<v3d::event::kind::WindowResize>().connect<&Controller::handleResize>(*this);

    renderer_ = boost::make_shared<Renderer>(window(), logger(), assets());
    renderer_->views(views_);
    renderer_->scene(scene_);
    renderer_->transformTool(transformTool_);
    renderer_->selectTool(selectTool_);

    if (!buildUi()) {
        return false;
    }
    renderer_->ui(vgui_);

    for (const std::string& name : refused_) {
        logger()->get()->error("{} is registered twice", name);
    }
    std::size_t named = 0;
    for (const std::string& name : commands().names()) {
        if (menu_ && menu_->find(name)) {
            named++;
        }
    }
    logger()->get()->info("{} commands registered, {} of them on a menu", commands().size(), named);

    layoutViews(window()->width(), window()->height());
    syncUi();
    syncCaption();

    logger()->get()->info("{} with {} views", layout_->name(), views_.size());
    return true;
}

/**
 **/
bool Controller::buildViews() {
    views_.clear();
    for (const ViewLayout::View& view : layout_->views()) {
        if (!profiles_->has(view.camera)) {
            logger()->get()->error("The layout names a camera profile that does not exist: {}", view.camera);
            return false;
        }
        views_.push_back(boost::make_shared<ViewPort>(view.name, profiles_->get(view.camera)));
    }
    if (!views_.empty()) {
        activeView_ = views_.front();
    }
    return true;
}

/**
 **/
bool Controller::buildUi() {
    const boost::json::object* config = document(v3d::config::Type::Ui);
    if (config == nullptr) {
        logger()->get()->error("The editor has no ui config, so it would have no menus");
        return false;
    }

    vgui_ = boost::make_shared<v3d::ui::Engine>(events(), dispatcher(), logger());
    if (!vgui_->load(*config)) {
        return false;
    }
    // the ui hit-tests the cursor before the app, in the order its own strips are drawn.
    // The text measure is the renderer's, so a press inside a text box places the caret
    // on the character drawn under the pointer
    uiCursor_ = boost::make_shared<v3d::ui::input::Cursor>(vgui_, dispatcher(), renderer_->measure());
    uiKeys_ = boost::make_shared<v3d::ui::shell::Keyboard>(vgui_, dispatcher(), window());
    chooser_ = boost::make_shared<v3d::ui::shell::FileChooser>(vgui_);
    prompt_ = boost::make_shared<v3d::ui::shell::Prompt>(vgui_);

    const boost::shared_ptr<v3d::ui::Container> container = vgui_->container(uiContainer);
    if (!container) {
        logger()->get()->error("The ui config has no {} container", uiContainer);
        return false;
    }
    menu_ = boost::dynamic_pointer_cast<v3d::ui::component::menu::Bar>(container->get(menuBar));
    if (!menu_) {
        logger()->get()->error("The {} container has no {} in it", uiContainer, menuBar);
        return false;
    }

    toolbars_.clear();
    for (const boost::shared_ptr<v3d::ui::Component>& component : container->components()) {
        const boost::shared_ptr<v3d::ui::component::Toolbar> bar =
            boost::dynamic_pointer_cast<v3d::ui::component::Toolbar>(component);
        if (bar) {
            toolbars_.push_back(bar);
        }
    }
    return true;
}

/**
 **/
void Controller::syncUi() {
    if (!menu_) {
        return;
    }
    auto mark = [this](const char* command, bool on) {
        const boost::shared_ptr<v3d::ui::component::menu::Item> item = menu_->find(command);
        if (item) {
            item->checked(on);
        }
        // a command may be on a menu, on a toolbar or on both, and both show the editor's
        // flag rather than keeping one each
        for (const boost::shared_ptr<v3d::ui::component::Toolbar>& bar : toolbars_) {
            const boost::shared_ptr<v3d::ui::component::Button> button = bar->find(command);
            if (button) {
                button->checked(on);
            }
        }
    };

    if (activeView_) {
        mark("view::show::mesh", activeView_->shows(ViewPort::SHOW_MESH));
        mark("view::show::handle", activeView_->shows(ViewPort::SHOW_HANDLE));
        mark("view::show::grid", activeView_->shows(ViewPort::SHOW_GRID));
    }

    const SelectMask mask = selectTool_->mask();
    mark("select::mask::object", mask == SelectMask::Object);
    mark("select::mask::vertex", mask == SelectMask::Vertex);
    mark("select::mask::edge", mask == SelectMask::Edge);
    mark("select::mask::face", mask == SelectMask::Face);

    const TransformTool::Mode mode = transformTool_->mode();
    mark("transform::select", mode == TransformTool::Mode::None);
    mark("transform::translate", mode == TransformTool::Mode::Translate);
    mark("transform::rotate", mode == TransformTool::Mode::Rotate);
    mark("transform::scale", mode == TransformTool::Mode::Scale);
}

/**
 **/
void Controller::syncCaption() {
    if (!history_) {
        return;
    }
    const std::string caption = "Vertical|3D - " + projectPath_.filename().string() + (history_->dirty() ? " *" : "");
    if (caption == caption_) {
        return;
    }
    caption_ = caption;
    window()->caption(caption_);
}

/**
 **/
void Controller::registerCommands() {
    // a check item shows what the editor holds rather than remembering its own state, so
    // whatever a command changed is read back after it runs
    auto press = [this](const std::string& name, const v3d::event::Commands::PressHandler& handler) {
        const bool added = commands().addPress(name, [this, handler]() {
            handler();
            syncUi();
            syncCaption();
        });
        if (!added) {
            refused_.push_back(name);
        }
    };
    auto hold = [this](const std::string& name, const v3d::event::Commands::Handler& handler) {
        const bool added = commands().add(name, [this, handler](const v3d::event::Event& event) {
            handler(event);
            syncUi();
            syncCaption();
        });
        if (!added) {
            refused_.push_back(name);
        }
    };

    // a key binding in mappings.json and a menu item in the ui config invoke a command
    // by these names, so all three have to spell them the same way
    press("create::poly::cube", [this]() { createPoly("cube"); });
    press("create::poly::plane", [this]() { createPoly("plane"); });
    press("create::poly::cylinder", [this]() { createPoly("cylinder"); });
    press("create::poly::cone", [this]() { createPoly("cone"); });
    press("transform::poly::split_edge", [this]() { splitSelectedEdges(); });
    press("transform::poly::split_face", [this]() { splitSelectedFace(); });
    press("transform::poly::extrude_face", [this]() { changeSelectedFaces("extrude face", extrudeFaces); });
    press("transform::poly::inset_face", [this]() { changeSelectedFaces("inset face", insetFaces); });
    press("transform::poly::merge_vertices", [this]() { mergeSelectedVertices(); });

    press("select::mask::object", [this]() { selectMask("object"); });
    press("select::mask::vertex", [this]() { selectMask("vertex"); });
    press("select::mask::edge", [this]() { selectMask("edge"); });
    press("select::mask::face", [this]() { selectMask("face"); });

    press("transform::select", [this]() { transformMode("select"); });
    press("transform::translate", [this]() { transformMode("translate"); });
    press("transform::rotate", [this]() { transformMode("rotate"); });
    press("transform::scale", [this]() { transformMode("scale"); });

    // camera and light have flags on the view but nothing draws them, so they have no
    // command: a toggle for either would appear to work and change nothing
    press("view::show::grid", [this]() { toggleShow(ViewPort::SHOW_GRID); });
    press("view::show::mesh", [this]() { toggleShow(ViewPort::SHOW_MESH); });
    press("view::show::handle", [this]() { toggleShow(ViewPort::SHOW_HANDLE); });
    // not a view flag: the readout covers the window rather than one pane, so the renderer
    // shows it and nothing here reads it back
    press("view::show::statistics", [this]() { renderer_->statistics()->toggle(); });

    // zoom and truck are held rather than latched: the modifier going down chooses what a
    // drag of the primary button performs and it coming up puts the tool back to none
    hold("view::camera::zoom", [this](const v3d::event::Event& event) {
        cameraMode("zoom", event.state() != v3d::event::State::Released);
    });
    hold("view::camera::truck", [this](const v3d::event::Event& event) {
        cameraMode("truck", event.state() != v3d::event::State::Released);
    });
    // pan is a button of its own, so holding it is the drag
    hold("view::camera::pan", [this](const v3d::event::Event& event) {
        const bool pressed = event.state() != v3d::event::State::Released;
        cameraMode("pan", pressed);
        cameraTool_->button(1, pressed, cursor_);
    });
    // held: while it is down a click adds to the selection or takes away from it
    hold("select::add", [this](const v3d::event::Event& event) {
        selectTool_->adding(event.state() != v3d::event::State::Released);
    });

    hold("view::drag", [this](const v3d::event::Event& event) {
        drag(event.state() == v3d::event::State::Pressed);
    });

    press("project::load", [this]() { openProject(); });
    press("project::save", [this]() { saveProject(); });
    press("project::saveAs", [this]() { saveProjectAs(); });
    press("project::chooser::pick", [this]() { chooser_->pick(); });
    press("project::chooser::accept", [this]() { chooser_->accept(); });
    press("project::chooser::cancel", [this]() { chooser_->close(); });
    press("project::export::rib", [this]() { exportProject(); });
    // the prompt's buttons, in the order its answers are given
    press("project::prompt::answer0", [this]() { prompt_->answer(0); });
    press("project::prompt::answer1", [this]() { prompt_->answer(1); });
    press("project::prompt::answer2", [this]() { prompt_->answer(2); });

    press("edit::undo", [this]() { history("undo"); });
    press("edit::redo", [this]() { history("redo"); });
}

/**
 **/
void Controller::createPoly(const std::string& name) {
    boost::shared_ptr<v3d::brep::BRep> mesh;
    if (name == "cube") {
        mesh = create_poly_cube();
    } else if (name == "plane") {
        mesh = create_poly_plane();
    } else if (name == "cylinder") {
        mesh = create_poly_cylinder();
    } else if (name == "cone") {
        mesh = create_poly_cone();
    } else {
        return;
    }

    // the command does the creating, so making a mesh and redoing one run the same code
    const boost::shared_ptr<CreateCommand> command = boost::make_shared<CreateCommand>(scene_, mesh, name);
    command->redo();
    history_->push(command);
    logger()->get()->info("created a {} - {} meshes", name, scene_->count());
}

/**
 **/
void Controller::splitSelectedEdges() {
    if (selectTool_->mask() != SelectMask::Edge) {
        logger()->get()->info("split edge needs the edge select mask");
        return;
    }
    const boost::shared_ptr<TopologyCommand> command = splitEdges(*scene_);
    if (!command) {
        logger()->get()->info("split edge: no edge is selected");
        return;
    }
    history_->push(command);
    logger()->get()->info("split the selected edges");
}

/**
 **/
void Controller::splitSelectedFace() {
    if (selectTool_->mask() != SelectMask::Vertex) {
        logger()->get()->info("split face needs the vertex select mask");
        return;
    }
    std::string problem;
    const boost::shared_ptr<TopologyCommand> command = splitFace(*scene_, &problem);
    if (!command) {
        logger()->get()->info("split face: {}", problem);
        return;
    }
    history_->push(command);
    logger()->get()->info("split a face between the selected vertices");
}

/**
 **/
void Controller::mergeSelectedVertices() {
    if (selectTool_->mask() != SelectMask::Vertex) {
        logger()->get()->info("merge vertices needs the vertex select mask");
        return;
    }
    std::string problem;
    const boost::shared_ptr<TopologyCommand> command = mergeVertices(*scene_, &problem);
    if (!command) {
        logger()->get()->info("merge vertices: {}", problem);
        return;
    }
    history_->push(command);
    logger()->get()->info("merged the selected vertices");
}

/**
 **/
void Controller::changeSelectedFaces(const std::string& name, FaceOperation operation) {
    if (selectTool_->mask() != SelectMask::Face) {
        logger()->get()->info("{} needs the face select mask", name);
        return;
    }
    const boost::shared_ptr<TopologyCommand> command = operation(*scene_);
    if (!command) {
        logger()->get()->info("{}: no face is selected", name);
        return;
    }
    history_->push(command);
    logger()->get()->info("{}: done to the selected faces", name);
}

/**
 **/
void Controller::history(const std::string& name) {
    boost::shared_ptr<Command> command;
    if (name == "undo") {
        command = history_->undo();
    } else if (name == "redo") {
        command = history_->redo();
    } else {
        return;
    }
    if (!command) {
        logger()->get()->info("nothing to {}", name);
        return;
    }
    logger()->get()->info("{} {}", name, command->name());
}

/**
 **/
std::string Controller::exportPath() const {
    return path_ + "export.rib";
}

/**
 **/
void Controller::exportProject() {
    if (!activeView_ || !activeView_->camera()) {
        logger()->get()->error("nothing to export a scene from - there is no active view");
        return;
    }

    std::ofstream file(exportPath().c_str());
    if (!file.is_open()) {
        logger()->get()->error("could not write {}", exportPath());
        return;
    }

    RIBExportVisitor visitor(&file);
    const glm::uvec2 size = activeView_->camera()->profile().size();
    visitor.begin(*activeView_->camera(), size.x, size.y);
    scene_->accept(&visitor);
    visitor.end();

    logger()->get()->info("exported {} mesh(es) to {}", scene_->count(), exportPath());
}

/**
 **/
void Controller::openProject() {
    askBeforeDiscarding([this]() {
        chooser_->open(v3d::ui::shell::FileChooser::Mode::Open, projectPath_.parent_path(), ".json",
            [this](const boost::filesystem::path& chosen) {
                // a gesture under way is holding the mesh it started on, which the read is
                // about to take out of the scene
                transformTool_->cancel();
                if (!project_->read(chosen.string(), scene_)) {
                    return;
                }
                projectPath_ = chosen;
                // the history describes the scene that was replaced, so none of it can be
                // undone against the new one. Clearing it also marks the new scene as saved
                history_->clear();
            });
    });
}

/**
 **/
bool Controller::saveProject() {
    if (!project_->write(projectPath_.string(), scene_)) {
        return false;
    }
    history_->markSaved();
    return true;
}

/**
 **/
void Controller::askBeforeDiscarding(const std::function<void()>& proceed) {
    if (!history_->dirty() || !prompt_) {
        proceed();
        return;
    }
    prompt_->open("Save the changes to " + projectPath_.filename().string() + "?", {
        { "Save", [this, proceed]() {
            // a project that could not be written still has changes to lose
            if (saveProject()) {
                proceed();
            }
        } },
        { "Discard", proceed },
        { "Cancel", {} }
    }, {});
}

/**
 **/
bool Controller::quitRequested() {
    if (!history_ || !history_->dirty()) {
        return true;
    }
    // a second close while the question is up would replace it, and lose what the first one
    // was asking for
    if (prompt_ && prompt_->visible()) {
        return false;
    }
    askBeforeDiscarding([this]() { quit(); });
    return false;
}

/**
 **/
void Controller::saveProjectAs() {
    chooser_->open(v3d::ui::shell::FileChooser::Mode::Save, projectPath_.parent_path(), ".json",
        [this](const boost::filesystem::path& chosen) {
            if (project_->write(chosen.string(), scene_)) {
                projectPath_ = chosen;
                history_->markSaved();
            }
        });
}

/**
 **/
void Controller::toggleShow(ViewPort::VisibleFilter filter) {
    if (activeView_) {
        activeView_->show(filter, !activeView_->shows(filter));
    }
}

/**
 **/
void Controller::transformMode(const std::string& name) {
    transformTool_->activate(name);
}

/**
 **/
void Controller::selectMask(const std::string& name) {
    selectTool_->activate(name);
    transformTool_->mask(selectTool_->mask());
}

/**
 **/
void Controller::cameraMode(const std::string& name, bool pressed) {
    if (pressed) {
        cameraTool_->activate(name);
    } else {
        cameraTool_->deactivate(name);
    }
}

/**
 **/
bool Controller::onEvent(const SDL_Event& event) {
    return uiKeys_ && uiKeys_->event(event);
}

/**
 **/
bool Controller::uiMotion(const glm::vec2& cursor) {
    return uiCursor_ && uiCursor_->motion(cursor);
}

/**
 **/
bool Controller::uiPress(const glm::vec2& cursor) {
    return uiCursor_ && uiCursor_->press(cursor);
}

/**
 **/
void Controller::drag(bool pressed) {
    // the ui is drawn over every view, so it is offered the press first. The release that
    // ends a press the ui took goes nowhere else
    if (pressed) {
        uiGrab_ = uiPress(cursor_);
        if (uiGrab_) {
            return;
        }
    } else if (uiGrab_) {
        uiGrab_ = false;
        if (uiCursor_) {
            uiCursor_->release(cursor_);
        }
        return;
    }

    // the primary mouse button. The mapped event carries no button number, because the
    // binding names which button it is
    cameraTool_->button(1, pressed, cursor_);
    // one button drives three tools. With a modifier held, the drag drives a camera.
    // Otherwise a handle of the selection under the cursor takes the press, and a press no
    // handle took picks
    if (cameraTool_->mode() != CameraControlTool::CAMERA_MODE_NONE) {
        // a modifier pressed during a handle drag or a rubber band does not keep the
        // release from the tool that ends it
        if (!pressed) {
            transformTool_->button(1, false, cursor_);
            selectTool_->button(1, false, cursor_);
        }
        return;
    }
    transformTool_->button(1, pressed, cursor_);
    if (!transformTool_->dragging()) {
        selectTool_->button(1, pressed, cursor_);
    }
}

/**
 **/
void Controller::layoutViews(int width, int height) {
    // the menu bar and the toolbars cover strips along two edges, and the views divide
    // what is left
    const glm::vec2 inset = renderer_ ? renderer_->insets() : glm::vec2(0.0F, 0.0F);
    layout_->resize(glm::vec4(inset.x, inset.y,
        std::max(0.0F, static_cast<float>(width) - inset.x),
        std::max(0.0F, static_cast<float>(height) - inset.y)));
    const std::vector<ViewLayout::View>& regions = layout_->views();
    for (std::size_t index = 0; index < views_.size() && index < regions.size(); index++) {
        views_[index]->resize(regions[index].region);
    }
    if (cameraTool_ && cameraTool_->view()) {
        // the arcball is sized by the view, so it has to hear about the new region too
        cameraTool_->view(cameraTool_->view());
    }
}

/**
 **/
bool Controller::render() {
    const v3d::engine::Statistics& measured = statistics();
    renderer_->draw({ measured.mean(), measured.last(), measured.steps() });
    return true;
}

/**
 **/
bool Controller::release() {
    if (renderer_) {
        // the device has to be idle before the window it presents to is destroyed
        renderer_->shutdown();
    }
    return true;
}

/**
 **/
void Controller::handleResize(const v3d::event::kind::WindowResize& event) {
    layoutViews(event.width(), event.height());
}

/**
 **/
void Controller::handleMotion(const v3d::event::kind::MouseMotion& event) {
    cursor_ = event.position();

    const bool dragging = (cameraTool_ && cameraTool_->dragging()) || (transformTool_ && transformTool_->dragging()) ||
        (selectTool_ && selectTool_->dragging());
    // while a drag is under way the ui is not offered the cursor, so a drag that passes
    // over the menu bar or a toolbar is not interrupted
    if (!dragging && uiMotion(cursor_)) {
        return;
    }

    // the view under the cursor becomes the one a drag drives. It does not change during a
    // drag, so a drag that crosses a border between views keeps its camera
    if (!dragging) {
        const std::size_t index = layout_->viewAt(cursor_.x, cursor_.y);
        if (index < views_.size()) {
            const bool changed = views_[index] != activeView_;
            activeView_ = views_[index];
            if (changed) {
                // the show flags a check item draws are the active view's
                syncUi();
            }
            cameraTool_->view(activeView_);
            if (selectTool_) {
                selectTool_->view(activeView_);
            }
            transformTool_->view(activeView_);
        }
    }

    if (cameraTool_) {
        cameraTool_->motion(cursor_);
    }
    if (transformTool_) {
        transformTool_->motion(cursor_);
    }
    if (selectTool_) {
        selectTool_->motion(cursor_);
    }
}

};  // namespace v3d::editor
