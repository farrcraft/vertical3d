#include "Controller.h"

#include "../command/BindLoader.h"
#include "../hookah/Hookah.h"
#include "../core/CreatePolyCommandSet.h"
#include "CameraControlTool.h"

#include <boost/property_tree/xml_parser.hpp>
#include <iostream>


Controller::Controller()
{
	// create new app window and set caption
	window_ = Hookah::Create3DWindow(800, 600);

	// create input devices 
	keyboard_ = boost::dynamic_pointer_cast<v3D::KeyboardDevice, v3D::InputDevice>(Hookah::CreateInputDevice("keyboard"));
	mouse_ = boost::dynamic_pointer_cast<v3D::MouseDevice, v3D::InputDevice>(Hookah::CreateInputDevice("mouse"));

	// register directory as an observer of input device events
	listenerAdapter_.reset(new v3D::InputEventAdapter(keyboard_, mouse_));
	listenerAdapter_->connect(&directory_);

	// add input devices to window
	window_->addInputDevice("keyboard", keyboard_);
	window_->addInputDevice("mouse", mouse_);

	// set window caption
	window_->caption("Vertical|3D");

	// load config file into a property tree
	boost::property_tree::ptree ptree;
	boost::property_tree::read_xml("config.xml", ptree);

	// setup scene
	scene_.reset(new v3D::Scene());

	// read camera profile info from config file - we need to have this info before we create the viewport
	load_camera_profiles(ptree);

	// create viewport
	view_.reset(new v3D::ViewPort(window_, scene_));

	// set the viewport size according to the window
	view_->resize(window_->width(), window_->height());

	// load key binds from the property tree
	v3D::utility::load_binds(ptree, &directory_);

	// register event listeners
	window_->addDrawListener(boost::bind(&v3D::ViewPort::draw, boost::ref(view_), _1));
	window_->addResizeListener(boost::bind(&v3D::ViewPort::resize, boost::ref(view_), _1, _2));
//	_window->addTickListener(boost::bind(&Scene::tick, boost::ref(_scene), _1));

	// register core application commands
	directory_.add("poly_cube", "create", boost::bind(&Controller::exec, boost::ref(*this), _1, _2));
	directory_.add("poly_plane", "create", boost::bind(&Controller::exec, boost::ref(*this), _1, _2));
	directory_.add("poly_cylinder", "create", boost::bind(&Controller::exec, boost::ref(*this), _1, _2));
	directory_.add("poly_cone", "create", boost::bind(&Controller::exec, boost::ref(*this), _1, _2));
	directory_.add("activate_tool", "tool", boost::bind(&Controller::exec, boost::ref(*this), _1, _2));

	// register tools
	boost::shared_ptr<v3D::Tool> tool(new CameraControlTool(mouse_, view_));
	tools_["pan_camera_tool"] = tool;
	tools_["zoom_camera_tool"] = tool;
	tools_["truck_camera_tool"] = tool;

	// register v3dgui application commands

	// initialize vgui

	// register component manager as an observer of input device events
	keyboard_->addEventListener(&cm_, "luxa::cm");
	mouse_->addEventListener(&cm_, "luxa::cm");

	cm_->registerComponentFactory(new ViewPortFactory());

	// register ui event listeners
	window_->addDrawListener(boost::bind(&Luxa::ComponentManager::draw, boost::ref(cm_), _1));
	window_->addResizeListener(boost::bind(&Luxa::ComponentManager::resize, boost::ref(cm_), _1, _2));
	window_->addTickListener(boost::bind(&Luxa::ComponentManager::tick, boost::ref(cm_), _1));

	// set default managed area to match canvas size
	cm_.resize(window_->width(), window_->height());

	// set a black colored overlay
	v3D::Color3 overlay_color(0.0f, 0.0f, 0.0f);
	Luxa::Overlay overlay(overlay_color);
	cm_.renderer()->overlay(overlay);

	// load UI from the root config property tree
	Luxa::UILoader ui_loader;
	ui_loader.load(ptree, &cm_);

}

Controller::~Controller()
{
}

bool Controller::run()
{
	return window_->run(Hookah::Window::EVENT_HANDLING_BLOCKING, Hookah::Window::DRAW_DIRTY);
}

bool Controller::exec(const v3D::CommandInfo & command, const std::string & param)
{
	if (command.scope() == "create")
	{
		if (command.name() == "poly_cube")
		{
			scene_->addBRep(v3D::create_poly_cube());
			window_->invalidate();
			return true;
		}
		if (command.name() == "poly_plane")
		{
			scene_->addBRep(v3D::create_poly_plane());
			window_->invalidate();
			return true;
		}
		if (command.name() == "poly_cylinder")
		{
			scene_->addBRep(v3D::create_poly_cylinder());
			window_->invalidate();
			return true;
		}
		if (command.name() == "poly_cone")
		{
			scene_->addBRep(v3D::create_poly_cone());
			window_->invalidate();
			return true;
		}
	}

	if (command.scope() == "tool")
	{
		if (command.name() == "activate_tool")
		{
			activate_tool(param);
			return true;
		}
	}

	return false;
}

void Controller::activate_tool(const std::string & name)
{
	std::map<std::string, boost::shared_ptr<v3D::Tool> >::iterator iter = tools_.find(name);
	if (iter != tools_.end())
	{
		if (activeTool_)
			activeTool_->deactivate(name);
		activeTool_ = iter->second;
		activeTool_->activate(name);
	}
}

void Controller::load_camera_profiles(const boost::property_tree::ptree & tree) 
{
	/*
	  <cameraprofiles>
		<camera name="Front" near=".001" far="100.0" adaptive="both"
			orthographic="true" zoom="10.0" aspect="1.33"
	        eye="0.0, 0.0, -10.0" up="0.0, 1.0, 0.0"
		    right="1.0, 0.0, 0.0" direction="0.0, 0.0, 1.0" />
	*/

	const boost::property_tree::ptree::const_assoc_iterator & cameras = tree.find("config.cameraprofiles");

	std::string param;
	boost::property_tree::ptree::const_iterator iter = cameras.begin();
	for ( ; iter != cameras.end(); iter++)
	{
		param = iter->get("<xmlattr>.name");
		v3D::CameraProfile profile(name, eye, up, right, direction);
		float near, far;
		profile.clipping(near, far);
		scene_->addCameraProfile(profile);
	}
}