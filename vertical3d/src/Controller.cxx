/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
**/

#include "Controller.h"

#include <api/asset/File.h>
#include <api/asset/Json.h>
#include <api/asset/Writer.h>
#include <api/engine/CameraProfiles.h>
#include <api/engine/Engine.h>
#include <api/engine/Settings.h>
#include <api/event/Event.h>
#include <api/event/State.h>
#include <api/event/kind/MouseButton.h>
#include <api/event/kind/MouseMotion.h>
#include <api/event/kind/WindowResize.h>
#include <api/image/Image.h>
#include <api/input/MouseState.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/shell/Shell.h>
#include <api/type/camera/Camera.h>
#include <api/type/camera/Profile.h>
#include <api/type/geometry/Bound2D.h>
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
#include <vertical3d/src/control/AreaChrome.h>
#include <vertical3d/src/control/CommandSearch.h>
#include <vertical3d/src/control/IconSheet.h>
#include <vertical3d/src/control/Notices.h>
#include <vertical3d/src/control/Outliner.h>
#include <vertical3d/src/control/PropertiesEditor.h>
#include <vertical3d/src/control/Playback.h>
#include <vertical3d/src/control/Pointer.h>
#include <vertical3d/src/control/RenderWindow.h>
#include <vertical3d/src/control/StatusBar.h>
#include <vertical3d/src/control/TopBar.h>
#include <vertical3d/src/control/Workspaces.h>
#include <vertical3d/src/document/Document.h>
#include <vertical3d/src/render/Batch.h>
#include <vertical3d/src/render/MoyaDelegate.h>
#include <vertical3d/src/render/RenderPanels.h>
#include <vertical3d/src/render/Shot.h>
#include <vertical3d/src/scene/Camera.h>
#include <vertical3d/src/scene/CreateItem.h>
#include <vertical3d/src/scene/Light.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/scene/Timeline.h>
#include <vertical3d/src/selection/SelectMask.h>
#include <vertical3d/src/store/Project.h>
#include <vertical3d/src/tool/CameraControlTool.h>
#include <vertical3d/src/tool/SelectTool.h>
#include <vertical3d/src/tool/ToolRouter.h>
#include <vertical3d/src/tool/TransformTool.h>
#include <vertical3d/src/view/AreaLayout.h>
#include <vertical3d/src/view/AreaViews.h>
#include <vertical3d/src/view/Shading.h>
#include <vertical3d/src/view/ViewOverlay.h>
#include <vertical3d/src/view/ViewPort.h>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keycode.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <variant>
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

/**
 * The first container the ui config lists after the editor's own. The top bar goes before it,
 * so it is drawn over the menu bar and under every dialog.
 **/
