/*
	Copyright (C) 2001-2004 by Josh Farr
	merkaba_at_quantumfish_dot_com

	This file is part of Vertical|3D.

	Vertical|3D is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Vertical|3D is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Vertical|3D; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <iostream>

#ifdef USE_MOYA
#include <libmoya/Moya.h>
//#include <libmoya/Vertex.h>
#endif // USE_MOYA

#include <libv3dcore/Project.h>
#include <libv3dcommand/CommandDirectory.h>

#include "Window.h"
#include "RenderView.h"

#include "manipulators/TranslateManipulator.h"
#include "manipulators/RotateManipulator.h"
#include "manipulators/ScaleManipulator.h"

#define DEFAULT_TITLE  "Vertical|3D (Rigel)"

using namespace v3D;

extern slug::logstream _log;

Window::Window() : 
			_vbox(false, 0), _frame(""), _button("Quit"),
			_subWindow(0), _menu(0), _activeToolMode(NULL_TOOL_MODE),
			_initialized(false), _selectMasks(SELECT_MASK_ANY),
			_activeManipulator(0)
{
}

Window::~Window()
{
	if (_subWindow)
		delete _subWindow;
	if (_activeManipulator)
		delete _activeManipulator;
}

Window * Window::instance(void)
{
	static Window * instance = 0;
	if (!instance)
		instance = new Window();
	return instance;
}

bool Window::initialize(void)
{
	if (_initialized)
	{
		_log.level(slug::log_base::ll_warning) << "can't re-initialize - window already initialized...\n";
		_log.flush();
		return true;
	}

	set_title(DEFAULT_TITLE);
	// Get automatically redrawn if any of their children changed allocation.
	set_reallocate_redraws(true);
	// add main vbox to window
	add(_vbox);

	// add top toolbar
	_vbox.pack_start(_topBar, Gtk::PACK_SHRINK);
	// add main hbox below top toolbar
	_vbox.pack_start(_hbox);
	// set the left toolbar's orientation
	_leftBar.set_orientation(Gtk::ORIENTATION_VERTICAL);
	// add left toolbar in main hbox
	_hbox.pack_start(_leftBar, Gtk::PACK_SHRINK);

	_log.level(slug::log_base::ll_info) << "building gui...\n";
	_log.flush();

	// build menu and viewlayout from gui.xml
	build_gui();

	_viewport = _layout._views[0];
	_viewport->_active = true;

	_frame.set_size_request(200, -1);
	_pane.add2(_frame);
	_hbox.pack_start(_pane);

	// Simple quit button.
	_button.signal_clicked().connect(sigc::ptr_fun(&Gtk::Main::quit));
	_vbox.pack_start(_button, Gtk::PACK_SHRINK, 0);

	_log.level(slug::log_base::ll_info) << "showing window...\n";
	_log.flush();

	// show window
	show_all_children();

	_initialized = true;

	return true;
}

unsigned int Window::selectMasks(void) const
{
	return _selectMasks;
}

void Window::selectMasks(unsigned int mask)
{
	_selectMasks = mask;
}


std::vector<CameraProfile> & Window::profiles(void)
{
	return _profiles;
}

Window::ToolModes Window::activeToolMode(void) const
{
	return _activeToolMode;
}

void Window::activeToolMode(ToolModes mode)
{
	if (mode == _activeToolMode)
		return;

	// set active manipulator
	if (_activeManipulator)
		delete _activeManipulator;

	if (mode == TRANSLATE_TOOL_MODE)
		_activeManipulator = new TranslateManipulator();
	else if (mode == ROTATE_TOOL_MODE)
		_activeManipulator = new RotateManipulator();
	else if (mode == SCALE_TOOL_MODE)
		_activeManipulator = new ScaleManipulator();
	else
		_activeManipulator = 0;

	_activeToolMode = mode;
}

Gtk::Menu * Window::menu(void)
{
	return _menu;
}

ViewLayout * Window::layout(void)
{
	return &_layout;
}

bool Window::build_gui(void)
{
	std::string fileName = std::string(VERTICAL3D_SHARED_PATH) + std::string("gui.xml");

	xmlpp::DomParser parser(fileName, false); // false = don't validate

	xmlpp::Document * doc = parser.get_document();
	xmlpp::Element * root = doc->get_root_node();

	// get root menu element
	xmlpp::Node::NodeList root_menu_elem = root->get_children("menu");
	xmlpp::Node::NodeList::iterator it = root_menu_elem.begin();
	if (it == root_menu_elem.end())
		return false;
	xmlpp::Element * elem = dynamic_cast<xmlpp::Element*>(*it);

	_log.level(slug::log_base::ll_info) << "building menus...\n";
	_log.flush();

	// build popup menu
	_menu = build_menu(elem);

	_log.level(slug::log_base::ll_info) << "building toolbars...\n";
	_log.flush();

	// build toolbars
	xmlpp::Node::NodeList toolbar_elems = root->get_children("toolbar");
	it = toolbar_elems.begin();
	for (; it != toolbar_elems.end(); it++)
	{
		elem = dynamic_cast<xmlpp::Element*>(*it);
		build_toolbar(elem);
	}

	_log.level(slug::log_base::ll_info) << "loading keybindings...\n";
	_log.flush();

	// get keybindings
	xmlpp::Node::NodeList keybindings_elem = root->get_children("keybindings");
	it = keybindings_elem.begin();
	if (it != keybindings_elem.end())
	{
		elem = dynamic_cast<xmlpp::Element*>(*it);
		load_keybindings(elem);
	}

	_log.level(slug::log_base::ll_info) << "loading camera profiles...\n";
	_log.flush();

	// get camera profiles
	xmlpp::Node::NodeList root_profile_elem = root->get_children("profile");
	it = root_profile_elem.begin();
	if (it != root_profile_elem.end())
	{
		elem = dynamic_cast<xmlpp::Element*>(*it);
		load_camera_profiles(elem);
	}

	_log.level(slug::log_base::ll_info) << "loading view layouts...\n";
	_log.flush();

	// get root viewlayout element
	xmlpp::Node::NodeList root_view_elem = root->get_children("viewlayout");
	it = root_view_elem.begin();
	if (it == root_view_elem.end())
		return false;
	elem = dynamic_cast<xmlpp::Element*>(*it);

	_log.level(slug::log_base::ll_info) << "building view layouts...\n";
	_log.flush();

	// build view layout
	Gtk::Widget * views = _layout.build(elem);
	if (views == 0)
	{
		return false;
	}
	_pane.add1(*views);

	return true;
}

void Window::load_keybindings(xmlpp::Element * keybindings)
{
	xmlpp::Node::NodeList bind_elems = keybindings->get_children("bind");
	xmlpp::Node::NodeList::iterator it = bind_elems.begin();
	if (it == bind_elems.end())
		return;
	xmlpp::Element * bind;
	for (; it != bind_elems.end(); it++)
	{
		bind = dynamic_cast<xmlpp::Element*>(*it);

		//<bind command="view::display::shaded" event="Keyboard::5" />
		std::string cmd;
		std::string key;

		xmlpp::Attribute * attr;
		attr = bind->get_attribute("command");
		if (attr)
			cmd = attr->get_value();
		attr = bind->get_attribute("event");
		if (attr)
			key = attr->get_value();

		Event e(key);
		CommandDirectory::instance().bind(cmd, e);
	}
}

void Window::build_toolbar(xmlpp::Element * toolbar)
{
	// get position attribute
	xmlpp::Attribute * attr;
	attr = toolbar->get_attribute("position");
	// value == top | left
	Gtk::Toolbar * bar;
	if (attr && attr->get_value() == "top")
		bar = &_topBar;
	else
		bar = &_leftBar;

	// get style attribute - "icons", "text", "both"; default = "both"
	attr = toolbar->get_attribute("style");
	if (attr)
	{
		if (attr->get_value() == "text")
			bar->set_toolbar_style(Gtk::TOOLBAR_TEXT);
		else if (attr->get_value() == "icons")
			bar->set_toolbar_style(Gtk::TOOLBAR_ICONS);
	}

	// get buttons
	xmlpp::Node::NodeList button_elems = toolbar->get_children("button");
	xmlpp::Node::NodeList::iterator it = button_elems.begin();
	if (it == button_elems.end())
		return;
	xmlpp::Element * button;
	Gtk::ToolButton * item;
	for (; it != button_elems.end(); it++)
	{
		button = dynamic_cast<xmlpp::Element*>(*it);
		// <button name="Select Tool" command="transform::select" icon="select.png" />
		// get button attributes
		xmlpp::Attribute * attr;
		attr = button->get_attribute("name");
		std::string label;
		if (attr)
			label = attr->get_value();
		std::string cmd;
		attr = button->get_attribute("command");
		if (attr)
			cmd = attr->get_value();
		std::string icon;
		attr = button->get_attribute("icon");
		if (attr)
			icon = attr->get_value();
		// create icon
		Gtk::Image * icon_widget = 0;
		if (icon.size() > 0)
		{
			std::string icon_path = VERTICAL3D_SHARED_PATH;
			icon_path += "icons/";
			std::string icon_name = icon_path + icon;
			icon_widget = Gtk::manage(new Gtk::Image(icon_name));
		}

		attr = button->get_attribute("toggle");
		bool toggle_button = false;
		if (attr && attr->get_value() == "true")
			toggle_button = true;

		// create button
		if (icon_widget)
		{
			if (toggle_button)
				item = Gtk::manage(new Gtk::ToggleToolButton(*icon_widget, label));
			else
				item = Gtk::manage(new Gtk::ToolButton(*icon_widget, label));
		}
		else
		{
			if (toggle_button)
				item = Gtk::manage(new Gtk::ToggleToolButton(label));
			else
				item = Gtk::manage(new Gtk::ToolButton(label));
		}

		// connect signal
		if (cmd.size() > 0)
			item->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &Window::dispatch_menu_command), cmd));

		// attach button
		bar->append(*item);
	}
}

void Window::load_camera_profiles(xmlpp::Element * profile)
{
	xmlpp::Node::NodeList camera_elems = profile->get_children("camera");
	xmlpp::Node::NodeList::iterator it = camera_elems.begin();
	if (it == camera_elems.end())
		return;
	xmlpp::Element * camera;
	for (; it != camera_elems.end(); it++)
	{
		camera = dynamic_cast<xmlpp::Element*>(*it);
		CameraProfile cam_profile;

		xmlpp::Attribute * attr;
		attr = camera->get_attribute("name");
		std::string val;
		if (attr)
		{
			cam_profile.name(attr->get_value());
		}
		attr = camera->get_attribute("near");
		if (attr)
		{
			val = attr->get_value();
			cam_profile.near(atof(val.c_str()));
		}
		attr = camera->get_attribute("far");
		if (attr)
		{
			val = attr->get_value();
			cam_profile.far(atof(val.c_str()));
		}
		attr = camera->get_attribute("orthographic");
		if (attr)
		{
			bool ortho = false;
			if (attr->get_value() == "true")
				ortho = true;
			else if (attr->get_value() == "false")
				ortho = false;
			cam_profile.orthographic(ortho);
		}
		attr = camera->get_attribute("adaptive");
		if (attr)
		{
			bool projection = false;
			bool position = false;
			if (attr->get_value() == "projection")
				projection = true;
			else if (attr->get_value() == "position")
				position = true;
			else if (attr->get_value() == "both")
				projection = position = true;
			cam_profile.adaptiveProjection(projection);
			cam_profile.adaptivePosition(position);
		}
		attr = camera->get_attribute("zoom");
		if (attr)
		{
			val = attr->get_value();
			cam_profile.orthoZoom(atof(val.c_str()));
		}
		attr = camera->get_attribute("aspect");
		if (attr)
		{
			val = attr->get_value();
			cam_profile.pixelAspect(atof(val.c_str()));
		}
		attr = camera->get_attribute("eye");
		if (attr)
		{
			cam_profile.eye(Vector3(attr->get_value()));
		}
		attr = camera->get_attribute("up");
		if (attr)
		{
			cam_profile.up(Vector3(attr->get_value()));
		}
		attr = camera->get_attribute("right");
		if (attr)
		{
			cam_profile.right(Vector3(attr->get_value()));
		}
		attr = camera->get_attribute("direction");
		if (attr)
		{
			cam_profile.direction(Vector3(attr->get_value()));
		}
		attr = camera->get_attribute("orientation");
		if (attr)
		{
			Vector3 euler(attr->get_value());
			Quaternion q;
			q.euler(euler[0], euler[1], euler[2]);
			cam_profile.rotation(q);
		}
		attr = camera->get_attribute("lookat");
		if (attr)
		{
			Vector3 origin(attr->get_value());
			cam_profile.lookat(origin);
			
		}
		_profiles.push_back(cam_profile);
	}
}

// build a menu structure
Gtk::Menu * Window::build_menu(xmlpp::Element * root_elem)
{
	// the root menu
	Gtk::Menu * root_menu;
	root_menu = Gtk::manage(new Gtk::Menu());
	Gtk::Menu::MenuList & menu_list = root_menu->items();

	xmlpp::Node::NodeList entries = root_elem->get_children();
	xmlpp::Node::NodeList::iterator it = entries.begin();

	// iterate over each menu entry
	for (; it != entries.end(); it++)
	{
		xmlpp::Element * elem = dynamic_cast<xmlpp::Element*>(*it);
		if (!elem || (elem->get_name() != "menu" && elem->get_name() != "menuitem"))
			continue;

		xmlpp::Attribute * name_attr = elem->get_attribute("name");
		if (elem->get_name() == "menu")
		{
			menu_list.push_back(Gtk::Menu_Helpers::MenuElem(name_attr->get_value(), *(build_submenu(elem))));
		}
		else if (elem->get_name() == "menuitem")
		{
			xmlpp::Attribute * cmd_attr = elem->get_attribute("command");
			menu_list.push_back(Gtk::Menu_Helpers::MenuElem(name_attr->get_value(), sigc::bind(sigc::mem_fun(*this, &Window::dispatch_menu_command), cmd_attr->get_value())));
		}
	}
	return root_menu;
}

// recursive menu builder
Gtk::Menu * Window::build_submenu(xmlpp::Element * menu_elem)
{
	// create a new submenu
	Gtk::Menu * submenu;
	submenu = Gtk::manage(new Gtk::Menu());
	// group for radio menu entries
	Gtk::RadioMenuItem::Group group;
	unsigned int count = 0;
	Gtk::Menu::MenuList & menu_list = submenu->items();

	xmlpp::Node::NodeList entries = menu_elem->get_children();
	xmlpp::Node::NodeList::iterator it = entries.begin();

	// iterate over all menu entries not below this depth
	for (; it != entries.end(); it++) 
	{
		xmlpp::Element * elem = dynamic_cast<xmlpp::Element*>(*it);
		if (!elem || (elem->get_name() != "menu" && elem->get_name() != "menuitem"))
			continue;
		xmlpp::Attribute * name_attr = elem->get_attribute("name");

		if (elem->get_name() == "menu")
		{
			menu_list.push_back(Gtk::Menu_Helpers::MenuElem(name_attr->get_value(), *(build_submenu(elem))));
		}
		else if (elem->get_name() == "menuitem")
		{
			xmlpp::Attribute * type_attr = elem->get_attribute("type");
			xmlpp::Attribute * cmd_attr = elem->get_attribute("command");
			if (type_attr && type_attr->get_value() == "radio")
			{
				menu_list.push_back(Gtk::Menu_Helpers::RadioMenuElem(group, name_attr->get_value(), sigc::bind(sigc::mem_fun(*this, &Window::dispatch_menu_command), cmd_attr->get_value())));
			}
			else if (type_attr && type_attr->get_value() == "check")
			{
				menu_list.push_back(Gtk::Menu_Helpers::CheckMenuElem(name_attr->get_value(), sigc::bind(sigc::mem_fun(*this, &Window::dispatch_menu_command), cmd_attr->get_value())));
			}
			else
			{
				menu_list.push_back(Gtk::Menu_Helpers::MenuElem(name_attr->get_value(), sigc::bind(sigc::mem_fun(*this, &Window::dispatch_menu_command), cmd_attr->get_value())));
			}
		}
	}
	return submenu;
}

void Window::redraw(void)
{
	_layout.invalidate();
}

ViewPort * Window::activeView(void) const
{
	return _viewport;
}

void Window::activeView(unsigned int viewID)
{
	std::vector<ViewPort*>::iterator it = _layout._views.begin();
	for (; it != _layout._views.end(); it++)
	{
		if ((*it)->_active)
		{
			(*it)->_active = false;
		}
		else if ((*it)->_viewID == viewID)
		{
			(*it)->_active = true;
			_viewport = (*it);
		}
	}
	queue_draw();
}

std::string Window::fileChooser(const std::string & title, bool mode)
{
	Gtk::FileChooserDialog dialog(title);
	dialog.set_transient_for(*this);
	
	// add response buttons the the dialog
	dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);

	if (mode)
	{
		dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);
		dialog.set_action(Gtk::FILE_CHOOSER_ACTION_OPEN);
	}
	else
	{
		dialog.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);
		dialog.set_action(Gtk::FILE_CHOOSER_ACTION_SAVE);
	}

	dialog.set_select_multiple(false);

	Gtk::FileFilter filter;
	filter.add_pattern("*.v3d");
	filter.set_name("Vertical|3D Projects");
	dialog.add_filter(filter);

	Gtk::FileFilter filter_all;
	filter_all.add_pattern("*");
	filter_all.set_name("All Files");
	dialog.add_filter(filter_all);

	if (dialog.run() == Gtk::RESPONSE_OK)
		return dialog.get_filename();

	return "";
}

void Window::dispatch_menu_command(const std::string & cmd)
{
	if (CommandDirectory::instance().dispatch(cmd))
	{
		return;
	}
	else if (cmd == "render::view")
	{
		if (!_subWindow)
			_subWindow = new RenderViewWindow();
		else
			_subWindow->show();
	}
	else if (cmd == "render::batch")
	{
		renderScene();
	}
	else if (cmd == "quit")
	{
		Gtk::Main::quit();
	}
	else
	{
		std::cerr << "Unhandled Command! - " << cmd << std::endl;
	}
}

/***
 *** The "key_press_event" signal handler. Any processing required when key
 *** presses occur should be done here.
 ***/
