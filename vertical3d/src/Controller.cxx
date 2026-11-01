/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
**/

#include "Controller.h"

#include <api/asset/Json.h>
#include <api/engine/CameraProfiles.h>
#include <api/engine/Engine.h>
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
#include <vertical3d/src/control/Playback.h>
#include <vertical3d/src/control/Pointer.h>
#include <vertical3d/src/control/PropertiesEditor.h>
#include <vertical3d/src/control/RenderWindow.h>
#include <vertical3d/src/control/StatusBar.h>
#include <vertical3d/src/control/TopBar.h>
#include <vertical3d/src/control/Workspaces.h>
#include <vertical3d/src/document/Document.h>
#include <vertical3d/src/render/Batch.h>
#include <vertical3d/src/render/MoyaDelegate.h>
#include <vertical3d/src/render/Panels.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/scene/Timeline.h>
#include <vertical3d/src/store/Project.h>
#include <vertical3d/src/tool/CameraControl.h>
#include <vertical3d/src/tool/Router.h>
#include <vertical3d/src/tool/Select.h>
#include <vertical3d/src/tool/Transform.h>
#include <vertical3d/src/view/AreaLayout.h>
#include <vertical3d/src/view/AreaViews.h>
#include <vertical3d/src/view/ViewOverlay.h>
#include <vertical3d/src/view/ViewPort.h>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keycode.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "Renderer.h"

#include <boost/filesystem/path.hpp>
#include <boost/make_shared.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor {

/**
 **/
Controller::Controller(const std::string& path) :
v3d::engine::Engine(path),
shell_(this, shellOptions()),
table_(&commands(), [this]() { commandRan(); }),
edits_(context_),
toolCommands_(context_),
viewCommands_(context_),
areaCommands_(context_),
workspaces_(&context_),
files_(context_, &workspaces_),
playbackCommands_(context_),
renderCommands_(context_) {
    context_.path = path;
    context_.areasChanged = [this]() { areasChanged(); };
    registerCommands();
}

/**
 **/
v3d::shell::Shell::Options Controller::shellOptions() {
    v3d::shell::Shell::Options options;
    options.menu = false;
    options.statistics = "view::show::statistics";
    // the editor keeps a theme of its own, not the games'
    options.theme.clear();
    return options;
}

/**
 **/
void Controller::registerCommands() {
    edits_.registerCommands(&table_);
    toolCommands_.registerCommands(&table_);
    viewCommands_.registerCommands(&table_);
    areaCommands_.registerCommands(&table_);
    workspaces_.registerCommands(&table_);
    files_.registerCommands(&table_);
    playbackCommands_.registerCommands(&table_);
    renderCommands_.registerCommands(&table_);

    table_.press("search::open", [this]() { openSearch(); });
    table_.press("search::run", [this]() { runSearch(); });
    table_.hold("view::drag", [this](const v3d::event::Event& event) {
        if (context_.pointer) {
            context_.pointer->button(event.state() == v3d::event::State::Pressed, clicks_);
            syncPointerShape();
        }
    });
}

/**
 **/
bool Controller::start() {
    if (!manifest()) {
        logger()->get()->error("The editor needs its config to know what to draw");
        return false;
    }
    context_.logger = logger();
    context_.settings = settings();
    if (!buildDocument() || !workspaces_.start(document(view::AreaLayout::DOCUMENT)) || !buildViews()) {
        return false;
    }
    buildTools();
    motion_ = dispatcher()->sink<v3d::event::kind::MouseMotion>().connect<&Controller::handleMotion>(*this);
    button_ = dispatcher()->sink<v3d::event::kind::MouseButton>().connect<&Controller::handleButton>(*this);
    resize_ = dispatcher()->sink<v3d::event::kind::WindowResize>().connect<&Controller::handleResize>(*this);
    buildRenderer();
    if (!buildUi()) {
        return false;
    }
    renderer_->ui(shell_.ui());
    renderer_->pointer(context_.pointer);
    reportCommands();

    layoutViews(window()->width(), window()->height());
    syncUi();
    syncCaption();
    logger()->get()->info("{} with {} areas", context_.layout->name(), context_.layout->areas().size());

    // the editor starts on the default project without reading it, so an autosave a crash left
    // there is the one way back to that session's work
    files_.offerRecovery();
    return true;
}

