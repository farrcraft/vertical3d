/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
**/

#include "Controller.h"

#include <cstddef>
#include <string>
#include <vector>

#include "CreatePoly.h"
#include "Renderer.h"

#include "../../api/config/Type.h"
#include "../../api/engine/Feature.h"
#include "../../api/render/realtime/Window.h"

#include <boost/make_shared.hpp>

namespace v3d::editor {

    namespace {

        /**
         * The context the camera bindings and the view commands arrive in. Named by
         * data/mappings.json, so the two have to be changed together.
         **/
        const char* const viewContext = "view";

        /**
         * The context the application level commands arrive in, which every app in the
         * repository shares.
         **/
        const char* const uiContext = "ui";

        /**
         * The context the Create menu's commands arrive in - gui.xml's create::poly::*,
         * which are bound to keys here because the editor has no menus yet.
         **/
        const char* const createContext = "create";

    };  // namespace

    /**
     **/
    Controller::Controller(const std::string& path) :
        v3d::engine::Engine(path),
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

        dispatcher_->sink<v3d::event::Event>().connect<&Controller::handleEvent>(*this);
        dispatcher_->sink<v3d::event::MouseMotion>().connect<&Controller::handleMotion>(*this);
        dispatcher_->sink<v3d::event::WindowResize>().connect<&Controller::handleResize>(*this);

        renderer_ = boost::make_shared<Renderer>(window(), logger_, assetManager_, &registry_);
        renderer_->views(views_);
        renderer_->scene(scene_);

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
    bool Controller::createPoly(const std::string& name) {
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
            return false;
        }

        // a new mesh is the selected one, which is what the transform tools will act on
        scene_->deselect();
        mesh->selected(true);
        scene_->add(mesh);
        logger_->get()->info("created a {} - {} meshes", name, scene_->count());
        return true;
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
        if (cameraTool_ && !cameraTool_->dragging()) {
            const std::size_t index = layout_->viewAt(cursor_.x, cursor_.y);
            if (index < views_.size()) {
                activeView_ = views_[index];
                cameraTool_->view(activeView_);
            }
        }

        if (cameraTool_) {
            cameraTool_->motion(cursor_);
        }
    }

    /**
     **/
    void Controller::handleEvent(const v3d::event::Event& event) {
        if (event.context()->name() == uiContext) {
            if (event.name() == "quit") {
                // not shutdown() - this is running inside the event loop, which would tick
                // and render one more frame against the window shutdown() had destroyed
                quit();
            }
            return;
        }

        if (event.context()->name() == createContext) {
            // a create is a press, so the release the same key also delivers is ignored
            if (event.state() != v3d::event::State::Released) {
                createPoly(std::string(event.name()));
            }
            return;
        }

        if (event.context()->name() != viewContext) {
            return;
        }

        const std::string name(event.name());

        if (name == "drag") {
            // the primary mouse button, whose number the input layer does not put on the
            // mapped event - the binding names which button it is
            cameraTool_->button(1, event.state() == v3d::event::State::Pressed, cursor_);
            return;
        }

        if (name == "toggleGrid") {
            if (activeView_) {
                activeView_->show(ViewPort::SHOW_GRID, !activeView_->shows(ViewPort::SHOW_GRID));
            }
            return;
        }

        if (name == "toggleMesh") {
            if (activeView_) {
                activeView_->show(ViewPort::SHOW_MESH, !activeView_->shows(ViewPort::SHOW_MESH));
            }
            return;
        }

        // the three camera modes are held rather than toggled: the modifier going down
        // selects the move a drag performs and it coming up puts the tool back to none
        if (name == "zoomCamera" || name == "truckCamera" || name == "panCamera") {
            if (event.state() == v3d::event::State::Released) {
                cameraTool_->deactivate(name);
            } else {
                cameraTool_->activate(name);
            }
        }
    }

};  // namespace v3d::editor