const char* const firstDialog = "chooser";

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

    workspaces_ = boost::make_shared<Workspaces>(logger());
    const boost::json::object* shipped = document(AreaLayout::DOCUMENT);
    if (shipped == nullptr || !workspaces_->load(*shipped)) {
        logger()->get()->error("The editor has no workspace it can read, so it would have no views");
        return false;
    }
    std::string first = std::string(v3d::asset::readString(workspaces_->all().front(), "name").value_or(std::string()));
    if (const boost::shared_ptr<v3d::engine::Settings>& store = settings()) {
        workspaces_->restore({ .saved = store->value(std::string(Workspaces::SAVED)),
            .projects = store->value(std::string(Workspaces::PROJECTS)) });
        const std::size_t pruned = workspaces_->prune([](const std::string& project) { return boost::filesystem::is_directory(project); });
        if (pruned > 0) {
            logger()->get()->info("Forgot the arrangements of projects that no longer exist: {}", pruned);
        }
        first = store->text(std::string(Workspaces::LAST)).value_or(first);
    }
    // the project's own record wins over the workspace last shown
    if (const std::optional<Workspaces::Record> record = workspaces_->recall(projectKey())) {
        first = record->workspace;
    }
    layout_ = boost::make_shared<AreaLayout>(logger());
    if (!showWorkspace(first) && !showWorkspace(std::string(v3d::asset::readString(workspaces_->all().front(), "name").value_or(std::string())))) {
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
    button_ = dispatcher()->sink<v3d::event::kind::MouseButton>().connect<&Controller::handleButton>(*this);
    resize_ = dispatcher()->sink<v3d::event::kind::WindowResize>().connect<&Controller::handleResize>(*this);

    renderer_ = boost::make_shared<Renderer>(window(), logger(), assets(), &shell_);
    IconSheet icons;
    const boost::json::object* sheet = document(IconSheet::DOCUMENT);
    if (sheet == nullptr || !icons.load(*sheet)) {
        logger()->get()->error("The editor has no icon sheet it can read, so its toolbar shows labels");
    }
    renderer_->icons(icons);
    renderer_->views(views_);
    renderer_->scene(scene);
    renderer_->transformTool(transformTool_);
    renderer_->selectTool(selectTool_);
    notices_ = boost::make_shared<Notices>();
    renderer_->notices(notices_);

    if (!buildUi()) {
        return false;
    }
    renderer_->ui(shell_.ui());
    renderer_->pointer(pointer_);

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

    logger()->get()->info("{} with {} areas", layout_->name(), layout_->areas().size());

    // the editor starts on the default project without reading it, so an autosave a crash left
    // there is the one way back to that session's work
    offerRecovery();
    return true;
}

/**
 **/
bool Controller::buildViews() {
    views_ = boost::make_shared<AreaViews>(layout_, [this](const AreaLayout::Editor& editor) -> boost::shared_ptr<ViewPort> {
        const std::optional<std::string> camera = v3d::asset::readString(editor.state, "camera");
        if (!camera || !profiles_->has(*camera)) {
            logger()->get()->error("A viewport in the layout names no camera profile that exists");
            return nullptr;
        }
        // the name is the view's pass name, so the editor's id keeps two views of a profile apart
        return boost::make_shared<ViewPort>(*camera + " " + std::to_string(editor.id), profiles_->get(*camera));
    });
    return views_->sync();
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
    pointer_ = boost::make_shared<Pointer>(cursor, views_, tools_, [this](const boost::shared_ptr<ViewPort>&) {
        // the show flags a check item draws are the active view's
        syncUi();
    });
    pointer_->tabs([this](const glm::vec2& point) { return chrome_ ? chrome_->tabAt(point) : std::nullopt; });
    // a press on a ball of a view's gizmo turns its camera to look down that axis
    pointer_->controls([this](const glm::vec2& point) {
        const boost::shared_ptr<ViewPort> view = views_->of(layout_->areaAt(point.x, point.y));
        const std::optional<AxisEnd> end = view ? gizmoAt(*view, renderer_->overlay(), point) : std::nullopt;
        if (!end) {
            return false;
        }
        lookAlong(view->camera().get(), *end);
        syncUi();
        return true;
    });
    // the areas' headers go under the editor's own container, so its menus drop over them
    chrome_ = boost::make_shared<AreaChrome>(vgui, uiContainer,
        [this](const boost::shared_ptr<v3d::ui::Component>& component) { renderer_->resolve(component); });
    uiKeys_ = boost::make_shared<v3d::ui::shell::Keyboard>(vgui, dispatcher(), window());
    uiKeys_->immediate(renderer_->immediate());
    properties_ = boost::make_shared<PropertiesEditor>(document_->history());
    renderer_->panels([this](v3d::ui::Immediate* layer, v3d::render::realtime::canvas::Canvas* canvas) { drawPanels(layer, canvas); });
    chooser_ = boost::make_shared<v3d::ui::shell::FileChooser>(vgui);
    prompt_ = boost::make_shared<v3d::ui::shell::Prompt>(vgui);
    search_ = boost::make_shared<CommandSearch>(vgui);
    topBar_ = boost::make_shared<TopBar>(vgui, firstDialog, renderer_->measure());
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

    // the View menu marks the active view's state, as each header marks its own view's
    const boost::shared_ptr<ViewPort> activeView = pointer_ ? pointer_->activeView() : nullptr;
    if (activeView) {
        for (const auto& [command, on] : AreaChrome::marks(*activeView)) {
            mark(command, on);
        }
    }

    for (const auto& [command, on] : AreaChrome::marks(selectTool_->mask())) {
        mark(command, on);
    }
    const CameraControlTool::CameraMode latched = cameraTool_->latched();
    mark("camera::truck", latched == CameraControlTool::CAMERA_MODE_TRUCK);
    mark("camera::dolly", latched == CameraControlTool::CAMERA_MODE_DOLLY);
    mark("camera::zoom", latched == CameraControlTool::CAMERA_MODE_ZOOM);
    mark("camera::pan", latched == CameraControlTool::CAMERA_MODE_PAN);
    mark("camera::pitch", latched == CameraControlTool::CAMERA_MODE_PITCH);

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
        if (notices_) {
            notices_->post(outcome.message);
        }
    }
}