/**
 **/
bool Controller::buildDocument() {
    context_.document = boost::make_shared<document::Document>(logger(), boost::filesystem::path(context_.path) / "project");
    context_.playback = boost::make_shared<control::Playback>(context_.document);
    context_.profiles = boost::make_shared<v3d::engine::CameraProfiles>(logger());
    const boost::json::object* cameras = document(v3d::engine::CameraProfiles::DOCUMENT);
    if (cameras == nullptr) {
        logger()->get()->error("The editor has no camera config, so its views would have no cameras");
        return false;
    }
    return context_.profiles->load(*cameras);
}

/**
 **/
bool Controller::buildViews() {
    context_.views = boost::make_shared<view::AreaViews>(context_.layout, [this](const view::AreaLayout::Editor& editor) -> boost::shared_ptr<view::ViewPort> {
        const std::optional<std::string> camera = v3d::asset::readString(editor.state, "camera");
        if (!camera || !context_.profiles->has(*camera)) {
            logger()->get()->error("A viewport in the layout names no camera profile that exists");
            return nullptr;
        }
        // the name is the view's pass name, so the editor's id keeps two views of a profile apart
        return boost::make_shared<view::ViewPort>(*camera + " " + std::to_string(editor.id), context_.profiles->get(*camera));
    });
    return context_.views->sync();
}

/**
 **/
void Controller::buildTools() {
    const boost::shared_ptr<scene::Scene>& scene = context_.document->scene();
    context_.cameraTool = boost::make_shared<tool::CameraControl>();
    context_.selectTool = boost::make_shared<tool::Select>(scene, logger());
    context_.transformTool = boost::make_shared<tool::Transform>(scene, logger());
    context_.transformTool->commands(context_.document->history());
    tools_ = boost::make_shared<tool::Router>();
    tools_->add(context_.cameraTool);
    tools_->add(context_.transformTool);
    tools_->add(context_.selectTool);
}

/**
 **/
void Controller::buildRenderer() {
    renderer_ = boost::make_shared<Renderer>(window(), logger(), assets(), &shell_);
    control::IconSheet icons;
    const boost::json::object* sheet = document(control::IconSheet::DOCUMENT);
    if (sheet == nullptr || !icons.load(*sheet)) {
        logger()->get()->error("The editor has no icon sheet it can read, so its toolbar shows labels");
    }
    renderer_->icons(icons);
    renderer_->views(context_.views);
    renderer_->scene(context_.document->scene());
    renderer_->transformTool(context_.transformTool);
    renderer_->selectTool(context_.selectTool);
    context_.notices = boost::make_shared<control::Notices>();
    renderer_->notices(context_.notices);
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
    buildPointer(vgui);
    // the areas' headers go under the editor's own container, so its menus drop over them
    chrome_ = boost::make_shared<control::AreaChrome>(vgui, uiContainer,
        [this](const boost::shared_ptr<v3d::ui::Component>& component) { renderer_->resolve(component); });
    uiKeys_ = boost::make_shared<v3d::ui::shell::Keyboard>(vgui, dispatcher(), window());
    uiKeys_->immediate(renderer_->immediate());
    properties_ = boost::make_shared<control::PropertiesEditor>(context_.document->history());
    renderer_->panels([this](v3d::ui::Immediate* layer, v3d::render::realtime::canvas::Canvas* canvas) { drawPanels(layer, canvas); });
    context_.chooser = boost::make_shared<v3d::ui::shell::FileChooser>(vgui);
    context_.prompt = boost::make_shared<v3d::ui::shell::Prompt>(vgui);
    search_ = boost::make_shared<control::CommandSearch>(vgui);
    topBar_ = boost::make_shared<control::TopBar>(vgui, firstDialog, renderer_->measure());
    // moya.exe is built beside the editor
    context_.renderWindow = boost::make_shared<control::RenderWindow>(boost::make_shared<render::Panels>(vgui),
        boost::make_shared<render::MoyaDelegate>(logger()), boost::make_shared<render::Batch>(logger(), boost::filesystem::path(context_.path) / MOYA),
        [this](const boost::shared_ptr<v3d::image::Image>& image) { return renderer_->picture(image); });
    return findBars(vgui);
}

