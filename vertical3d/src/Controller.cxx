/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
**/

#include "Controller.h"

#include <api/engine/CameraProfiles.h>
#include <api/engine/Engine.h>
#include <api/event/Event.h>
#include <api/event/State.h>
#include <api/event/kind/MouseMotion.h>
#include <api/event/kind/WindowResize.h>
#include <api/image/Image.h>
#include <api/shell/Shell.h>
#include <api/type/camera/Camera.h>
#include <api/type/camera/Profile.h>
#include <api/ui/Component.h>
#include <api/ui/Container.h>
#include <api/ui/Interface.h>
#include <api/ui/component/Button.h>
#include <api/ui/component/Toolbar.h>
#include <api/ui/component/menu/Item.h>
#include <api/ui/input/Cursor.h>
#include <api/ui/shell/FileChooser.h>
#include <api/ui/shell/Keyboard.h>
#include <api/ui/shell/Prompt.h>
#include <api/window/Window.h>
#include <vertical3d/src/control/Playback.h>
#include <vertical3d/src/control/Pointer.h>
#include <vertical3d/src/control/RenderWindow.h>
#include <vertical3d/src/document/Document.h>
#include <vertical3d/src/render/Batch.h>
#include <vertical3d/src/render/MoyaDelegate.h>
#include <vertical3d/src/render/RenderPanels.h>
#include <vertical3d/src/render/Shot.h>
#include <vertical3d/src/scene/Camera.h>
#include <vertical3d/src/scene/Light.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/scene/Timeline.h>
#include <vertical3d/src/selection/SelectMask.h>
#include <vertical3d/src/store/Project.h>
#include <vertical3d/src/tool/CameraControlTool.h>
#include <vertical3d/src/tool/SelectTool.h>
#include <vertical3d/src/tool/ToolRouter.h>
#include <vertical3d/src/tool/TransformTool.h>
#include <vertical3d/src/view/Shading.h>
#include <vertical3d/src/view/ViewLayout.h>
#include <vertical3d/src/view/ViewPort.h>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "Renderer.h"

#include <boost/filesystem/path.hpp>
#include <boost/make_shared.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

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
    shell_(this, shellOptions()) {
    registerCommands();
}

/**
 **/
