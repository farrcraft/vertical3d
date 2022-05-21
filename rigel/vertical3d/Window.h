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
#ifndef INCLUDED_V3D_WINDOW_H
#define INCLUDED_V3D_WINDOW_H

#include <gtkmm.h>
#include <gtkglmm.h>

#include "ViewLayout.h"
#include "ViewPort.h"
#include "manipulators/TransformManipulator.h"

#include <libv3dcore/log/logstream.h>

namespace v3D
{

	/**
		The top level gui window.
	 */
	class Window : public Gtk::Window
	{
		public:
			virtual ~Window();

			static Window * instance(void);

			bool initialize(void);

			typedef enum ToolModes
			{
				NULL_TOOL_MODE,
				SELECT_TOOL_MODE,
				TRANSLATE_TOOL_MODE,
				ROTATE_TOOL_MODE,
				SCALE_TOOL_MODE,
				COMPONENT_TOOL_MODE
			} ToolModes;

			typedef enum SelectMasks
			{
				SELECT_MASK_ANY		= (1 << 0),
				SELECT_MASK_POINTS	= (1 << 1),
				SELECT_MASK_EDGES	= (1 << 2),
				SELECT_MASK_FACES	= (1 << 3),
				SELECT_MASK_MESHES	= (1 << 4),
				SELECT_MASK_LIGHTS	= (1 << 5),
				SELECT_MASK_CAMERAS	= (1 << 6),
				SELECT_MASK_HANDLES	= (1 << 7),
				SELECT_MASK_CURVES	= (1 << 8),
				SELECT_MASK_OBJECTS	= (1 << 9)
			} SelectMasks;

			void redraw(void);
			void activeView(unsigned int viewID);
			ViewPort * activeView(void) const;
			ViewLayout * layout(void);

			std::vector<CameraProfile> & profiles(void);
			ToolModes activeToolMode(void) const;
			void activeToolMode(ToolModes mode);
			Gtk::Menu * menu(void);

			unsigned int selectMasks(void) const;
			void selectMasks(unsigned int mask);

			std::string fileChooser(const std::string & title, bool mode);

			TransformManipulator	* _activeManipulator;

		protected:
			// signal handlers
			virtual bool on_key_press_event(GdkEventKey* event);
			virtual bool on_key_release_event(GdkEventKey* event);

			void dispatch_menu_command(const std::string &);
			bool build_gui(void);
			Gtk::Menu * build_submenu(xmlpp::Element * elem);
			Gtk::Menu * build_menu(xmlpp::Element * elem);
			void load_camera_profiles(xmlpp::Element * profile);
			void build_toolbar(xmlpp::Element * toolbar);
			void load_keybindings(xmlpp::Element * keybindings);

		private:
			Window();
			void renderScene(void);

		private:
			ToolModes		_activeToolMode; // = NULL_TOOL_MODE;
			Gtk::Menu *		_menu;
			unsigned int	_selectMasks;
			Gtk::Window *	_subWindow;
			bool			_initialized;

			std::vector <CameraProfile> _profiles;


			// member widgets
			Gtk::VBox		_vbox;
			Gtk::HBox		_hbox;
			Gtk::Toolbar	_leftBar;
			Gtk::Toolbar	_topBar;
			Gtk::Button		_button;
			Gtk::HPaned		_pane;
			Gtk::Frame		_frame;

			ViewPort *		_viewport; // this is the active viewport
			ViewLayout		_layout;
	};

}; // end namespace v3D

#endif // INCLUDED_V3D_WINDOW_H
