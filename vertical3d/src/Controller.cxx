/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
**/

#include "Controller.h"

#include <api/brep/BRep.h>
#include <api/engine/CameraProfiles.h>
#include <api/engine/Engine.h>
#include <api/event/Event.h>
#include <api/event/State.h>
#include <api/event/kind/MouseMotion.h>
#include <api/event/kind/WindowResize.h>
#include <api/image/Image.h>
#include <api/shell/Shell.h>
#include <api/type/camera/Camera.h>
#include <api/ui/Component.h>
#include <api/ui/Container.h>
#include <api/ui/Interface.h>
#include <api/ui/Image.h>
#include <api/ui/component/Button.h>
#include <api/ui/component/Toolbar.h>
#include <api/ui/component/menu/Item.h>
#include <api/ui/input/Cursor.h>
#include <api/ui/shell/FileChooser.h>
#include <api/ui/shell/Keyboard.h>
#include <api/ui/shell/Prompt.h>
#include <api/window/Window.h>
#include <vertical3d/src/command/Command.h>
#include <vertical3d/src/command/CommandStack.h>
#include <vertical3d/src/command/CreateCommand.h>
#include <vertical3d/src/command/GroupCommand.h>
#include <vertical3d/src/command/KeyCommand.h>
#include <vertical3d/src/command/SettingsCommand.h>
#include <vertical3d/src/command/TopologyCommand.h>
#include <vertical3d/src/render/Batch.h>
#include <vertical3d/src/render/Delegate.h>
#include <vertical3d/src/render/MoyaDelegate.h>
#include <vertical3d/src/render/RenderPanels.h>
#include <vertical3d/src/render/Shot.h>
#include <vertical3d/src/scene/Camera.h>
#include <vertical3d/src/scene/CreateItem.h>
#include <vertical3d/src/scene/CreatePoly.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Light.h>
#include <vertical3d/src/scene/Mesh.h>
#include <vertical3d/src/scene/Modelling.h>
#include <vertical3d/src/scene/Project.h>
#include <vertical3d/src/scene/RIBExportVisitor.h>
#include <vertical3d/src/scene/RenderSettings.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/scene/Timeline.h>
#include <vertical3d/src/tool/CameraControlTool.h>
#include <vertical3d/src/tool/SelectMask.h>
#include <vertical3d/src/tool/SelectTool.h>
#include <vertical3d/src/tool/TransformTool.h>
#include <vertical3d/src/view/Triangulation.h>
#include <vertical3d/src/view/ViewLayout.h>
#include <vertical3d/src/view/ViewPort.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <optional>
#include <utility>
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

#ifdef _WIN32
const char* const MOYA = "moya.exe";
#else
const char* const MOYA = "moya";
#endif

/**
 * A keyed frame as a whole frame within the timeline's range, rounded up or down.
 **/
int wholeFrame(float frame, const Timeline& timeline, bool up) {
    const float held = std::clamp(frame, static_cast<float>(timeline.start), static_cast<float>(timeline.end));
    return static_cast<int>(up ? std::ceil(held) : std::floor(held));  // checked: a key's frame is finite, and the clamp holds it in the range
}

/**
 * The editor's shell. It has no game menu, because the editor's menu is its menu bar.
 **/
v3d::shell::Shell::Options shellOptions() {
    v3d::shell::Shell::Options options;
    options.menu = false;
    options.statistics = "view::show::statistics";
    return options;
}

};  // namespace

/**
 **/
Controller::Controller(const std::string& path) :
    v3d::engine::Engine(path),
    path_(path),
    shell_(this, shellOptions()),
    cursor_(0.0F, 0.0F),
    uiGrab_(false) {
    registerCommands();
}

/**
 **/