/**
 **/
TopBar::State Controller::topState() const {
    TopBar::State state{ .workspaces = {}, .shown = workspace_, .project = document_->project()->directory().filename().string(),
        .dirty = document_->dirty() };
    for (const boost::json::object& workspace : workspaces_->all()) {
        state.workspaces.emplace_back(v3d::asset::readString(workspace, "name").value_or(std::string()));
    }
    return state;
}

/**
 **/
void Controller::openSearch() {
    std::vector<boost::shared_ptr<v3d::ui::component::menu::Bar>> bars{ menu_ };
    // an area's commands are on its header, and run from the search they act on the active area
    if (!layout_->areas().empty()) {
        const boost::shared_ptr<v3d::ui::Container> area = shell_.ui()->container(AreaChrome::name(layout_->areas().front().id));
        if (area) {
            bars.push_back(boost::dynamic_pointer_cast<v3d::ui::component::menu::Bar>(area->get("header")));
        }
    }
    std::vector<CommandSearch::Entry> entries = CommandSearch::catalogue(bars);
    for (CommandSearch::Entry& entry : entries) {
        entry.shortcut = shell_.screen()->shortcut(v3d::event::Event(entry.command));
    }
    search_->open(entries);
}

/**
 **/
void Controller::runSearch() {
    const std::optional<std::string> command = search_->chosen();
    search_->close();
    if (command && !commands().invoke(v3d::event::Event(*command))) {
        refuse("nothing runs " + *command);
    }
}

/**
 **/
void Controller::refuse(const std::string& message) {
    report({ .done = false, .message = message });
}

/**
 **/
Status Controller::status() const {
    Status made;
    const boost::shared_ptr<ViewPort> view = pointer_ ? pointer_->activeView() : nullptr;
    const bool perspective = view && view->camera() && !view->camera()->orthographic();
    made.buttons = StatusBar::buttons(*cameraTool_, *transformTool_, *selectTool_, perspective);
    made.selection = StatusBar::selection(*document_->scene(), selectTool_->mask());
    made.frame = "Frame " + std::to_string(document_->scene()->timeline().frame);
    made.rate = StatusBar::rate(statistics().mean());
    return made;
}

/**
 **/
void Controller::masked(SelectMask mask, const std::string& name, const std::function<Outcome()>& operation) {
    if (selectTool_->mask() != mask) {
        refuse(name + " needs the " + selectMaskName(mask) + " select mask");
        return;
    }
    report(operation());
}

/**
 **/