bool Window::on_key_press_event (GdkEventKey * event)
{
	switch (event->keyval)
	{
		case GDK_4:
			CommandDirectory::instance().exec(Event("Keyboard::4"));
			break;
		case GDK_5:
			CommandDirectory::instance().exec(Event("Keyboard::5"));
			break;
		case GDK_6:
			CommandDirectory::instance().exec(Event("Keyboard::6"));
			break;
		case GDK_q:
		case GDK_Q:
			CommandDirectory::instance().exec(Event("Keyboard::Q"));
			break;
		case GDK_w:
		case GDK_W:
			CommandDirectory::instance().exec(Event("Keyboard::W"));
			break;
		case GDK_e:
		case GDK_E:
			CommandDirectory::instance().exec(Event("Keyboard::E"));
			break;
		case GDK_r:
		case GDK_R:
			CommandDirectory::instance().exec(Event("Keyboard::R"));
			break;
		case GDK_t:
		case GDK_T:
			CommandDirectory::instance().exec(Event("Keyboard::T"));
			break;
		case GDK_a:
			CommandDirectory::instance().exec(Event("Keyboard::A"));
//			toggle_animation (widget);
			break;
		case GDK_Alt_L:
			CommandDirectory::instance().exec(Event("Keyboard::Alt"));
			break;
		case GDK_Control_L:
			CommandDirectory::instance().exec(Event("Keyboard::Control"));
			break;
		case GDK_Shift_L:
			CommandDirectory::instance().exec(Event("Keyboard::Shift"));
			break;
		case GDK_space:
			CommandDirectory::instance().exec(Event("Keyboard::Space"));
			break;
		case GDK_Escape:
			CommandDirectory::instance().exec(Event("Keyboard::Escape"));
//			Gtk::Main::quit();
			break;
		default:
			return false;
	}
	Window::instance()->redraw();

	return true;
}

