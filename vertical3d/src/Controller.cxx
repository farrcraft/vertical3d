/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
**/

#include "Controller.h"

#include <cstddef>
#include <string>
#include <vector>

#include "CreateCommand.h"
#include "CreatePoly.h"
#include "Renderer.h"

#include "../../api/config/Type.h"
#include "../../api/engine/Feature.h"
#include "../../api/render/realtime/Window.h"

#include <boost/make_shared.hpp>

namespace v3d::editor {

    /**
     **/
    Controller::Controller(const std::string& path) :
        v3d::engine::Engine(path),
        path_(path),
        cursor_(0.0f, 0.0f) {
    }

    /**
     **/
    bool Controller::initialize() {
        if (!v3d::engine::Engine::initialize(static_cast<int>(
            v3d::engine::Feature::Config |
            v3d::engine::Feature::Window |
            v3d::engine::Feature::MouseInput |
            v3d::engine::Feature::KeyboardInput))) {
            return false;
        }

        window_->caption("Vertical|3D");

        if (!config_) {
            logger_->get()->error("The editor needs its config to know what to draw");
            return false;
        }

        scene_ = boost::make_shared<Scene>();
        project_ = boost::make_shared<Project>(logger_);
        commands_ = boost::make_shared<CommandStack>();

        profiles_ = boost::make_shared<CameraProfiles>(logger_);
        if (!profiles_->load(config_->get(v3d::config::Type::Camera))) {
            return false;
        }

        layout_ = boost::make_shared<ViewLayout>(logger_);
        if (!layout_->load(config_->get(v3d::config::Type::Layout))) {
            return false;
        }

        if (!buildViews()) {
            return false;
        }

        cameraTool_ = boost::make_shared<CameraControlTool>();
        selectTool_ = boost::make_shared<SelectTool>(scene_, logger_);
        transformTool_ = boost::make_shared<TransformTool>(scene_, logger_);
        transformTool_->commands(commands_);

        dispatcher_->sink<v3d::event::Event>().connect<&Controller::handleEvent>(*this);
        dispatcher_->sink<v3d::event::MouseMotion>().connect<&Controller::handleMotion>(*this);
        dispatcher_->sink<v3d::event::WindowResize>().connect<&Controller::handleResize>(*this);

        renderer_ = boost::make_shared<Renderer>(window(), logger_, assetManager_, &registry_);
        renderer_->views(views_);
        renderer_->scene(scene_);
        renderer_->manipulator(transformTool_->manipulator());

        // after the tools and the renderer, because every handler closes over one of them
        registerCommands();

        layoutViews(window_->width(), window_->height());

        logger_->get()->info("{} with {} views", layout_->name(), views_.size());
        return true;
    }

    /**
     **/
    bool Controller::buildViews() {
        views_.clear();
        for (const ViewLayout::View& view : layout_->views()) {
            if (!profiles_->has(view.camera)) {
                logger_->get()->error("The layout names a camera profile that does not exist: {}", view.camera);
                return false;
            }
            views_.push_back(boost::make_shared<ViewPort>(view.camera, profiles_->get(view.camera)));
        }
        if (!views_.empty()) {
            activeView_ = views_.front();
        }
        return true;
    }

