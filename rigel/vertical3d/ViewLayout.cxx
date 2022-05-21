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

#include <gtkmm.h>

#include "ViewLayout.h"

using namespace v3D;


ViewLayout::ViewLayout() : _font("courier 10")
{
}

ViewLayout::~ViewLayout()
{
}

Gtk::Widget * ViewLayout::build_children(xmlpp::Element * elem)
{
	// elem is a viewgroup
	xmlpp::Attribute * attr = elem->get_attribute("type");
	// a viewgroup always has 2 children
	Gtk::Paned * pane;
	if (attr->get_value() == "horizontal")
	{
		pane = Gtk::manage(new Gtk::HPaned());
	}
	else if (attr->get_value() == "vertical")
	{
		pane = Gtk::manage(new Gtk::VPaned());
	}
	else
	{
		return 0;
	}
	_panes.push_back(pane);

	xmlpp::Node::NodeList child_elems = elem->get_children();
	xmlpp::Node::NodeList::iterator it = child_elems.begin();

	xmlpp::Element * view_elem;
	for (; it != child_elems.end(); it++)
	{
		view_elem = dynamic_cast<xmlpp::Element*>(*it);
		if (!view_elem || (view_elem->get_name() != "viewgroup" && view_elem->get_name() != "viewport"))
			continue;
		else
			break;
	}

	if (view_elem->get_name() == "viewgroup")
	{
		Gtk::Widget * child;
		child = build_children(view_elem);
		if (child == 0)
		{
			return 0;
		}
		pane->add1(*child);
	}
	else if (view_elem->get_name() == "viewport")
	{
		ViewPort * view;
		view = Gtk::manage(new ViewPort(_glconfig));
		view->set_size_request(_width / 2, _height / 2);
		attr = view_elem->get_attribute("camera");
		if (attr)
		{
			view->activeCamera(attr->get_value());
		}
		_views.push_back(view);
		pane->add1(*view);
	}
	else
	{
		return 0;
	}

	if (it == child_elems.end())
	{
		return 0;
	}
	// second child
	it++;
	for (; it != child_elems.end(); it++)
	{
		view_elem = dynamic_cast<xmlpp::Element*>(*it);
		if (!view_elem || (view_elem->get_name() != "viewgroup" && view_elem->get_name() != "viewport"))
			continue;
		else
			break;
	}

	if (view_elem->get_name() == "viewgroup")
	{
		Gtk::Widget * child;
		child = build_children(view_elem);
		if (child == 0)
		{
			return 0;
		}
		pane->add2(*child);
	}
	else if (view_elem->get_name() == "viewport")
	{
		// view_elem also has a camera attribute we can use here
		ViewPort * view;
		view = Gtk::manage(new ViewPort(_glconfig));
		view->set_size_request(_width / 2, _height / 2);
		attr = view_elem->get_attribute("camera");
		if (attr)
		{
			view->activeCamera(attr->get_value());
		}
		_views.push_back(view);
		pane->add2(*view);
	}
	else
	{
		return 0;
	}
	return pane;
}

Gtk::Widget * ViewLayout::build(xmlpp::Element * root)
{
	xmlpp::Attribute * attr = root->get_attribute("width");

	if (attr)
		_width = atoi(std::string(attr->get_value()).c_str());
	attr = root->get_attribute("height");
	if (attr)
		_height = atoi(std::string(attr->get_value()).c_str());

	attr = root->get_attribute("font");
	if (attr)
		_font = attr->get_value();

	// there should only be 1 child and it will either be a viewgroup or viewport
	xmlpp::Node::NodeList child_elems = root->get_children();
	xmlpp::Node::NodeList::iterator it = child_elems.begin();

	xmlpp::Element * elem;
	for (; it != child_elems.end(); it++)
	{
		elem = dynamic_cast<xmlpp::Element*>(*it);
		if (!elem || (elem->get_name() != "viewgroup" && elem->get_name() != "viewport"))
			continue;
		else
			break;
	}

	// create gl config
	_glconfig = Gdk::GL::Config::create(Gdk::GL::MODE_RGB | Gdk::GL::MODE_DEPTH | Gdk::GL::MODE_DOUBLE);
	if (!_glconfig)
	{
		std::cerr << "double-buffered visual not available." << std::endl;

		_glconfig = Gdk::GL::Config::create(Gdk::GL::MODE_RGB | Gdk::GL::MODE_DEPTH);
		if (!_glconfig)
		{
			std::cerr << "no opengl capable visual available." << std::endl;
			std::exit(1);
		}
	}

	Gtk::Widget * view;
	if (elem->get_name() == "viewgroup")
	{
		view = build_children(elem);
	}
	else if (elem->get_name() == "viewport")
	{
		ViewPort * viewc;
		viewc = Gtk::manage(new ViewPort(_glconfig));
		viewc->set_size_request(_width / 2, _height / 2);
		_views.push_back(viewc);
		view = viewc;
	}
	else
	{
		return 0;
	}
	return view;
}

void ViewLayout::pop(ViewPort * active_view)
{
	unsigned int view_id = active_view->_viewID;
	bool out = true;
	if (active_view->_size[0] >= _width && active_view->_size[1] >= _height)
		out = false;
	std::vector<ViewPort*>::iterator it = _views.begin();

	for (; it != _views.end(); it++)
	{
		if ((*it)->_viewID == view_id)
		{
			if (out)
				(*it)->set_size_request(_width, _height);
			else
				(*it)->set_size_request(_width / 2, _height / 2);
		}
		else
		{
			if (out)
				(*it)->set_size_request(0, 0);
			else
				(*it)->set_size_request(_width / 2, _height / 2);
		}
	}
}

void ViewLayout::invalidate(void)
{
	std::vector<ViewPort*>::iterator it = _views.begin();

	for (; it != _views.end(); it++)
	{
		(*it)->invalidate();
	}
}