bool Controller::start() {
    if (!manifest()) {
        logger()->get()->error("The editor needs its config to know what to draw");
        return false;
    }

    scene_ = boost::make_shared<Scene>();
    project_ = boost::make_shared<Project>(logger(), boost::filesystem::path(path_) / "project");
    history_ = boost::make_shared<CommandStack>();

    profiles_ = boost::make_shared<v3d::engine::CameraProfiles>(logger());
    const boost::json::object* cameras = document(v3d::engine::CameraProfiles::DOCUMENT);
    if (cameras == nullptr) {
        logger()->get()->error("The editor has no camera config, so its views would have no cameras");
        return false;
    }
    if (!profiles_->load(*cameras)) {
        return false;
    }

    layout_ = boost::make_shared<ViewLayout>(logger());
    if (!layout_->load(manifest()->get(ViewLayout::DOCUMENT))) {
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

    renderer_ = boost::make_shared<Renderer>(window(), logger(), assets(), &shell_);
    renderer_->views(views_);
    renderer_->scene(scene_);
    renderer_->transformTool(transformTool_);
    renderer_->selectTool(selectTool_);

    if (!buildUi()) {
        return false;
    }
    renderer_->ui(shell_.ui());

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

    // the editor starts on the default project without reading it, so an autosave a crash left
    // there is the one way back to that session's work
    offerRecovery();
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
    const boost::json::object* config = document(v3d::ui::Interface::DOCUMENT);
    if (config == nullptr) {
        logger()->get()->error("The editor has no ui config, so it would have no menus");
        return false;
    }

    if (!shell_.start()) {
        return false;
    }
    const boost::shared_ptr<v3d::ui::Interface>& vgui = shell_.ui();
    // the ui hit-tests the cursor before the app, in the order its own strips are drawn.
    // The text measure is the renderer's, so a press inside a text box places the caret
    // on the character drawn under the pointer
    uiCursor_ = boost::make_shared<v3d::ui::input::Cursor>(vgui, dispatcher(), renderer_->measure());
    uiKeys_ = boost::make_shared<v3d::ui::shell::Keyboard>(vgui, dispatcher(), window());
    chooser_ = boost::make_shared<v3d::ui::shell::FileChooser>(vgui);
    prompt_ = boost::make_shared<v3d::ui::shell::Prompt>(vgui);
    panels_ = boost::make_shared<RenderPanels>(vgui);
    delegate_ = boost::make_shared<MoyaDelegate>(logger());
    // moya.exe is built beside the editor
    batch_ = boost::make_shared<Batch>(logger(), boost::filesystem::path(path_) / MOYA);

    const boost::shared_ptr<v3d::ui::Container> container = vgui->container(uiContainer);
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
    auto mark = [this](const std::string& command, bool on) {
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
        mark("view::show::camera", activeView_->shows(ViewPort::SHOW_CAMERA));
        mark("view::show::light", activeView_->shows(ViewPort::SHOW_LIGHT));
        mark("view::show::handle", activeView_->shows(ViewPort::SHOW_HANDLE));
        mark("view::show::grid", activeView_->shows(ViewPort::SHOW_GRID));
        const ViewPort::Display& shown = activeView_->display();
        mark("view::shade::flat", shown.shading == Shading::Flat);
        mark("view::shade::smooth", shown.shading == Shading::Smooth);
        mark("view::cull::none", shown.cull == ViewPort::Cull::None);
        mark("view::cull::front", shown.cull == ViewPort::Cull::Front);
        mark("view::cull::back", shown.cull == ViewPort::Cull::Back);
        mark("view::cull::both", shown.cull == ViewPort::Cull::Both);
        for (const auto& [name, fill] : std::vector<std::pair<const char*, ViewPort::Fill>>{ { "points", ViewPort::Fill::Points },
            { "lines", ViewPort::Fill::Lines }, { "fill", ViewPort::Fill::Fill } }) {
            mark(std::string("view::rendermode::front::") + name, shown.front == fill);
            mark(std::string("view::rendermode::back::") + name, shown.back == fill);
        }

        // a view laid out from a profile the menu does not offer, such as a second
        // perspective, marks none of the fixed views
        const v3d::type::camera::Camera& camera = *activeView_->camera();
        for (const FixedView& view : fixedViews()) {
            mark(view.command, camera.profile().name() == view.profile);
        }
        mark("view::camera::orthographic", camera.orthographic());
    }

    const SelectMask mask = selectTool_->mask();
    mark("select::mask::object", mask == SelectMask::Object);
    mark("select::mask::vertex", mask == SelectMask::Vertex);
    mark("select::mask::edge", mask == SelectMask::Edge);
    mark("select::mask::face", mask == SelectMask::Face);
    const CameraControlTool::CameraMode latched = cameraTool_->latched();
    mark("camera::truck", latched == CameraControlTool::CAMERA_MODE_TRUCK);
    mark("camera::dolly", latched == CameraControlTool::CAMERA_MODE_DOLLY);
    mark("camera::zoom", latched == CameraControlTool::CAMERA_MODE_ZOOM);
    mark("camera::pan", latched == CameraControlTool::CAMERA_MODE_PAN);
    mark("camera::pitch", latched == CameraControlTool::CAMERA_MODE_PITCH);
    mark("select::mask::mesh", mask == SelectMask::Mesh);
    mark("select::mask::light", mask == SelectMask::Light);
    mark("select::mask::camera", mask == SelectMask::Camera);

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
    std::string caption = "Vertical|3D - " + project_->directory().filename().string() + (history_->dirty() ? " *" : "") +
        " - frame " + std::to_string(scene_->timeline().frame);
    const Batch::Status batch = batch_ ? batch_->status() : Batch::Status();
    if (batch.running) {
        caption += " - rendering " + std::to_string(batch.done + 1) + " of " + std::to_string(batch.total);
    }
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
    press("create::perspective::camera", [this]() { createCamera(Camera::Projection::Perspective); });
    press("create::ortho::camera", [this]() { createCamera(Camera::Projection::Orthographic); });
    press("create::light::point", [this]() { createLight(Light::Type::Point); });
    press("create::light::distant", [this]() { createLight(Light::Type::Distant); });
    press("create::light::spot", [this]() { createLight(Light::Type::Spot); });
    press("create::group", [this]() { createGroup(); });
    press("timeline::set", [this]() { setKey(); });
    press("render::view", [this]() { renderView(); });
    press("render::stop", [this]() {
        delegate_->cancel();
        batch_->cancel();
    });
    press("render::close", [this]() {
        delegate_->cancel();
        panels_->hideView();
    });
    press("render::settings", [this]() { panels_->openSettings(scene_->renderSettings()); });
    press("render::settings::apply", [this]() { applySettings(); });
    press("render::settings::cancel", [this]() { panels_->closeSettings(); });
    press("render::batch", [this]() { renderBatch(); });
    press("timeline::playback::play", [this]() {
        playing_ = true;
        played_ = 0.0F;
    });
    press("timeline::playback::stop", [this]() { playing_ = false; });
    press("timeline::playback::next", [this]() { goTo(scene_->timeline().frame + 1); });
    press("timeline::playback::previous", [this]() { goTo(scene_->timeline().frame - 1); });
    press("timeline::playback::first", [this]() { goTo(scene_->timeline().start); });
    press("timeline::playback::last", [this]() { goTo(scene_->timeline().end); });
    press("timeline::playback::next_key", [this]() { nextKey(true); });
    press("timeline::playback::previous_key", [this]() { nextKey(false); });
    press("timeline::playback::first_key", [this]() {
        const std::vector<float> frames = keyedFrames();
        if (!frames.empty()) {
            goTo(wholeFrame(frames.front(), scene_->timeline(), true));
        }
    });
    press("timeline::playback::last_key", [this]() {
        const std::vector<float> frames = keyedFrames();
        if (!frames.empty()) {
            goTo(wholeFrame(frames.back(), scene_->timeline(), false));
        }
    });
    press("transform::poly::split_edge", [this]() { splitSelectedEdges(); });
    press("transform::poly::split_face", [this]() { splitSelectedFace(); });
    press("transform::poly::extrude_face", [this]() { changeSelectedFaces("extrude face", extrudeFaces); });
    press("transform::poly::inset_face", [this]() { changeSelectedFaces("inset face", insetFaces); });
    press("transform::poly::merge_vertices", [this]() { mergeSelectedVertices(); });

    press("select::mask::object", [this]() { selectMask("object"); });
    press("select::mask::vertex", [this]() { selectMask("vertex"); });
    press("select::mask::edge", [this]() { selectMask("edge"); });
    press("select::mask::face", [this]() { selectMask("face"); });
    press("select::mask::mesh", [this]() { selectMask("mesh"); });
    press("select::mask::light", [this]() { selectMask("light"); });
    press("select::mask::camera", [this]() { selectMask("camera"); });

    // the Camera Manipulators menu latches a move, so a drag with no modifier drives the
    // camera until the move is chosen again or a transform mode is
    for (const char* move : { "truck", "dolly", "zoom", "pan", "pitch" }) {
        press(std::string("camera::") + move, [this, move]() { cameraTool_->latch(move); });
    }
    press("transform::select", [this]() { transformMode("select"); });
    press("transform::translate", [this]() { transformMode("translate"); });
    press("transform::rotate", [this]() { transformMode("rotate"); });
    press("transform::scale", [this]() { transformMode("scale"); });

    // how the active view shades its meshes. Each view keeps its own
    const auto display = [this](const std::function<void(ViewPort::Display*)>& change) {
        if (activeView_) {
            change(&activeView_->display());
        }
    };
    press("view::shade::flat", [display]() { display([](ViewPort::Display* shown) { shown->shading = Shading::Flat; }); });
    press("view::shade::smooth", [display]() { display([](ViewPort::Display* shown) { shown->shading = Shading::Smooth; }); });
    for (const auto& [name, cull] : std::vector<std::pair<const char*, ViewPort::Cull>>{ { "none", ViewPort::Cull::None },
        { "front", ViewPort::Cull::Front }, { "back", ViewPort::Cull::Back }, { "both", ViewPort::Cull::Both } }) {
        press(std::string("view::cull::") + name, [display, cull]() { display([cull](ViewPort::Display* shown) { shown->cull = cull; }); });
    }
    for (const auto& [name, fill] : std::vector<std::pair<const char*, ViewPort::Fill>>{ { "points", ViewPort::Fill::Points },
        { "lines", ViewPort::Fill::Lines }, { "fill", ViewPort::Fill::Fill } }) {
        press(std::string("view::rendermode::front::") + name,
            [display, fill]() { display([fill](ViewPort::Display* shown) { shown->front = fill; }); });
        press(std::string("view::rendermode::back::") + name,
            [display, fill]() { display([fill](ViewPort::Display* shown) { shown->back = fill; }); });
    }
    press("view::show::grid", [this]() { toggleShow(ViewPort::SHOW_GRID); });
    press("view::show::mesh", [this]() { toggleShow(ViewPort::SHOW_MESH); });
    press("view::show::camera", [this]() { toggleShow(ViewPort::SHOW_CAMERA); });
    press("view::show::light", [this]() { toggleShow(ViewPort::SHOW_LIGHT); });
    press("view::show::handle", [this]() { toggleShow(ViewPort::SHOW_HANDLE); });
    for (const FixedView& view : fixedViews()) {
        const std::string profile = view.profile;
        press(view.command, [this, profile]() { look(profile); });
    }
    press("view::camera::orthographic", [this]() { toggleProjection(); });
    // not a view flag: the readout covers the window rather than one pane, so the renderer
    // shows it and nothing here reads it back

    // these are held: the modifier going down chooses what a drag of the primary button
    // performs, and it coming up puts the tool back to whatever is latched
    hold("view::camera::zoom", [this](const v3d::event::Event& event) {
        cameraMode("zoom", event.state() != v3d::event::State::Released);
    });
    hold("view::camera::dolly", [this](const v3d::event::Event& event) {
        cameraMode("dolly", event.state() != v3d::event::State::Released);
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

    create(boost::make_shared<Mesh>(mesh), name);
}

/**
 **/
void Controller::createCamera(Camera::Projection projection) {
    if (!activeView_ || !activeView_->camera()) {
        logger()->get()->info("a camera is made at a view, and there is no active view");
        return;
    }
    const char* name = projection == Camera::Projection::Orthographic ? "orthographic camera" : "perspective camera";
    create(create_camera(activeView_->camera()->profile(), projection), name);
}

/**
 **/
void Controller::createLight(Light::Type type) {
    if (!activeView_ || !activeView_->camera()) {
        logger()->get()->info("a light is made at a view, and there is no active view");
        return;
    }
    const char* name = "point light";
    if (type == Light::Type::Distant) {
        name = "distant light";
    } else if (type == Light::Type::Spot) {
        name = "spot light";
    }
    create(create_light(activeView_->camera()->profile(), type), name);
}

/**
 **/
void Controller::createGroup() {
    const boost::shared_ptr<GroupCommand> command = boost::make_shared<GroupCommand>(scene_, scene_->selection());
    if (command->empty()) {
        logger()->get()->info("a group gathers the selection, and nothing is selected");
        return;
    }
    command->redo();
    history_->push(command);
    logger()->get()->info("grouped the selection - {} items", scene_->count());
}

/**
 **/
void Controller::setKey() {
    const int frame = scene_->timeline().frame;
    const boost::shared_ptr<KeyCommand> command = boost::make_shared<KeyCommand>(scene_, scene_->selection(),
        static_cast<float>(frame));
    if (command->empty()) {
        logger()->get()->info("a key is set on the selection, and nothing is selected");
        return;
    }
    command->redo();
    history_->push(command);
    logger()->get()->info("set a key at frame {}", frame);
}

/**
 **/
v3d::type::camera::Profile Controller::shot() const {
    return shotFrom(*scene_, activeView_->camera()->profile());
}

/**
 **/
void Controller::renderView() {
    if (!activeView_ || !activeView_->camera()) {
        logger()->get()->info("a render is seen through the active view, and there is none");
        return;
    }
    panels_->showView();
    delegate_->render(*scene_, shot(), scene_->renderSettings());
    syncRender();
}

/**
 **/
void Controller::applySettings() {
    const std::optional<RenderSettings> settings = panels_->settings();
    if (!settings) {
        logger()->get()->info("the render settings need a width and a height of 1 to {} and samples of 1 to {}",
            RenderSettings::LARGEST_SIDE, RenderSettings::MOST_SAMPLES);
        return;
    }
    panels_->closeSettings();
    if (*settings == scene_->renderSettings()) {
        return;
    }
    const boost::shared_ptr<SettingsCommand> command = boost::make_shared<SettingsCommand>(scene_, *settings);
    command->redo();
    history_->push(command);
}

/**
 **/
void Controller::renderBatch() {
    if (!activeView_ || !activeView_->camera()) {
        logger()->get()->info("a batch with no scene camera is seen through the active view, and there is none");
        return;
    }
    const std::vector<Batch::Job> jobs = batch_->start(scene_.get(), activeView_->camera()->profile(),
        scene_->renderSettings(), project_->directory() / "renders");
    logger()->get()->info("rendering {} frame(s) into {}", jobs.size(), (project_->directory() / "renders").string());
    syncCaption();
}

/**
 **/
void Controller::syncRender() {
    const Delegate::Status status = delegate_->status();
    const int done = static_cast<int>(status.done * 100.0F);  // checked: done is between 0 and 1
    if (status.state != shownState_ || done != shownDone_) {
        shownState_ = status.state;
        shownDone_ = done;
        switch (status.state) {
            case Delegate::State::Idle:
                panels_->status("Nothing rendered yet");
                break;
            case Delegate::State::Rendering:
                panels_->status("Rendering - " + std::to_string(done) + "%");
                break;
            case Delegate::State::Finished:
                panels_->status("Rendered");
                break;
            case Delegate::State::Stopped:
                panels_->status("Stopped");
                break;
            case Delegate::State::Failed:
                panels_->status("The render failed - the log says why");
                break;
        }
    }
    if (status.pictures != shownPictures_) {
        shownPictures_ = status.pictures;
        const boost::shared_ptr<v3d::image::Image> image = delegate_->picture();
        if (image && renderer_) {
            panels_->picture(v3d::ui::Image(renderer_->picture(image)), glm::uvec2(image->width(), image->height()));
        }
    }
}

/**
 **/
void Controller::goTo(int frame) {
    Timeline timeline = scene_->timeline();
    timeline.frame = std::clamp(frame, timeline.start, timeline.end);
    scene_->timeline(timeline);
    scene_->pose(static_cast<float>(timeline.frame));
    syncCaption();
}

/**
 **/
std::vector<float> Controller::keyedFrames() const {
    const Items selection = scene_->selection();
    return scene_->keyedFrames(selection.empty() ? scene_->items() : selection);
}

/**
 **/
void Controller::nextKey(bool forwards) {
    const float frame = static_cast<float>(scene_->timeline().frame);
    const std::vector<float> frames = keyedFrames();
    if (forwards) {
        const auto after = std::upper_bound(frames.begin(), frames.end(), frame);
        if (after != frames.end()) {
            goTo(wholeFrame(*after, scene_->timeline(), true));
        }
        return;
    }
    const auto before = std::lower_bound(frames.begin(), frames.end(), frame);
    if (before != frames.begin()) {
        goTo(wholeFrame(*(before - 1), scene_->timeline(), false));
    }
}

/**
 **/
void Controller::create(const boost::shared_ptr<Item>& item, const std::string& name) {
    // the command does the creating, so making an item and redoing one run the same code
    const boost::shared_ptr<CreateCommand> command = boost::make_shared<CreateCommand>(scene_, item, name);
    command->redo();
    history_->push(command);
    logger()->get()->info("created a {} - {} items", name, scene_->count());
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

    // the picture is named beside the export, so rendering the file writes it there
    const std::string picture = boost::filesystem::path(path_ + "export.png").generic_string();
    writeShot(&file, *scene_, shot(), RIBExportVisitor::Output{ .settings = scene_->renderSettings(), .file = picture });

    logger()->get()->info("exported {} item(s) to {}", scene_->count(), exportPath());
}

/**
 **/
void Controller::openProject() {
    askBeforeDiscarding([this]() {
        // a project is opened by its manifest
        chooser_->open(v3d::ui::shell::FileChooser::Mode::Open, project_->directory().parent_path(), ".json",
            [this](const boost::filesystem::path& chosen) {
                // a gesture under way is holding the mesh it started on, which the read is
                // about to take out of the scene
                transformTool_->cancel();
                if (!project_->open(chosen, scene_)) {
                    return;
                }
                sinceAutosave_ = 0;
                // the history describes the scene that was replaced, so none of it can be
                // undone against the new one. Clearing it also marks the new scene as saved
                history_->clear();
                offerRecovery();
            });
    });
}

/**
 **/
bool Controller::saveProject() {
    if (!project_->save(scene_)) {
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
    prompt_->open("Save the changes to " + project_->directory().filename().string() + "?", {
        { "Save", [this, proceed]() {
            // a project that could not be written still has changes to lose
            if (saveProject()) {
                proceed();
            }
        } },
        { "Discard", [this, proceed]() {
            // the changes are thrown away, so an autosave of them is not offered again
            project_->discardRecovery();
            proceed();
        } },
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
    // the name typed is the new project's directory, so no extension is added to it
    chooser_->open(v3d::ui::shell::FileChooser::Mode::Save, project_->directory().parent_path(), "",
        [this](const boost::filesystem::path& chosen) {
            if (project_->saveAs(chosen, scene_)) {
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
const std::vector<Controller::FixedView>& Controller::fixedViews() {
    static const std::vector<FixedView> views{
        {.command = "view::camera::front", .profile = "Front"},
        {.command = "view::camera::back", .profile = "Back"},
        {.command = "view::camera::left", .profile = "Left"},
        {.command = "view::camera::right", .profile = "Right"},
        {.command = "view::camera::top", .profile = "Top"},
        {.command = "view::camera::bottom", .profile = "Bottom"},
        {.command = "view::camera::perspective", .profile = "Perspective"},
    };
    return views;
}

/**
 **/
void Controller::look(const std::string& profile) {
    if (!activeView_) {
        return;
    }
    if (!profiles_->has(profile)) {
        logger()->get()->error("There is no camera profile called {}", profile);
        return;
    }
    activeView_->look(profiles_->get(profile));
    logger()->get()->info("{} shows the {} camera", activeView_->name(), profile);
}

/**
 **/
void Controller::toggleProjection() {
    if (!activeView_) {
        return;
    }
    const boost::shared_ptr<v3d::type::camera::Camera> camera = activeView_->camera();
    camera->orthographic(!camera->orthographic());
    logger()->get()->info("{} is {}", activeView_->name(), camera->orthographic() ? "orthographic" : "perspective");
}

/**
 **/
void Controller::transformMode(const std::string& name) {
    // choosing a manipulator asks for the handles, so a latched camera move lets them go
    cameraTool_->unlatch();
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
        // a press on a divider between views resizes them, and no tool sees it
        if (layout_->grab(cursor_.x, cursor_.y)) {
            return;
        }
    } else if (uiGrab_) {
        uiGrab_ = false;
        if (uiCursor_) {
            uiCursor_->release(cursor_);
        }
        return;
    } else if (layout_->grabbed()) {
        layout_->release();
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
    placeViews();
}

/**
 **/
void Controller::placeViews() {
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
    renderer_->draw();
    return true;
}

/**
 **/
bool Controller::release() {
    // a close with nothing unsaved leaves nothing to recover. A close with changes still
    // unsaved keeps the autosave, so they are offered back at the next start
    if (project_ && history_ && !history_->dirty()) {
        project_->discardRecovery();
    }
    // a render's thread and a batch's are waited for, before anything they report to goes
    if (delegate_) {
        delegate_->cancel();
    }
    if (batch_) {
        batch_->cancel();
    }
    // the screen draws through the renderer, so it goes first
    shell_.release();
    if (renderer_) {
        // the device has to be idle before the window it presents to is destroyed
        renderer_->shutdown();
    }
    return true;
}

/**
 **/
bool Controller::tick(unsigned int delta) {
    sinceAutosave_ += delta;
    if (sinceAutosave_ >= autosaveInterval) {
        sinceAutosave_ = 0;
        if (project_ && history_ && history_->dirty()) {
            project_->autosave(scene_);
        }
    }
    if (panels_ && panels_->viewVisible() && activeView_ && activeView_->camera()) {
        delegate_->update(*scene_, shot(), scene_->renderSettings());
        syncRender();
    }
    if (batch_) {
        syncCaption();
    }
    return Engine::tick(delta);
}

/**
 **/
bool Controller::simulate(float step) {
    if (playing_ && scene_) {
        const Timeline& timeline = scene_->timeline();
        played_ += step;
        const float length = 1.0F / timeline.rate;
        if (played_ >= length) {
            const int frames = static_cast<int>(played_ / length);
            played_ -= static_cast<float>(frames) * length;
            const int span = timeline.end - timeline.start + 1;
            goTo(timeline.start + ((timeline.frame - timeline.start + frames) % span));
        }
    }
    return Engine::simulate(step);
}

/**
 **/
void Controller::offerRecovery() {
    if (!project_->recoverable() || !prompt_) {
        return;
    }
    prompt_->open("Recover the unsaved changes to " + project_->directory().filename().string() + "?", {
        { "Recover", [this]() {
            transformTool_->cancel();
            if (project_->recover(scene_)) {
                // what was recovered is not what the document holds
                history_->clear();
                history_->markUnsaved();
            }
        } },
        { "Discard", [this]() { project_->discardRecovery(); } }
    }, {});
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

    if (layout_ && layout_->grabbed()) {
        layout_->drag(cursor_.x, cursor_.y);
        placeViews();
        return;
    }

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