void Controller::atView(const std::string& what, const std::function<void(const v3d::type::camera::Profile&)>& run) {
    const boost::shared_ptr<ViewPort> view = pointer_ ? pointer_->activeView() : nullptr;
    if (!view || !view->camera()) {
        refuse(what + " at a view, and there is no active view");
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
    for (const auto& [name, type] : std::vector<std::pair<const char*, CurveType>>{
        { "create::curve::poly", CurveType::Poly }, { "create::curve::bezier", CurveType::Bezier },
        { "create::curve::catmull_rom", CurveType::CatmullRom }, { "create::curve::nurbs", CurveType::Nurbs } }) {
        press(name, [this, type]() { report(document_->createCurve(type)); });
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

    for (const char* mask : { "object", "vertex", "edge", "face", "mesh", "light", "camera", "curve", "handle" }) {
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
            pointer_->button(event.state() == v3d::event::State::Pressed, clicks_);
            syncPointerShape();
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

    press("search::open", [this]() { openSearch(); });
    press("search::run", [this]() { runSearch(); });

    press("edit::undo", [this]() { report(document_->undo()); });
    press("edit::redo", [this]() { report(document_->redo()); });

    registerViewCommands();
    registerAreaCommands();
    registerWorkspaceCommands();
}

/**
 **/
void Controller::registerViewCommands() {
    // how a view draws. Each view keeps its own. A header's command names its area by id, and
    // one from the View menu or a key acts on the active area
    auto display = [this](const std::string& name, const std::function<void(std::size_t area, ViewPort* view)>& change) {
        const bool added = commands().add(name, [this, change](const v3d::event::Event& event) {
            if (event.state() == v3d::event::State::Released || event.repeat()) {
                return;
            }
            const std::size_t area = areaOf(event);
            if (const boost::shared_ptr<ViewPort> view = views_ ? views_->of(area) : nullptr) {
                change(area, view.get());
            }
            syncUi();
            syncCaption();
        });
        if (!added) {
            refused_.push_back(name);
        }
    };
    display("view::shade::flat", [](std::size_t, ViewPort* view) { view->display().shading = Shading::Flat; });
    display("view::shade::smooth", [](std::size_t, ViewPort* view) { view->display().shading = Shading::Smooth; });
    for (const auto& [name, cull] : std::vector<std::pair<const char*, ViewPort::Cull>>{ { "none", ViewPort::Cull::None },
        { "front", ViewPort::Cull::Front }, { "back", ViewPort::Cull::Back }, { "both", ViewPort::Cull::Both } }) {
        display(std::string("view::cull::") + name, [cull](std::size_t, ViewPort* view) { view->display().cull = cull; });
    }
    for (const auto& [name, fill] : std::vector<std::pair<const char*, ViewPort::Fill>>{ { "points", ViewPort::Fill::Points },
        { "lines", ViewPort::Fill::Lines }, { "fill", ViewPort::Fill::Fill } }) {
        display(std::string("view::rendermode::front::") + name, [fill](std::size_t, ViewPort* view) { view->display().front = fill; });
        display(std::string("view::rendermode::back::") + name, [fill](std::size_t, ViewPort* view) { view->display().back = fill; });
    }
    for (const auto& [name, filter] : std::vector<std::pair<const char*, ViewPort::VisibleFilter>>{
        { "grid", ViewPort::SHOW_GRID }, { "mesh", ViewPort::SHOW_MESH }, { "camera", ViewPort::SHOW_CAMERA },
        { "light", ViewPort::SHOW_LIGHT }, { "handle", ViewPort::SHOW_HANDLE } }) {
        display(std::string("view::show::") + name, [filter](std::size_t, ViewPort* view) { view->show(filter, !view->shows(filter)); });
    }
    for (const AreaChrome::FixedView& fixed : AreaChrome::fixedViews()) {
        const std::string profile = fixed.profile;
        display(fixed.command, [this, profile](std::size_t area, ViewPort* view) { look(area, view, profile); });
    }
    display("view::camera::orthographic", [this](std::size_t, ViewPort* view) { toggleProjection(view); });
    // view::show::statistics is not a view flag: the readout covers the window rather than one
    // pane, so the shell shows it and nothing here reads it back
}

/**
 **/
void Controller::registerAreaCommands() {
    auto hold = [this](const std::string& name, const v3d::event::Commands::Handler& handler) {
        if (!commands().add(name, [this, handler](const v3d::event::Event& event) {
            handler(event);
            areasChanged();
        })) {
            refused_.push_back(name);
        }
    };
    // a header's command names its area by id, and one sent by a key acts on the active area
    const std::pair<const char*, AreaLayout::Edge> edges[] = {
        { "left", AreaLayout::Edge::Left }, { "right", AreaLayout::Edge::Right },
        { "top", AreaLayout::Edge::Top }, { "bottom", AreaLayout::Edge::Bottom },
    };
    for (const auto& [word, edge] : edges) {
        hold(std::string("area::split::") + word, [this, edge](const v3d::event::Event& event) {
            layout_->split(areaOf(event), edge);
        });
        hold(std::string("area::join::") + word, [this, edge](const v3d::event::Event& event) {
            const std::size_t area = areaOf(event);
            layout_->join(area, layout_->neighbour(area, edge));
        });
        hold(std::string("area::swap::") + word, [this, edge](const v3d::event::Event& event) {
            const std::size_t area = areaOf(event);
            layout_->swap(area, layout_->neighbour(area, edge));
        });
    }
    hold("area::maximise", [this](const v3d::event::Event& event) {
        if (event.state() != v3d::event::State::Released) {
            layout_->maximise(areaOf(event));
        }
    });
    hold("area::tab::new", [this](const v3d::event::Event& event) {
        const std::size_t area = areaOf(event);
        if (area >= layout_->areas().size()) {
            return;
        }
        // a new tab shows the camera the area shows, or the perspective view
        const AreaLayout::Editor& shown = layout_->areas()[area].editor();
        const std::optional<std::string> camera = v3d::asset::readString(shown.state, "camera");
        layout_->add(area, boost::json::object{ { "kind", AreaViews::KIND }, { "camera", camera.value_or("Perspective") } });
    });
    for (const AreaChrome::Kind& kind : AreaChrome::kinds()) {
        const std::string named = kind.kind;
        hold("area::editor::" + named, [this, named](const v3d::event::Event& event) {
            const std::size_t area = areaOf(event);
            if (area >= layout_->areas().size() || layout_->areas()[area].editor().kind == named) {
                return;
            }
            // a viewport made this way starts from the perspective view
            boost::json::object state{ { "kind", named } };
            if (named == AreaViews::KIND) {
                state["camera"] = "Perspective";
            }
            layout_->replace(area, state);
        });
    }
    hold("area::tab::close", [this](const v3d::event::Event& event) {
        const std::size_t area = areaOf(event);
        if (area < layout_->areas().size()) {
            layout_->close(area, layout_->areas()[area].chosen);
        }
    });
    hold("area::tab::choose", [this](const v3d::event::Event& event) {
        const boost::optional<v3d::event::EventData> data = event.data();
        if (!data || !std::holds_alternative<int>(*data)) {
            return;
        }
        const std::size_t editor = static_cast<std::size_t>(std::get<int>(*data));
        for (std::size_t area = 0; area < layout_->areas().size(); area++) {
            const std::vector<AreaLayout::Editor>& editors = layout_->areas()[area].editors;
            const auto found = std::ranges::find(editors, editor, &AreaLayout::Editor::id);
            if (found != editors.end()) {
                layout_->choose(area, static_cast<std::size_t>(found - editors.begin()));
                return;
            }
        }
    });
}

/**
 **/
void Controller::registerWorkspaceCommands() {
    auto hold = [this](const std::string& name, const v3d::event::Commands::Handler& handler) {
        if (!commands().add(name, [this, handler](const v3d::event::Event& event) {
            handler(event);
            areasChanged();
        })) {
            refused_.push_back(name);
        }
    };
    hold("workspace::next", [this](const v3d::event::Event& event) {
        if (event.state() != v3d::event::State::Released) {
            switchWorkspace(1);
        }
    });
    hold("workspace::previous", [this](const v3d::event::Event& event) {
        if (event.state() != v3d::event::State::Released) {
            switchWorkspace(-1);
        }
    });
    hold("workspace::show", [this](const v3d::event::Event& event) {
        const boost::optional<v3d::event::EventData> data = event.data();
        if (event.state() == v3d::event::State::Released || !data || !std::holds_alternative<std::string>(*data)) {
            return;
        }
        const std::string name = std::get<std::string>(*data);
        if (name == workspace_) {
            return;
        }
        // the arrangement left behind is kept, so coming back to the workspace finds it
        recordProject();
        if (showWorkspace(name)) {
            recordProject();
            logger()->get()->info("Showing the {} workspace", workspace_);
        }
    });
    hold("workspace::save", [this](const v3d::event::Event&) {
        boost::json::object layout = layout_->save();
        layout["name"] = workspace_;
        workspaces_->keep(layout);
        recordProject();
    });
    hold("workspace::reset", [this](const v3d::event::Event&) {
        workspaces_->reset(workspace_);
        workspaces_->forget({ .project = projectKey(), .workspace = workspace_ });
        showWorkspace(workspace_);
        recordProject();
    });
    hold("workspace::export", [this](const v3d::event::Event&) { exportWorkspace(); });
    hold("workspace::import", [this](const v3d::event::Event&) { importWorkspace(); });
}

/**
 **/
std::size_t Controller::areaOf(const v3d::event::Event& event) const {
    const boost::optional<v3d::event::EventData> data = event.data();
    if (data && std::holds_alternative<int>(*data)) {
        return layout_->find(static_cast<std::size_t>(std::get<int>(*data)));
    }
    return pointer_ ? pointer_->activeArea() : layout_->areas().size();
}

/**
 **/
void Controller::drawPanels(v3d::ui::Immediate* layer, v3d::render::realtime::canvas::Canvas* canvas) {
    v3d::ui::Immediate::Input input;
    const v3d::input::MouseState* held = mouse();
    if (held != nullptr && pointer_ && pointer_->panels()) {
        input.cursor = pointer_->cursor();
        input.down = held->held("left");
        input.pressed = held->pressed("left");
        input.released = held->released("left");
        input.wheel = held->wheel();
    }
    layer->begin(canvas, input);
    // a panel starts under its area's header
    const float header = renderer_->barHeight();
    std::vector<std::size_t> outliners;
    for (const AreaLayout::Area& area : layout_->areas()) {
        const glm::vec4 region(area.content.x, area.content.y + header, area.content.z, area.content.w - header);
        const AreaLayout::Editor& editor = area.editor();
        if (region.z <= 0.0F || region.w <= 0.0F) {
            continue;
        }
        if (editor.kind == Outliner::KIND) {
            outliner_.draw(layer, document_->scene().get(), editor.id, region, selectTool_->adding());
            outliners.push_back(editor.id);
        } else if (editor.kind == PropertiesEditor::KIND) {
            properties_->draw(layer, document_->scene().get(), editor.id, region);
        }
    }
    layer->end();
    outliner_.keep(outliners);
    if (pointer_) {
        pointer_->settle();
    }
}

/**
 **/
void Controller::areasChanged() {
    if (pointer_) {
        pointer_->place();
    }
    syncPointerShape();
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
                recordProject();
                if (document_->open(chosen)) {
                    projectChanged();
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
    // Escape while a divider is held puts the divider back rather than quitting
    if (pointer_ && pointer_->cancel()) {
        syncPointerShape();
        return false;
    }
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
        [this](const boost::filesystem::path& chosen) {
            if (document_->saveAs(chosen)) {
                projectChanged();
            }
        });
}

/**
 **/
std::string Controller::projectKey() const {
    if (!document_) {
        return {};
    }
    return boost::filesystem::absolute(document_->project()->directory()).lexically_normal().generic_string();
}

/**
 **/
bool Controller::showWorkspace(const std::string& name) {
    const std::size_t index = workspaces_->find(name);
    if (index >= workspaces_->all().size()) {
        return false;
    }
    // the project's own arrangement of the workspace, when it has one, and the workspace as it
    // is kept otherwise
    const std::optional<Workspaces::Record> record = workspaces_->recall(projectKey());
    const auto arranged = record ? record->layouts.find(name) : decltype(record->layouts.find(name))();
    const bool own = record && arranged != record->layouts.end();
    if (!layout_->load(own ? arranged->second : workspaces_->all()[index]) && (!own || !layout_->load(workspaces_->all()[index]))) {
        return false;
    }
    workspace_ = name;
    if (views_) {
        areasChanged();
    }
    return true;
}

/**
 **/
void Controller::recordProject() {
    if (!workspaces_ || !layout_ || workspace_.empty()) {
        return;
    }
    workspaces_->remember({ .project = projectKey(), .workspace = workspace_ }, layout_->save());
    saveSettings();
}

/**
 **/
void Controller::projectChanged() {
    const std::optional<Workspaces::Record> record = workspaces_->recall(projectKey());
    if (record && showWorkspace(record->workspace)) {
        return;
    }
    // a project with no record keeps the arrangement it was opened or saved in
    recordProject();
}

/**
 **/
void Controller::saveSettings() {
    const boost::shared_ptr<v3d::engine::Settings>& store = settings();
    if (!store) {
        return;
    }
    store->set(std::string(Workspaces::PROJECTS), workspaces_->projects());
    if (const std::optional<boost::json::value> own = workspaces_->saved()) {
        store->set(std::string(Workspaces::SAVED), *own);
    }
    store->set(std::string(Workspaces::LAST), workspace_);
    store->save();
}

/**
 **/
void Controller::switchWorkspace(int step) {
    const std::size_t count = workspaces_->all().size();
    if (count < 2) {
        return;
    }
    // the arrangement left behind is kept, so coming back to the workspace finds it
    recordProject();
    const std::size_t at = workspaces_->find(workspace_);
    const std::size_t from = at < count ? at : 0;
    const std::size_t next = (from + count + static_cast<std::size_t>(step + static_cast<int>(count))) % count;
    showWorkspace(std::string(v3d::asset::readString(workspaces_->all()[next], "name").value_or(std::string())));
    recordProject();
    logger()->get()->info("Showing the {} workspace", workspace_);
}

/**
 **/
void Controller::exportWorkspace() {
    chooser_->open(v3d::ui::shell::FileChooser::Mode::Save, document_->project()->directory().parent_path(), { ".json" },
        [this](const boost::filesystem::path& chosen) {
            boost::json::object layout = layout_->save();
            layout["name"] = workspace_;
            boost::filesystem::path file = chosen;
            if (file.extension() != ".json") {
                file += ".json";
            }
            if (!v3d::asset::writeDocument(file, boost::json::value(Workspaces::exported(layout)))) {
                logger()->get()->error("The workspace could not be written to {}", file.generic_string());
                return;
            }
            logger()->get()->info("Exported the {} workspace to {}", workspace_, file.generic_string());
        });
}

/**
 **/
void Controller::importWorkspace() {
    chooser_->open(v3d::ui::shell::FileChooser::Mode::Open, document_->project()->directory().parent_path(), { ".json" },
        [this](const boost::filesystem::path& chosen) {
            const std::optional<std::string> text = v3d::asset::readFile(chosen.string());
            boost::system::error_code error;
            const boost::json::value file = text ? boost::json::parse(*text, error) : boost::json::value();
            const std::vector<std::string> names = !error && file.is_object() ? workspaces_->imported(file.as_object()) : std::vector<std::string>();
            if (names.empty()) {
                logger()->get()->error("{} holds no workspace the editor can read", chosen.generic_string());
                return;
            }
            recordProject();
            // an imported workspace is shown as it was exported
            workspaces_->forget({ .project = projectKey(), .workspace = names.front() });
            showWorkspace(names.front());
            recordProject();
        });
}

/**
 **/
void Controller::look(std::size_t area, ViewPort* view, const std::string& profile) {
    if (!profiles_->has(profile)) {
        logger()->get()->error("There is no camera profile called {}", profile);
        return;
    }
    view->look(profiles_->get(profile));
    // the editor's state names the camera, so its tab and a saved layout show the new one
    layout_->set(layout_->areas()[area].editor().id, "camera", boost::json::string(profile));
    logger()->get()->info("{} shows the {} camera", view->name(), profile);
}

/**
 **/
void Controller::toggleProjection(ViewPort* view) {
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
    // the search takes its keys before the ui and the bindings, so Escape closes it rather than
    // quitting, while closing the window still quits
    if (search_ && search_->visible() && event.type == SDL_EVENT_KEY_DOWN) {
        if (event.key.key == SDLK_UP || event.key.key == SDLK_DOWN) {
            search_->step(event.key.key == SDLK_UP ? -1 : 1);
            return true;
        }
        if (event.key.key == SDLK_ESCAPE) {
            search_->close();
            return true;
        }
    }
    return uiKeys_ && uiKeys_->event(event);
}

/**
 **/
void Controller::layoutViews(int width, int height) {
    if (!pointer_) {
        return;
    }
    // the menu bar and the toolbars cover strips along the top and the left, the status bar
    // one along the foot, and the views divide what is left
    const glm::vec2 inset = renderer_ ? renderer_->insets() : glm::vec2(0.0F, 0.0F);
    const float foot = renderer_ ? renderer_->barHeight() : 0.0F;
    pointer_->resize(glm::vec4(inset.x, inset.y,
        std::max(0.0F, static_cast<float>(width) - inset.x),
        std::max(0.0F, static_cast<float>(height) - inset.y - foot)));
}

/**
 **/
bool Controller::render() {
    // the gaps and the reach follow the ui's scale, so a change of scale divides the window again
    const float scale = renderer_->scale();
    if (scale != metricsScale_) {
        metricsScale_ = scale;
        layout_->metrics({ .gap = std::max(1.0F, std::round(AreaLayout::Metrics::GAP * scale)),
            .reach = AreaLayout::Metrics::REACH * scale, .least = AreaLayout::Metrics::LEAST * scale,
            .strip = renderer_->barHeight() });
        layoutViews(window()->width(), window()->height());
    }
    if (search_) {
        search_->sync();
    }
    // the menus' labels are where the last frame laid them, which is where they are drawn again
    if (topBar_ && menu_ && menu_->count() > 0) {
        const v3d::type::geometry::Bound2D last = menu_->bound(menu_->count() - 1);
        const float end = last.position().x + last.size().x;
        topBar_->sync(topState(), glm::vec4(end, menu_->position().y, static_cast<float>(window()->width()) - end, renderer_->barHeight()));
    }
    // between frames, so a header is never built again while a press on it is being handled
    if (chrome_) {
        chrome_->sync(*views_, selectTool_->mask(), renderer_->barHeight());
    }
    renderer_->status(status());
    renderer_->draw();
    return true;
}

/**
 **/
bool Controller::release() {
    recordProject();
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
        pointer_->motion(event.position(), event.modifiers().ctrl);
        syncPointerShape();
    }
}

/**
 **/
void Controller::handleButton(const v3d::event::kind::MouseButton& event) {
    if (event.button() == "left") {
        clicks_ = event.clicks();
    } else if (event.button() == "right" && event.pressed() && pointer_ && pointer_->cancel()) {
        syncPointerShape();
    }
}

/**
 **/
void Controller::syncPointerShape() {
    const std::optional<AreaLayout::Divider> lit = pointer_ ? pointer_->lit() : std::nullopt;
    if (!lit) {
        window()->pointer(v3d::window::Window::Pointer::Arrow);
        return;
    }
    // a divider between stacked areas moves up and down
    window()->pointer(lit->across ? v3d::window::Window::Pointer::Rows : v3d::window::Window::Pointer::Columns);
}

};  // namespace v3d::editor