bool Window::on_key_release_event (GdkEventKey * event)
{
	switch (event->keyval)
	{
		case GDK_Alt_L:
			CommandDirectory::instance().exec(Event("Keyboard::Alt"));
			break;
		case GDK_Control_L:
			CommandDirectory::instance().exec(Event("Keyboard::Control"));
			break;
		case GDK_Shift_L:
			CommandDirectory::instance().exec(Event("Keyboard::Shift"));
			break;
		default:
			return false;
	}

	return true;
}

void Window::renderScene(void)
{
#ifdef USE_MOYA
	// get the global instance of the renderer
	Moya::RenderEngine & moya = Moya::RenderEngine::instance();
	// create a new render context
	moya.createRenderContext("");
	Moya::RenderContext & rc = moya.activeRenderContext();
	// setup camera and image settings
	// ...
	moya.prepareWorld();
	// iterate over scene geometry and convert to moya types
	// ...
	std::vector<MeshPtr> & meshes = Project::instance().activeScene()->meshes();
	std::vector<MeshPtr>::iterator it = meshes.begin();
	for (; it != meshes.end(); it++)
	{
		MeshPtr mesh = *it;
		// for each polygon in mesh
		std::vector<Polygon> & polys = mesh->polygons();
		std::vector<Polygon>::iterator pit = polys.begin();
		std::vector<Vector3> & vertices = mesh->vertices();

		//glPushMatrix();
		//glMultMatrixf(mesh->matrix().transpose().data());

		for (; pit != polys.end(); pit++)
		{
			//glColor3f(colors[color_index][0], colors[color_index][1], colors[color_index][2]);

			Moya::PolygonPtr p = new Moya::Polygon;
			//glBegin(GL_POLYGON);
			// for each vertex in polygon
			std::vector<int> & indices = (*pit).vertices();
			std::vector<int>::iterator vit = indices.begin();
			for (; vit != indices.end(); vit++)
			{
				Vector3 v = vertices[(*vit)];
				Moya::Vertex vert;
				vert.point(v);
				p->addVertex(vert);
				//glVertex3f(v[0], v[1], v[2]);
			}
			//glEnd();
			rc.addPolygon(p);
		}
		//glPopMatrix();
	}

	moya.render();
#endif // USE_MOYA
}