/**
 **/
void Controller::buildPointer(const boost::shared_ptr<v3d::ui::Interface>& vgui) {
    // the ui hit-tests the cursor before the app, in the order its own strips are drawn.
    // The text measure is the renderer's, so a press inside a text box places the caret
    // on the character drawn under the pointer
    const boost::shared_ptr<v3d::ui::input::Cursor> cursor =
        boost::make_shared<v3d::ui::input::Cursor>(vgui, dispatcher(), renderer_->measure());
    context_.pointer = boost::make_shared<control::Pointer>(cursor, context_.views, tools_, [this](const boost::shared_ptr<view::ViewPort>&) {
        // the show flags a check item draws are the active view's
        syncUi();
    });
    context_.pointer->tabs([this](const glm::vec2& point) { return chrome_ ? chrome_->tabAt(point) : std::nullopt; });
    // a press on a ball of a view's gizmo turns its camera to look down that axis
    context_.pointer->controls([this](const glm::vec2& point) {
        const boost::shared_ptr<view::ViewPort> view = context_.views->of(context_.layout->areaAt(point.x, point.y));
        const std::optional<view::AxisEnd> end = view ? gizmoAt(*view, renderer_->overlay(), point) : std::nullopt;
        if (!end) {
            return false;
        }
        lookAlong(view->camera().get(), *end);
        syncUi();
        return true;
    });
}

/**
 **/