    /**
     **/
    void Controller::registerCommands() {
        // a refused registration means the name is already taken, which is one of the two
        // handlers never running - so it is said out loud rather than returned to nobody
        auto press = [this](const std::string& name, const CommandDirectory::PressHandler& handler) {
            if (!directory_.addPress(name, handler)) {
                logger_->get()->error("{} is registered twice", name);
            }
        };
        auto hold = [this](const std::string& name, const CommandDirectory::Handler& handler) {
            if (!directory_.add(name, handler)) {
                logger_->get()->error("{} is registered twice", name);
            }
        };

        // the names are gui.xml's, because a menu translated from it names the command it
        // invokes and the two have to meet somewhere
        press("create::poly::cube", [this]() { createPoly("cube"); });
        press("create::poly::plane", [this]() { createPoly("plane"); });
        press("create::poly::cylinder", [this]() { createPoly("cylinder"); });
        press("create::poly::cone", [this]() { createPoly("cone"); });

        press("select::mask::object", [this]() { selectTool_->activate("object"); });
        press("select::mask::vertex", [this]() { selectTool_->activate("vertex"); });
        press("select::mask::edge", [this]() { selectTool_->activate("edge"); });
        press("select::mask::face", [this]() { selectTool_->activate("face"); });

        press("transform::select", [this]() { transformMode("select"); });
        press("transform::translate", [this]() { transformMode("translate"); });
        press("transform::rotate", [this]() { transformMode("rotate"); });
        press("transform::scale", [this]() { transformMode("scale"); });

        // camera and light have flags on the view and nothing that draws them, so a command
        // for either would be a menu item that appears to work
        press("view::show::grid", [this]() { toggleShow(ViewPort::SHOW_GRID); });
        press("view::show::mesh", [this]() { toggleShow(ViewPort::SHOW_MESH); });
        press("view::show::handle", [this]() { toggleShow(ViewPort::SHOW_HANDLE); });

        // the three camera moves are held rather than latched: the modifier going down
        // chooses what a drag performs and it coming up puts the tool back to none
        hold("view::camera::zoom", [this](const v3d::event::Event& event) {
            cameraMode("zoom", event.state() != v3d::event::State::Released);
        });
        hold("view::camera::truck", [this](const v3d::event::Event& event) {
            cameraMode("truck", event.state() != v3d::event::State::Released);
        });
        hold("view::camera::pan", [this](const v3d::event::Event& event) {
            cameraMode("pan", event.state() != v3d::event::State::Released);
        });

        hold("view::drag", [this](const v3d::event::Event& event) {
            drag(event.state() == v3d::event::State::Pressed);
        });

        press("project::load", [this]() { openProject(); });
        press("project::save", [this]() { saveProject(); });

        // gui.xml has neither, so there is no menu name to match
        press("edit::undo", [this]() { history("undo"); });
        press("edit::redo", [this]() { history("redo"); });

        // gui.xml names this one without a context; ui is the context every app in the
        // repository puts its application level commands in
        press("ui::quit", [this]() {
            // not shutdown() - this is running inside the event loop, which would tick and
            // render one more frame against the window shutdown() had destroyed
            quit();
        });

        logger_->get()->info("{} commands registered", directory_.size());
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

        // the command is what does the creating, so that making a mesh and redoing one are
        // the same code rather than two that have to agree
        boost::shared_ptr<CreateCommand> command = boost::make_shared<CreateCommand>(scene_, mesh, name);
        command->redo();
        commands_->push(command);
        logger_->get()->info("created a {} - {} meshes", name, scene_->count());
    }

    /**
     **/
    void Controller::history(const std::string& name) {
        boost::shared_ptr<Command> command;
        if (name == "undo") {
            command = commands_->undo();
        } else if (name == "redo") {
            command = commands_->redo();
        } else {
            return;
        }
        if (!command) {
            logger_->get()->info("nothing to {}", name);
            return;
        }
        logger_->get()->info("{} {}", name, command->name());
    }

    /**
     **/
    std::string Controller::projectPath() const {
        return path_ + "project.json";
    }

    /**
     **/
    void Controller::openProject() {
        // a gesture under way is holding the mesh it started on, which the read is about to
        // take out of the scene
        transformTool_->cancel();
        if (!project_->read(projectPath(), scene_)) {
            return;
        }
        // the history describes a scene that no longer exists, and nothing in it could be
        // undone against the one that replaced it
        commands_->clear();
    }

    /**
     **/
    void Controller::saveProject() {
        project_->write(projectPath(), scene_);
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
        renderer_->manipulator(transformTool_->manipulator());
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
    void Controller::drag(bool pressed) {
        // the primary mouse button, whose number the input layer does not put on the mapped
        // event - the binding names which button it is
        cameraTool_->button(1, pressed, cursor_);
        // one button, three tools. A modifier held means the drag is driving a camera;
        // otherwise a handle of the selection takes the press if the cursor is on one, and a
        // press no handle took is what picks
        if (cameraTool_->mode() != CameraControlTool::CAMERA_MODE_NONE) {
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
        layout_->resize(width, height);
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
    bool Controller::shutdown() {
        if (renderer_) {
            // the device has to be idle before the window it presents to is destroyed
            renderer_->shutdown();
        }
        return v3d::engine::Engine::shutdown();
    }

    /**
     **/
    void Controller::handleResize(const v3d::event::WindowResize& event) {
        layoutViews(event.width(), event.height());
    }

    /**
     **/
    void Controller::handleMotion(const v3d::event::MouseMotion& event) {
        cursor_ = event.position();

        // the view under the cursor is the one a drag would drive - but not while one is
        // under way, or a gesture that wandered over a border would change camera mid drag
        if (cameraTool_ && !cameraTool_->dragging() && transformTool_ && !transformTool_->dragging()) {
            const std::size_t index = layout_->viewAt(cursor_.x, cursor_.y);
            if (index < views_.size()) {
                activeView_ = views_[index];
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

    /**
     **/
    void Controller::handleEvent(const v3d::event::Event& event) {
        // the dispatcher carries both halves of a mapping. A source event is the keypress
        // itself, which event::Engine is what listens for; only what a binding or a menu
        // item produced is a command
        if (event.type() != v3d::event::Type::Destination) {
            return;
        }
        if (!directory_.invoke(event)) {
            logger_->get()->warn("no command is registered as {}", event.str());
        }
    }

};  // namespace v3d::editor