bool Controller::start() {
    if (!manifest()) {
        logger()->get()->error("The editor needs its config to know what to draw");
        return false;
    }

    document_ = boost::make_shared<Document>(logger(), boost::filesystem::path(path_) / "project");
    playback_ = boost::make_shared<Playback>(document_);

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

    const boost::shared_ptr<Scene>& scene = document_->scene();
    cameraTool_ = boost::make_shared<CameraControlTool>();
    selectTool_ = boost::make_shared<SelectTool>(scene, logger());
    transformTool_ = boost::make_shared<TransformTool>(scene, logger());
    transformTool_->commands(document_->history());
    // one button drives three tools. With a camera move held or latched, the drag drives a
    // camera. Otherwise a handle of the selection under the cursor takes the press, and a
    // press no handle took picks
    tools_ = boost::make_shared<ToolRouter>();
    tools_->add(cameraTool_);
    tools_->add(transformTool_);
    tools_->add(selectTool_);

    motion_ = dispatcher()->sink<v3d::event::kind::MouseMotion>().connect<&Controller::handleMotion>(*this);
    resize_ = dispatcher()->sink<v3d::event::kind::WindowResize>().connect<&Controller::handleResize>(*this);

    renderer_ = boost::make_shared<Renderer>(window(), logger(), assets(), &shell_);
    renderer_->views(views_);
    renderer_->scene(scene);
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
    return std::ranges::all_of(layout_->views(), [this](const ViewLayout::View& view) {
        if (!profiles_->has(view.camera)) {
            logger()->get()->error("The layout names a camera profile that does not exist: {}", view.camera);
            return false;
        }
        views_.push_back(boost::make_shared<ViewPort>(view.name, profiles_->get(view.camera)));
        return true;
    });
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
    const boost::shared_ptr<v3d::ui::input::Cursor> cursor =
        boost::make_shared<v3d::ui::input::Cursor>(vgui, dispatcher(), renderer_->measure());
    pointer_ = boost::make_shared<Pointer>(cursor, layout_, views_, tools_, [this](const boost::shared_ptr<ViewPort>&) {
        // the show flags a check item draws are the active view's
        syncUi();
    });
    uiKeys_ = boost::make_shared<v3d::ui::shell::Keyboard>(vgui, dispatcher(), window());
    chooser_ = boost::make_shared<v3d::ui::shell::FileChooser>(vgui);
    prompt_ = boost::make_shared<v3d::ui::shell::Prompt>(vgui);
    // moya.exe is built beside the editor
    renderWindow_ = boost::make_shared<RenderWindow>(boost::make_shared<RenderPanels>(vgui),
        boost::make_shared<MoyaDelegate>(logger()), boost::make_shared<Batch>(logger(), boost::filesystem::path(path_) / MOYA),
        [this](const boost::shared_ptr<v3d::image::Image>& image) { return renderer_->picture(image); });

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

    const boost::shared_ptr<ViewPort> activeView = pointer_ ? pointer_->activeView() : nullptr;
    if (activeView) {
        mark("view::show::mesh", activeView->shows(ViewPort::SHOW_MESH));
        mark("view::show::camera", activeView->shows(ViewPort::SHOW_CAMERA));
        mark("view::show::light", activeView->shows(ViewPort::SHOW_LIGHT));
        mark("view::show::handle", activeView->shows(ViewPort::SHOW_HANDLE));
        mark("view::show::grid", activeView->shows(ViewPort::SHOW_GRID));
        const ViewPort::Display& shown = activeView->display();
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
        const v3d::type::camera::Camera& camera = *activeView->camera();
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
    if (!document_) {
        return;
    }
    std::string caption = "Vertical|3D - " + document_->project()->directory().filename().string() +
        (document_->dirty() ? " *" : "") + " - frame " + std::to_string(document_->scene()->timeline().frame);
    const Batch::Status batch = renderWindow_ ? renderWindow_->batchStatus() : Batch::Status();
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
void Controller::report(const Outcome& outcome) {
    if (!outcome.message.empty()) {
        logger()->get()->info("{}", outcome.message);
    }
}

/**
 **/
void Controller::masked(SelectMask mask, const std::string& name, const std::function<Outcome()>& operation) {
    if (selectTool_->mask() != mask) {
        logger()->get()->info("{} needs the {} select mask", name, selectMaskName(mask));
        return;
    }
    report(operation());
}

/**
 **/
void Controller::atView(const std::string& what, const std::function<void(const v3d::type::camera::Profile&)>& run) {
    const boost::shared_ptr<ViewPort> view = pointer_ ? pointer_->activeView() : nullptr;
    if (!view || !view->camera()) {
        logger()->get()->info("{} at a view, and there is no active view", what);
        return;
    }
    run(view->camera()->profile());
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
    const auto selection = [this]() { return document_->scene()->selection(); };

    // a key binding in mappings.json and a menu item in the ui config invoke a command
    // by these names, so all three have to spell them the same way
    for (const auto& [name, primitive] : std::vector<std::pair<const char*, Document::Primitive>>{
        { "cube", Document::Primitive::Cube }, { "plane", Document::Primitive::Plane },
        { "cylinder", Document::Primitive::Cylinder }, { "cone", Document::Primitive::Cone } }) {
        press(std::string("create::poly::") + name, [this, primitive]() { report(document_->createPrimitive(primitive)); });
    }
    for (const auto& [name, projection] : std::vector<std::pair<const char*, Camera::Projection>>{
        { "create::perspective::camera", Camera::Projection::Perspective },
        { "create::ortho::camera", Camera::Projection::Orthographic } }) {
        press(name, [this, projection]() {
            atView("a camera is made", [this, projection](const v3d::type::camera::Profile& at) {
                report(document_->createCamera(at, projection));
            });
        });
    }
    for (const auto& [name, type] : std::vector<std::pair<const char*, Light::Type>>{
        { "create::light::point", Light::Type::Point }, { "create::light::distant", Light::Type::Distant },
        { "create::light::spot", Light::Type::Spot } }) {
        press(name, [this, type]() {
            atView("a light is made", [this, type](const v3d::type::camera::Profile& at) { report(document_->createLight(at, type)); });
        });
    }
    press("create::group", [this, selection]() { report(document_->group(selection())); });
    press("timeline::set", [this, selection]() { report(document_->key(selection(), document_->scene()->timeline().frame)); });

    press("render::view", [this]() {
        atView("a render is seen", [this](const v3d::type::camera::Profile&) { renderWindow_->show(*document_->scene(), shot()); });
    });
    press("render::stop", [this]() { renderWindow_->stop(); });
    press("render::close", [this]() { renderWindow_->close(); });
    press("render::settings", [this]() { renderWindow_->openSettings(document_->scene()->renderSettings()); });
    press("render::settings::apply", [this]() { report(renderWindow_->applySettings(document_.get())); });
    press("render::settings::cancel", [this]() { renderWindow_->closeSettings(); });
    press("render::batch", [this]() {
        atView("a batch with no scene camera is seen", [this](const v3d::type::camera::Profile& view) {
            report(renderWindow_->batch(document_->scene().get(), view, document_->project()->directory() / "renders"));
        });
    });

    press("timeline::playback::play", [this]() { playback_->play(); });
    press("timeline::playback::stop", [this]() { playback_->stop(); });
    press("timeline::playback::next", [this]() { playback_->step(1); });
    press("timeline::playback::previous", [this]() { playback_->step(-1); });
    press("timeline::playback::first", [this]() { playback_->first(); });
    press("timeline::playback::last", [this]() { playback_->last(); });
    press("timeline::playback::next_key", [this, selection]() { playback_->nextKey(selection(), true); });
    press("timeline::playback::previous_key", [this, selection]() { playback_->nextKey(selection(), false); });
    press("timeline::playback::first_key", [this, selection]() { playback_->firstKey(selection()); });
    press("timeline::playback::last_key", [this, selection]() { playback_->lastKey(selection()); });

    press("transform::poly::split_edge", [this]() { masked(SelectMask::Edge, "split edge", [this]() { return document_->splitEdges(); }); });
    press("transform::poly::split_face", [this]() { masked(SelectMask::Vertex, "split face", [this]() { return document_->splitFace(); }); });
    press("transform::poly::extrude_face", [this]() {
        masked(SelectMask::Face, "extrude face", [this]() { return document_->extrudeFaces(); });
    });
    press("transform::poly::inset_face", [this]() { masked(SelectMask::Face, "inset face", [this]() { return document_->insetFaces(); }); });
    press("transform::poly::merge_vertices", [this]() {
        masked(SelectMask::Vertex, "merge vertices", [this]() { return document_->mergeVertices(); });
    });

    for (const char* mask : { "object", "vertex", "edge", "face", "mesh", "light", "camera" }) {
        press(std::string("select::mask::") + mask, [this, mask]() { selectMask(mask); });
    }

    // the Camera Manipulators menu latches a move, so a drag with no modifier drives the
    // camera until the move is chosen again or a transform mode is
    for (const char* move : { "truck", "dolly", "zoom", "pan", "pitch" }) {
        press(std::string("camera::") + move, [this, move]() { cameraTool_->latch(move); });
    }
    for (const char* mode : { "select", "translate", "rotate", "scale" }) {
        press(std::string("transform::") + mode, [this, mode]() { transformMode(mode); });
    }

    // how the active view draws. Each view keeps its own
    const auto display = [this](const std::function<void(ViewPort*)>& change) {
        if (pointer_ && pointer_->activeView()) {
            change(pointer_->activeView().get());
        }
    };
    press("view::shade::flat", [display]() { display([](ViewPort* view) { view->display().shading = Shading::Flat; }); });
    press("view::shade::smooth", [display]() { display([](ViewPort* view) { view->display().shading = Shading::Smooth; }); });
    for (const auto& [name, cull] : std::vector<std::pair<const char*, ViewPort::Cull>>{ { "none", ViewPort::Cull::None },
        { "front", ViewPort::Cull::Front }, { "back", ViewPort::Cull::Back }, { "both", ViewPort::Cull::Both } }) {
        press(std::string("view::cull::") + name, [display, cull]() { display([cull](ViewPort* view) { view->display().cull = cull; }); });
    }
    for (const auto& [name, fill] : std::vector<std::pair<const char*, ViewPort::Fill>>{ { "points", ViewPort::Fill::Points },
        { "lines", ViewPort::Fill::Lines }, { "fill", ViewPort::Fill::Fill } }) {
        press(std::string("view::rendermode::front::") + name,
            [display, fill]() { display([fill](ViewPort* view) { view->display().front = fill; }); });
        press(std::string("view::rendermode::back::") + name,
            [display, fill]() { display([fill](ViewPort* view) { view->display().back = fill; }); });
    }
    for (const auto& [name, filter] : std::vector<std::pair<const char*, ViewPort::VisibleFilter>>{
        { "grid", ViewPort::SHOW_GRID }, { "mesh", ViewPort::SHOW_MESH }, { "camera", ViewPort::SHOW_CAMERA },
        { "light", ViewPort::SHOW_LIGHT }, { "handle", ViewPort::SHOW_HANDLE } }) {
        press(std::string("view::show::") + name,
            [display, filter]() { display([filter](ViewPort* view) { view->show(filter, !view->shows(filter)); }); });
    }
    for (const FixedView& view : fixedViews()) {
        const std::string profile = view.profile;
        press(view.command, [this, profile]() { look(profile); });
    }
    press("view::camera::orthographic", [this]() { toggleProjection(); });
    // view::show::statistics is not a view flag: the readout covers the window rather than one
    // pane, so the shell shows it and nothing here reads it back

    // these are held: the modifier going down chooses what a drag of the primary button
    // performs, and it coming up puts the tool back to whatever is latched
    for (const char* move : { "zoom", "dolly", "truck" }) {
        hold(std::string("view::camera::") + move, [this, move](const v3d::event::Event& event) {
            cameraMode(move, event.state() != v3d::event::State::Released);
        });
    }
    // pan is a button of its own, so holding it is the drag. It drives the camera alone,
    // whatever the primary button is doing
    hold("view::camera::pan", [this](const v3d::event::Event& event) {
        const bool pressed = event.state() != v3d::event::State::Released;
        cameraMode("pan", pressed);
        cameraTool_->button(1, pressed, pointer_ ? pointer_->cursor() : glm::vec2(0.0F, 0.0F));
    });
    // held: while it is down a click adds to the selection or takes away from it
    hold("select::add", [this](const v3d::event::Event& event) {
        selectTool_->adding(event.state() != v3d::event::State::Released);
    });

    hold("view::drag", [this](const v3d::event::Event& event) {
        if (pointer_) {
            pointer_->button(event.state() == v3d::event::State::Pressed);
        }
    });

    press("project::load", [this]() { openProject(); });
    press("project::save", [this]() { document_->save(); });
    press("project::saveAs", [this]() { saveProjectAs(); });
    press("project::chooser::pick", [this]() { chooser_->pick(); });
    press("project::chooser::accept", [this]() { chooser_->accept(); });
    press("project::chooser::cancel", [this]() { chooser_->close(); });
    press("project::export::rib", [this]() { exportProject(); });
    press("project::import::gltf", [this]() { importScene({ ".gltf", ".glb" }); });
    press("project::import::usd", [this]() { importScene({ ".usd", ".usda", ".usdc", ".usdz" }); });
    // the prompt's buttons, in the order its answers are given
    press("project::prompt::answer0", [this]() { prompt_->answer(0); });
    press("project::prompt::answer1", [this]() { prompt_->answer(1); });
    press("project::prompt::answer2", [this]() { prompt_->answer(2); });

    press("edit::undo", [this]() { report(document_->undo()); });
    press("edit::redo", [this]() { report(document_->redo()); });
}

/**
 **/
v3d::type::camera::Profile Controller::shot() const {
    return shotFrom(*document_->scene(), pointer_->activeView()->camera()->profile());
}

/**
 **/
void Controller::exportProject() {
    atView("a scene is exported", [this](const v3d::type::camera::Profile&) {
        // the picture is named beside the export, so rendering the file writes it there
        const boost::filesystem::path directory(path_);
        report(document_->exportRib(directory / "export.rib", shot(), (directory / "export.png").generic_string()));
    });
}

/**
 **/
void Controller::importScene(const std::vector<std::string>& extensions) {
    chooser_->open(v3d::ui::shell::FileChooser::Mode::Open, document_->project()->directory().parent_path(), extensions,
        [this](const boost::filesystem::path& chosen) { report(document_->importScene(chosen)); });
}

/**
 **/
void Controller::openProject() {
    askBeforeDiscarding([this]() {
        // a project is opened by its manifest
        chooser_->open(v3d::ui::shell::FileChooser::Mode::Open, document_->project()->directory().parent_path(), { ".json" },
            [this](const boost::filesystem::path& chosen) {
                // a gesture under way is holding the mesh it started on, which the read is
                // about to take out of the scene
                transformTool_->cancel();
                if (document_->open(chosen)) {
                    offerRecovery();
                }
            });
    });
}

/**
 **/
void Controller::askBeforeDiscarding(const std::function<void()>& proceed) {
    if (!document_->dirty() || !prompt_) {
        proceed();
        return;
    }
    prompt_->open("Save the changes to " + document_->project()->directory().filename().string() + "?", {
        { "Save", [this, proceed]() {
            // a project that could not be written still has changes to lose
            if (document_->save()) {
                proceed();
            }
        } },
        { "Discard", [this, proceed]() {
            // the changes are thrown away, so an autosave of them is not offered again
            document_->discardRecovery();
            proceed();
        } },
        { "Cancel", {} }
    }, {});
}

/**
 **/
bool Controller::quitRequested() {
    if (!document_ || !document_->dirty()) {
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
    chooser_->open(v3d::ui::shell::FileChooser::Mode::Save, document_->project()->directory().parent_path(), {},
        [this](const boost::filesystem::path& chosen) { document_->saveAs(chosen); });
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
    const boost::shared_ptr<ViewPort> view = pointer_ ? pointer_->activeView() : nullptr;
    if (!view) {
        return;
    }
    if (!profiles_->has(profile)) {
        logger()->get()->error("There is no camera profile called {}", profile);
        return;
    }
    view->look(profiles_->get(profile));
    logger()->get()->info("{} shows the {} camera", view->name(), profile);
}

/**
 **/
void Controller::toggleProjection() {
    const boost::shared_ptr<ViewPort> view = pointer_ ? pointer_->activeView() : nullptr;
    if (!view) {
        return;
    }
    const boost::shared_ptr<v3d::type::camera::Camera> camera = view->camera();
    camera->orthographic(!camera->orthographic());
    logger()->get()->info("{} is {}", view->name(), camera->orthographic() ? "orthographic" : "perspective");
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
void Controller::layoutViews(int width, int height) {
    if (!pointer_) {
        return;
    }
    // the menu bar and the toolbars cover strips along two edges, and the views divide
    // what is left
    const glm::vec2 inset = renderer_ ? renderer_->insets() : glm::vec2(0.0F, 0.0F);
    pointer_->resize(glm::vec4(inset.x, inset.y,
        std::max(0.0F, static_cast<float>(width) - inset.x),
        std::max(0.0F, static_cast<float>(height) - inset.y)));
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
    if (document_) {
        document_->close();
    }
    // a render's thread and a batch's are waited for, before anything they report to goes
    if (renderWindow_) {
        renderWindow_->release();
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
    if (document_) {
        document_->tick(delta);
    }
    if (renderWindow_ && renderWindow_->open() && pointer_ && pointer_->activeView() && pointer_->activeView()->camera()) {
        renderWindow_->tick(*document_->scene(), shot());
    }
    if (renderWindow_) {
        syncCaption();
    }
    return Engine::tick(delta);
}

/**
 **/
bool Controller::simulate(float step) {
    if (playback_) {
        const bool playing = playback_->playing();
        playback_->simulate(step);
        if (playing) {
            syncCaption();
        }
    }
    return Engine::simulate(step);
}

/**
 **/
void Controller::offerRecovery() {
    if (!document_->recoverable() || !prompt_) {
        return;
    }
    prompt_->open("Recover the unsaved changes to " + document_->project()->directory().filename().string() + "?", {
        { "Recover", [this]() {
            transformTool_->cancel();
            document_->recover();
        } },
        { "Discard", [this]() { document_->discardRecovery(); } }
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
    if (pointer_) {
        pointer_->motion(event.position());
    }
}

};  // namespace v3d::editor