bool Controller::findBars(const boost::shared_ptr<v3d::ui::Interface>& vgui) {
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
void Controller::reportCommands() const {
    // the log does not exist while the constructor registers, so a name refused then is
    // reported here
    for (const std::string& name : table_.refused()) {
        logger()->get()->error("{} is registered twice", name);
    }
    std::size_t named = 0;
    for (const std::string& name : commands().names()) {
        if (menu_ && menu_->find(name)) {
            named++;
        }
    }
    logger()->get()->info("{} commands registered, {} of them on a menu", commands().size(), named);
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
    if (const boost::shared_ptr<view::ViewPort> activeView = context_.activeView()) {
        for (const auto& [command, on] : control::AreaChrome::marks(*activeView)) {
            mark(command, on);
        }
    }

    for (const auto& [command, on] : control::AreaChrome::marks(context_.selectTool->mask())) {
        mark(command, on);
    }
    const tool::CameraControl::CameraMode latched = context_.cameraTool->latched();
    mark("camera::truck", latched == tool::CameraControl::CAMERA_MODE_TRUCK);
    mark("camera::dolly", latched == tool::CameraControl::CAMERA_MODE_DOLLY);
    mark("camera::zoom", latched == tool::CameraControl::CAMERA_MODE_ZOOM);
    mark("camera::pan", latched == tool::CameraControl::CAMERA_MODE_PAN);
    mark("camera::pitch", latched == tool::CameraControl::CAMERA_MODE_PITCH);

    const tool::Transform::Mode mode = context_.transformTool->mode();
    mark("transform::select", mode == tool::Transform::Mode::None);
    mark("transform::translate", mode == tool::Transform::Mode::Translate);
    mark("transform::rotate", mode == tool::Transform::Mode::Rotate);
    mark("transform::scale", mode == tool::Transform::Mode::Scale);
}

/**
 **/
void Controller::commandRan() {
    syncUi();
    syncCaption();
}

/**
 **/
void Controller::syncCaption() {
    const boost::shared_ptr<document::Document>& document = context_.document;
    if (!document) {
        return;
    }
    std::string caption = "Vertical|3D - " + document->project()->directory().filename().string() +
        (document->dirty() ? " *" : "") + " - frame " + std::to_string(document->scene()->timeline().frame);
    const render::Batch::Status batch = context_.renderWindow ? context_.renderWindow->batchStatus() : render::Batch::Status();
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
void Controller::syncPointerShape() {
    const std::optional<view::AreaLayout::Divider> lit = context_.pointer ? context_.pointer->lit() : std::nullopt;
    if (!lit) {
        window()->pointer(v3d::window::Window::Pointer::Arrow);
        return;
    }
    // a divider between stacked areas moves up and down
    window()->pointer(lit->across ? v3d::window::Window::Pointer::Rows : v3d::window::Window::Pointer::Columns);
}

/**
 **/
void Controller::areasChanged() {
    if (context_.pointer) {
        context_.pointer->place();
    }
    syncPointerShape();
}

/**
 **/
void Controller::drawPanels(v3d::ui::Immediate* layer, v3d::render::realtime::canvas::Canvas* canvas) {
    const boost::shared_ptr<control::Pointer>& pointer = context_.pointer;
    v3d::ui::Immediate::Input input;
    const v3d::input::MouseState* held = mouse();
    if (held != nullptr && pointer && pointer->panels()) {
        input.cursor = pointer->cursor();
        input.down = held->held("left");
        input.pressed = held->pressed("left");
        input.released = held->released("left");
        input.wheel = held->wheel();
    }
    layer->begin(canvas, input);
    // a panel starts under its area's header
    const float header = renderer_->barHeight();
    std::vector<std::size_t> outliners;
    for (const view::AreaLayout::Area& area : context_.layout->areas()) {
        const glm::vec4 region(area.content.x, area.content.y + header, area.content.z, area.content.w - header);
        const view::AreaLayout::Editor& editor = area.editor();
        if (region.z <= 0.0F || region.w <= 0.0F) {
            continue;
        }
        if (editor.kind == control::Outliner::KIND) {
            outliner_.draw(layer, context_.document->scene().get(), editor.id, region, context_.selectTool->adding());
            outliners.push_back(editor.id);
        } else if (editor.kind == control::PropertiesEditor::KIND) {
            properties_->draw(layer, context_.document->scene().get(), editor.id, region);
        }
    }
    layer->end();
    outliner_.keep(outliners);
    if (pointer) {
        pointer->settle();
    }
}

/**
 **/
control::TopBar::State Controller::topState() const {
    control::TopBar::State state{ .workspaces = {}, .shown = workspaces_.shown(),
        .project = context_.document->project()->directory().filename().string(), .dirty = context_.document->dirty() };
    for (const boost::json::object& workspace : context_.workspaces->all()) {
        state.workspaces.emplace_back(v3d::asset::readString(workspace, "name").value_or(std::string()));
    }
    return state;
}

/**
 **/
void Controller::openSearch() {
    std::vector<boost::shared_ptr<v3d::ui::component::menu::Bar>> bars{ menu_ };
    // an area's commands are on its header, and run from the search they act on the active area
    const std::size_t active = context_.areaOf(v3d::event::Event());
    if (active < context_.layout->areas().size()) {
        const boost::shared_ptr<v3d::ui::Container> area = shell_.ui()->container(control::AreaChrome::name(context_.layout->areas()[active].id));
        if (area) {
            bars.push_back(boost::dynamic_pointer_cast<v3d::ui::component::menu::Bar>(area->get("header")));
        }
    }
    std::vector<control::CommandSearch::Entry> entries = control::CommandSearch::catalogue(bars);
    for (control::CommandSearch::Entry& entry : entries) {
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
        context_.refuse("nothing runs " + *command);
    }
}

/**
 **/
control::Status Controller::status() const {
    control::Status made;
    const boost::shared_ptr<view::ViewPort> view = context_.activeView();
    const bool perspective = view && view->camera() && !view->camera()->orthographic();
    made.buttons = control::StatusBar::buttons(*context_.cameraTool, *context_.transformTool, *context_.selectTool, perspective);
    made.selection = control::StatusBar::selection(*context_.document->scene(), context_.selectTool->mask());
    made.frame = "Frame " + std::to_string(context_.document->scene()->timeline().frame);
    made.rate = control::StatusBar::rate(statistics().mean());
    return made;
}

/**
 **/
bool Controller::quitRequested() {
    // Escape while a divider is held puts the divider back rather than quitting
    if (context_.pointer && context_.pointer->cancel()) {
        syncPointerShape();
        return false;
    }
    if (!context_.document || !context_.document->dirty()) {
        return true;
    }
    // a second close while the question is up would replace it, and lose what the first one
    // was asking for
    if (context_.prompt && context_.prompt->visible()) {
        return false;
    }
    files_.askBeforeDiscarding([this]() { quit(); });
    return false;
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
    if (!context_.pointer) {
        return;
    }
    // the menu bar and the toolbars cover strips along the top and the left, the status bar
    // one along the foot, and the views divide what is left
    const glm::vec2 inset = renderer_ ? renderer_->insets() : glm::vec2(0.0F, 0.0F);
    const float foot = renderer_ ? renderer_->barHeight() : 0.0F;
    context_.pointer->resize(glm::vec4(inset.x, inset.y,
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
        context_.layout->metrics({ .gap = std::max(1.0F, std::round(view::AreaLayout::Metrics::GAP * scale)),
            .reach = view::AreaLayout::Metrics::REACH * scale, .least = view::AreaLayout::Metrics::LEAST * scale,
            .strip = renderer_->barHeight() });
        layoutViews(window()->width(), window()->height());
    }
    if (search_) {
        search_->sync();
    }
    // the menus' labels are drawn again where the last frame laid them
    if (topBar_ && menu_ && menu_->count() > 0) {
        const v3d::type::geometry::Bound2D last = menu_->bound(menu_->count() - 1);
        const float end = last.position().x + last.size().x;
        topBar_->sync(topState(), glm::vec4(end, menu_->position().y, static_cast<float>(window()->width()) - end, renderer_->barHeight()));
    }
    // between frames, so a header is never built again while a press on it is being handled
    if (chrome_) {
        chrome_->sync(*context_.views, context_.selectTool->mask(), renderer_->barHeight());
    }
    renderer_->status(status());
    renderer_->draw();
    return true;
}

/**
 **/
bool Controller::release() {
    workspaces_.record();
    if (context_.document) {
        context_.document->close();
    }
    // a render's thread and a batch's are waited for, before anything they report to goes
    if (context_.renderWindow) {
        context_.renderWindow->release();
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
    if (context_.document) {
        context_.document->tick(delta);
    }
    if (context_.renderWindow && context_.renderWindow->open() && context_.activeView() && context_.activeView()->camera()) {
        context_.renderWindow->tick(*context_.document->scene(), context_.shot());
    }
    if (context_.renderWindow) {
        syncCaption();
    }
    return Engine::tick(delta);
}

/**
 **/
bool Controller::simulate(float step) {
    if (context_.playback) {
        const bool playing = context_.playback->playing();
        context_.playback->simulate(step);
        if (playing) {
            syncCaption();
        }
    }
    return Engine::simulate(step);
}

/**
 **/
void Controller::handleResize(const v3d::event::kind::WindowResize& event) {
    layoutViews(event.width(), event.height());
}

/**
 **/
void Controller::handleMotion(const v3d::event::kind::MouseMotion& event) {
    if (context_.pointer) {
        context_.pointer->motion(event.position(), event.modifiers().ctrl);
        syncPointerShape();
    }
}

/**
 **/
void Controller::handleButton(const v3d::event::kind::MouseButton& event) {
    if (event.button() == "left") {
        clicks_ = event.clicks();
    } else if (event.button() == "right" && event.pressed() && context_.pointer && context_.pointer->cancel()) {
        syncPointerShape();
    }
}

};  // namespace v3d::editor
