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
#include <api/event/kind/WindowClose.h>
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
#include <vertical3d/src/control/chrome/Area.h>
#include <vertical3d/src/control/CommandSearch.h>
#include <vertical3d/src/control/IconSheet.h>
#include <vertical3d/src/control/chrome/Notices.h>
#include <vertical3d/src/control/Outliner.h>
#include <vertical3d/src/control/GraphEditor.h>
#include <vertical3d/src/control/Playback.h>
#include <vertical3d/src/control/chrome/Pointer.h>
#include <vertical3d/src/control/PropertiesEditor.h>
#include <vertical3d/src/control/TimelineEditor.h>
#include <vertical3d/src/control/RenderSettingsEditor.h>
#include <vertical3d/src/control/RenderView.h>
#include <vertical3d/src/control/chrome/StatusBar.h>
#include <vertical3d/src/control/chrome/TopBar.h>
#include <vertical3d/src/control/Workspaces.h>
#include <vertical3d/src/document/Document.h>
#include <vertical3d/src/render/Batch.h>
#include <vertical3d/src/render/MoyaDelegate.h>
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
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
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
        // the press goes to the pointer over the window the cursor is in
        const v3d::input::MouseState* held = mouse();
        const std::uint32_t in = held != nullptr ? held->window() : 0;
        const auto opened = std::ranges::find_if(opened_, [in](const Opened& each) { return each.window->id() == in; });
        const boost::shared_ptr<control::chrome::Pointer> pointer = opened != opened_.end() ? opened->pointer : context_.pointer;
        if (pointer) {
            pointer->button(event.state() == v3d::event::State::Pressed, clicks_);
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
    context_.keptWindows = [this]() { return keptWindows(); };
    context_.keyed = [this]() -> std::optional<app::Context::Placed> {
        const auto opened = std::ranges::find_if(opened_, [this](const Opened& each) { return each.window->id() == keyWindow_; });
        if (opened == opened_.end()) {
            return std::nullopt;
        }
        return app::Context::Placed{ .layout = opened->layout, .views = opened->views, .area = opened->pointer->activeArea() };
    };
    context_.reopen = [this](const boost::json::array& windows) { reopenWindows(windows); };
    if (!buildDocument() || !workspaces_.start(document(view::AreaLayout::DOCUMENT)) || !buildViews()) {
        return false;
    }
    buildTools();
    motion_ = dispatcher()->sink<v3d::event::kind::MouseMotion>().connect<&Controller::handleMotion>(*this);
    button_ = dispatcher()->sink<v3d::event::kind::MouseButton>().connect<&Controller::handleButton>(*this);
    resize_ = dispatcher()->sink<v3d::event::kind::WindowResize>().connect<&Controller::handleResize>(*this);
    close_ = dispatcher()->sink<v3d::event::kind::WindowClose>().connect<&Controller::handleClose>(*this);
    buildRenderer();
    if (!buildUi()) {
        return false;
    }
    renderer_->ui(shell_.ui());
    renderer_->pointer(context_.pointer);
    reportCommands();

    layoutViews(window()->width(), window()->height());
    // the windows the first workspace keeps open once there is a ui to give them
    const boost::json::array pending = std::exchange(pendingWindows_, boost::json::array());
    reopenWindows(pending);
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
    context_.views = boost::make_shared<view::AreaViews>(context_.layout, [this](const view::AreaLayout::Editor& editor) { return makeView(editor); });
    return context_.views->sync();
}

/**
 **/
boost::shared_ptr<view::ViewPort> Controller::makeView(const view::AreaLayout::Editor& editor) const {
    const std::optional<std::string> camera = v3d::asset::readString(editor.state, "camera");
    if (!camera || !context_.profiles->has(*camera)) {
        logger()->get()->error("A viewport in the layout names no camera profile that exists");
        return nullptr;
    }
    // the name is the view's pass name, so the editor's id keeps two views of a profile apart
    return boost::make_shared<view::ViewPort>(*camera + " " + std::to_string(editor.id), context_.profiles->get(*camera));
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
    outliner_.icons([this](const std::string& source) { return renderer_->image(source); });
    renderer_->views(context_.views);
    renderer_->scene(context_.document->scene());
    renderer_->transformTool(context_.transformTool);
    renderer_->selectTool(context_.selectTool);
    context_.notices = boost::make_shared<control::chrome::Notices>();
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
    chrome_ = boost::make_shared<control::chrome::Area>(vgui, uiContainer,
        [this](const boost::shared_ptr<v3d::ui::Component>& component) { renderer_->resolve(component); });
    uiKeys_ = boost::make_shared<v3d::ui::shell::Keyboard>(vgui, dispatcher(), window());
    uiKeys_->immediate(renderer_->immediate());
    properties_ = boost::make_shared<control::PropertiesEditor>(context_.document->history());
    context_.timeline = boost::make_shared<control::TimelineEditor>(context_.document, context_.playback,
        [this](const document::Outcome& outcome) { context_.report(outcome); });
    graph_ = boost::make_shared<control::GraphEditor>(context_.document, [this](const document::Outcome& outcome) { context_.report(outcome); });
    renderSettings_ = boost::make_shared<control::RenderSettingsEditor>(context_.document,
        [this](const document::Outcome& outcome) { context_.report(outcome); });
    renderer_->panels([this](v3d::ui::Immediate* layer, v3d::render::realtime::canvas::Canvas* canvas) { drawPanels(layer, canvas); });
    renderer_->opened([this](v3d::ui::Immediate* layer, v3d::render::realtime::canvas::Canvas* canvas, std::uint32_t window) {
        drawOpened(layer, canvas, window);
    });
    context_.window = [this](std::string_view kind) { return openWindow(kind); };
    context_.chooser = boost::make_shared<v3d::ui::shell::FileChooser>(vgui);
    context_.prompt = boost::make_shared<v3d::ui::shell::Prompt>(vgui);
    search_ = boost::make_shared<control::CommandSearch>(vgui);
    topBar_ = boost::make_shared<control::chrome::TopBar>(vgui, firstDialog, renderer_->type());
    // moya.exe is built beside the editor
    context_.renderView = boost::make_shared<control::RenderView>(boost::make_shared<render::MoyaDelegate>(logger()),
        boost::make_shared<render::Batch>(logger(), boost::filesystem::path(context_.path) / MOYA),
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
        boost::make_shared<v3d::ui::input::Cursor>(vgui, dispatcher(), renderer_->type());
    context_.pointer = boost::make_shared<control::chrome::Pointer>(cursor, context_.views, tools_, [this](const boost::shared_ptr<view::ViewPort>&) {
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
        for (const auto& [command, on] : control::chrome::Area::marks(*activeView)) {
            mark(command, on);
        }
    }

    for (const auto& [command, on] : control::chrome::Area::marks(context_.selectTool->mask())) {
        mark(command, on);
    }
    const tool::CameraControl::CameraMode latched = context_.cameraTool->latched();
    mark("camera::truck", latched == tool::CameraControl::CAMERA_MODE_TRUCK);
    mark("camera::dolly", latched == tool::CameraControl::CAMERA_MODE_DOLLY);
    mark("camera::zoom", latched == tool::CameraControl::CAMERA_MODE_ZOOM);
    mark("camera::pan", latched == tool::CameraControl::CAMERA_MODE_PAN);
    mark("camera::pitch", latched == tool::CameraControl::CAMERA_MODE_PITCH);

    mark("timeline::auto_key", context_.document->scene()->timeline().autoKey);

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
    const render::Batch::Status batch = context_.renderView ? context_.renderView->batchStatus() : render::Batch::Status();
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
    if (context_.pointer) {
        shapePointer(window().get(), *context_.pointer);
    }
    for (const Opened& opened : opened_) {
        shapePointer(opened.window.get(), *opened.pointer);
    }
}

/**
 **/
void Controller::areasChanged() {
    if (context_.pointer) {
        context_.pointer->place();
    }
    for (const Opened& opened : opened_) {
        opened.views->sync();
        opened.pointer->place();
    }
    syncPointerShape();
}

/**
 **/
void Controller::drawPanels(v3d::ui::Immediate* layer, v3d::render::realtime::canvas::Canvas* canvas) {
    drawAreas(layer, canvas, *context_.layout, context_.pointer.get(), window() ? window()->id() : 0);
    // an outliner keeps its folds while any window shows it
    std::vector<std::size_t> outliners;
    const auto held = [&outliners](const view::AreaLayout& layout) {
        for (const view::AreaLayout::Area& area : layout.areas()) {
            for (const view::AreaLayout::Editor& editor : area.editors) {
                if (editor.kind == control::Outliner::KIND) {
                    outliners.push_back(editor.id);
                }
            }
        }
    };
    held(*context_.layout);
    for (const Opened& opened : opened_) {
        held(*opened.layout);
    }
    outliner_.keep(outliners);
}

/**
 **/
void Controller::drawAreas(v3d::ui::Immediate* layer, v3d::render::realtime::canvas::Canvas* canvas, const view::AreaLayout& layout,
    control::chrome::Pointer* pointer, std::uint32_t window) {
    v3d::ui::Immediate::Input input;
    const v3d::input::MouseState* held = mouse();
    const bool here = held != nullptr && (held->window() == window || (held->window() == 0 && ownWindow(window)));
    if (here && pointer != nullptr && pointer->panels()) {
        input.cursor = pointer->cursor();
        input.down = held->held("left");
        input.pressed = held->pressed("left");
        input.released = held->released("left");
        input.wheel = held->wheel();
    }
    layer->begin(canvas, input);
    // a panel starts under its area's header
    const float header = renderer_->barHeight();
    // a graph moves its view by the camera move in force, as an orthographic view does
    control::GraphEditor::Navigate navigate = control::GraphEditor::Navigate::None;
    switch (context_.cameraTool->mode()) {
        case tool::CameraControl::CAMERA_MODE_TRUCK:
            navigate = control::GraphEditor::Navigate::Pan;
            break;
        case tool::CameraControl::CAMERA_MODE_ZOOM:
        case tool::CameraControl::CAMERA_MODE_DOLLY:
            navigate = control::GraphEditor::Navigate::Zoom;
            break;
        // turning the camera has no meaning in a graph, so a drag there moves a key
        case tool::CameraControl::CAMERA_MODE_PAN:
        case tool::CameraControl::CAMERA_MODE_PITCH:
        case tool::CameraControl::CAMERA_MODE_NONE:
            break;
    }
    control::RenderView::Asked asked = control::RenderView::Asked::Nothing;
    for (const view::AreaLayout::Area& area : layout.areas()) {
        const glm::vec4 region(area.content.x, area.content.y + header, area.content.z, area.content.w - header);
        const view::AreaLayout::Editor& editor = area.editor();
        if (region.z <= 0.0F || region.w <= 0.0F) {
            continue;
        }
        if (editor.kind == control::Outliner::KIND) {
            outliner_.draw(layer, context_.document->scene().get(), editor.id, region, context_.selectTool->adding());
        } else if (editor.kind == control::PropertiesEditor::KIND) {
            properties_->draw(layer, context_.document->scene(), editor.id, region);
        } else if (editor.kind == control::TimelineEditor::KIND && context_.timeline) {
            context_.timeline->draw(layer, editor.id, region);
        } else if (editor.kind == control::GraphEditor::KIND && graph_) {
            graph_->draw(layer, editor.id, region, navigate);
        } else if (editor.kind == control::RenderView::KIND && context_.renderView) {
            asked = std::max(asked, context_.renderView->draw(layer, editor.id, region));
        } else if (editor.kind == control::RenderSettingsEditor::KIND && renderSettings_) {
            renderSettings_->draw(layer, editor.id, region);
        }
    }
    layer->end();
    asked_ = std::max(asked_, asked);
    if (pointer != nullptr) {
        pointer->settle();
    }
}

/**
 **/
control::chrome::TopBar::State Controller::topState() const {
    control::chrome::TopBar::State state{ .workspaces = {}, .shown = workspaces_.shown(),
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
        const boost::shared_ptr<v3d::ui::Container> area = shell_.ui()->container(control::chrome::Area::name(context_.layout->areas()[active].id));
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
control::chrome::Status Controller::status() const {
    control::chrome::Status made;
    const boost::shared_ptr<view::ViewPort> view = context_.activeView();
    const bool perspective = view && view->camera() && !view->camera()->orthographic();
    made.buttons = control::chrome::StatusBar::buttons(*context_.cameraTool, *context_.transformTool, *context_.selectTool, perspective);
    made.selection = control::chrome::StatusBar::selection(*context_.document->scene(), context_.selectTool->mask());
    made.frame = control::chrome::StatusBar::frame(context_.document->scene()->timeline());
    made.rate = control::chrome::StatusBar::rate(statistics().mean());
    made.health = control::chrome::StatusBar::health({ .mean = statistics().mean(), .refresh = window()->refreshRate() });
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
    // a key or text goes to the ui of the window it was typed in
    std::uint32_t typed = 0;
    switch (event.type) {
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
            typed = event.key.windowID;
            break;
        case SDL_EVENT_TEXT_INPUT:
            typed = event.text.windowID;
            break;
        case SDL_EVENT_TEXT_EDITING:
            typed = event.edit.windowID;
            break;
        default:
            return uiKeys_ && uiKeys_->event(event);
    }
    keyWindow_ = typed;
    const auto opened = std::ranges::find_if(opened_, [typed](const Opened& each) { return each.window->id() == typed; });
    if (opened != opened_.end()) {
        return opened->keys && opened->keys->event(event);
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
        chrome_->sync(*context_.views, context_.selectTool->mask(), renderer_->barHeight(), context_.pointer ? context_.pointer->activeArea() : 0);
    }
    syncOpened();
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
    // each opened window's keyboard lets go of its screen's layer before the renderer releases
    // the screen
    for (Opened& opened : opened_) {
        opened.keys.reset();
    }
    // a render's thread and a batch's are waited for, before anything they report to goes
    if (context_.renderView) {
        context_.renderView->release();
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
    // a render view's buttons send what the Render menu does, after the frame that drew them
    const control::RenderView::Asked asked = std::exchange(asked_, control::RenderView::Asked::Nothing);
    if (asked != control::RenderView::Asked::Nothing) {
        commands().invoke(v3d::event::Event(asked == control::RenderView::Asked::Render ? "render::view" : "render::stop"));
    }
    if (context_.renderView && context_.activeView() && context_.activeView()->camera()) {
        context_.renderView->tick(*context_.document->scene(), context_.shot(), shows(control::RenderView::KIND));
    }
    if (context_.renderView) {
        syncCaption();
    }
    return Engine::tick(delta);
}

/**
 **/
bool Controller::shows(std::string_view kind) const {
    return std::ranges::any_of(context_.layout->areas(), [kind](const view::AreaLayout::Area& area) { return area.editor().kind == kind; }) ||
        std::ranges::any_of(opened_, [kind](const Opened& each) {
            return std::ranges::any_of(each.layout->areas(), [kind](const view::AreaLayout::Area& area) { return area.editor().kind == kind; });
        });
}

/**
 **/
bool Controller::openWindow(std::string_view kind) {
    for (const Opened& opened : opened_) {
        const std::vector<view::AreaLayout::Area>& areas = opened.layout->areas();
        for (std::size_t area = 0; area < areas.size(); area++) {
            const auto found = std::ranges::find(areas[area].editors, kind, &view::AreaLayout::Editor::kind);
            if (found != areas[area].editors.end()) {
                opened.layout->choose(area, static_cast<std::size_t>(found - areas[area].editors.begin()));
                return true;
            }
        }
    }
    const boost::json::object layout{ { "root", boost::json::object{ { "editors", boost::json::array{ boost::json::object{ { "kind", kind } } } } } } };
    return openWindow(layout, glm::ivec2(OPENED_WIDTH, OPENED_HEIGHT));
}

/**
 **/
bool Controller::openWindow(const boost::json::object& layout, const glm::ivec2& size) {
    Opened opened;
    opened.layout = boost::make_shared<view::AreaLayout>(logger(), nextOpened_);
    if (!opened.layout->load(layout)) {
        return false;
    }
    opened.window = addWindow(size);
    if (!opened.window) {
        return false;
    }
    nextOpened_ += OPENED_IDS;
    opened.layout->metrics(context_.layout->metrics());
    opened.views = boost::make_shared<view::AreaViews>(opened.layout, [this](const view::AreaLayout::Editor& editor) { return makeView(editor); });
    opened.views->sync();
    opened.ui = boost::make_shared<v3d::ui::Interface>(events(), dispatcher(), logger());
    opened.ui->shareThemes(*shell_.ui());
    opened.chrome = boost::make_shared<control::chrome::Area>(opened.ui, std::string(),
        [this](const boost::shared_ptr<v3d::ui::Component>& component) { renderer_->resolve(component); });
    const boost::shared_ptr<v3d::ui::input::Cursor> cursor = boost::make_shared<v3d::ui::input::Cursor>(opened.ui, dispatcher(), renderer_->type());
    // the show flags a check item draws are the active view's, in whichever window it is
    opened.pointer = boost::make_shared<control::chrome::Pointer>(cursor, opened.views, tools_, [this](const boost::shared_ptr<view::ViewPort>&) { syncUi(); });
    const boost::shared_ptr<control::chrome::Area> chrome = opened.chrome;
    opened.pointer->tabs([chrome](const glm::vec2& point) { return chrome->tabAt(point); });
    // a press on a ball of a view's gizmo turns its camera to look down that axis
    const boost::shared_ptr<view::AreaViews> views = opened.views;
    opened.pointer->controls([this, views](const glm::vec2& point) {
        const boost::shared_ptr<view::ViewPort> view = views->of(views->layout()->areaAt(point.x, point.y));
        const std::optional<view::AxisEnd> end = view ? gizmoAt(*view, renderer_->overlay(), point) : std::nullopt;
        if (!end) {
            return false;
        }
        lookAlong(view->camera().get(), *end);
        syncUi();
        return true;
    });
    if (!renderer_->open(opened.window, { .ui = opened.ui, .views = opened.views, .pointer = opened.pointer })) {
        removeWindow(opened.window);
        return false;
    }
    opened.window->caption("Vertical|3D");
    opened.keys = boost::make_shared<v3d::ui::shell::Keyboard>(opened.ui, dispatcher(), opened.window);
    if (v3d::ui::shell::Screen* screen = renderer_->screen(opened.window)) {
        opened.keys->immediate(screen->immediate());
    }
    opened.pointer->resize(glm::vec4(0.0F, 0.0F, static_cast<float>(size.x), static_cast<float>(size.y)));
    context_.others.push_back({ .layout = opened.layout, .views = opened.views });
    opened_.push_back(opened);
    return true;
}

/**
 **/
boost::json::array Controller::keptWindows() const {
    boost::json::array kept;
    for (const Opened& opened : opened_) {
        kept.emplace_back(boost::json::object{ { "width", opened.window->width() }, { "height", opened.window->height() },
            { "layout", opened.layout->save() } });
    }
    return kept;
}

/**
 **/
void Controller::reopenWindows(const boost::json::array& windows) {
    if (!renderer_ || !chrome_) {
        pendingWindows_ = windows;
        return;
    }
    pendingWindows_.clear();
    while (!opened_.empty()) {
        closeWindow(opened_.back().window->id());
    }
    for (const boost::json::value& entry : windows) {
        const boost::json::object* window = entry.if_object();
        const boost::json::object* layout = window != nullptr ? v3d::asset::readObject(*window, "layout") : nullptr;
        const std::optional<int> width = window != nullptr ? v3d::asset::readInt(*window, "width") : std::nullopt;
        const std::optional<int> height = window != nullptr ? v3d::asset::readInt(*window, "height") : std::nullopt;
        // a window too small to hold an area, or larger than any display, opens at its first size
        const auto side = [](const std::optional<int>& kept, int first) {
            return kept && *kept >= MIN_OPENED && *kept <= MAX_OPENED ? *kept : first;
        };
        if (layout == nullptr || !openWindow(*layout, glm::ivec2(side(width, OPENED_WIDTH), side(height, OPENED_HEIGHT)))) {
            logger()->get()->warn("A workspace's window could not be opened again");
        }
    }
}

/**
 **/
void Controller::closeWindow(std::uint32_t window) {
    const auto found = std::ranges::find_if(opened_, [window](const Opened& each) { return each.window->id() == window; });
    if (found == opened_.end()) {
        return;
    }
    // the keyboard lets go of the screen's layer before the screen goes
    found->keys.reset();
    const Opened closed = std::move(*found);
    opened_.erase(found);
    std::erase_if(context_.others, [&closed](const app::Context::Areas& each) { return each.layout == closed.layout; });
    renderer_->close(closed.window);
    removeWindow(closed.window);
}

/**
 **/
void Controller::syncOpened() {
    for (const Opened& opened : opened_) {
        const glm::vec4 region(0.0F, 0.0F, static_cast<float>(opened.window->width()), static_cast<float>(opened.window->height()));
        if (opened.layout->region() != region) {
            opened.pointer->resize(region);
        }
        opened.chrome->sync(*opened.views, context_.selectTool->mask(), renderer_->barHeight(), opened.pointer->activeArea());
    }
}

/**
 **/
void Controller::drawOpened(v3d::ui::Immediate* layer, v3d::render::realtime::canvas::Canvas* canvas, std::uint32_t window) {
    const auto found = std::ranges::find_if(opened_, [window](const Opened& each) { return each.window->id() == window; });
    if (found != opened_.end()) {
        drawAreas(layer, canvas, *found->layout, found->pointer.get(), window);
    }
}

/**
 **/
void Controller::shapePointer(v3d::window::Window* window, const control::chrome::Pointer& pointer) {
    const std::optional<view::AreaLayout::Divider> lit = pointer.lit();
    if (!lit) {
        window->pointer(v3d::window::Window::Pointer::Arrow);
        return;
    }
    // a divider between stacked areas moves up and down
    window->pointer(lit->across ? v3d::window::Window::Pointer::Rows : v3d::window::Window::Pointer::Columns);
}

/**
 **/
bool Controller::ownWindow(std::uint32_t window) const {
    return window == 0 || !this->window() || window == this->window()->id();
}

/**
 **/
void Controller::handleClose(const v3d::event::kind::WindowClose& event) {
    closeWindow(event.window());
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
    // an opened window reads its size each frame it is drawn
    if (!ownWindow(event.window())) {
        return;
    }
    layoutViews(event.width(), event.height());
}

/**
 **/
void Controller::handleMotion(const v3d::event::kind::MouseMotion& event) {
    const auto opened = std::ranges::find_if(opened_, [&event](const Opened& each) { return each.window->id() == event.window(); });
    if (opened != opened_.end()) {
        opened->pointer->motion(event.position(), event.modifiers().ctrl);
        syncPointerShape();
        return;
    }
    if (context_.pointer && ownWindow(event.window())) {
        context_.pointer->motion(event.position(), event.modifiers().ctrl);
        syncPointerShape();
    }
}

/**
 **/
void Controller::handleButton(const v3d::event::kind::MouseButton& event) {
    if (!ownWindow(event.window())) {
        return;
    }
    if (event.button() == "left") {
        clicks_ = event.clicks();
    } else if (event.button() == "right" && event.pressed() && context_.pointer && context_.pointer->cancel()) {
        syncPointerShape();
    }
}

};  // namespace v3d::editor
