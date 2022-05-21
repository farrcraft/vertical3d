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
#ifndef INCLUDED_V3D_VIEWPORT_H
#define INCLUDED_V3D_VIEWPORT_H

#include <string>

#include <gtkmm.h>
#include <gtkglmm.h>

#include <GL/gl.h>

#include <libv3dtypes/Vector2.h>
#include <libv3dtypes/Color3.h>
#include <libv3dcore/Camera.h>

#include "ConstructionPlane.h"
#include "ArcBall.h"

namespace v3D
{

	class Window;

	/**
		A 3D scene viewport.
	 */
	class ViewPort : public Gtk::DrawingArea, public Gtk::GL::Widget<ViewPort>
	{
		public:
			ViewPort(const Glib::RefPtr<const Gdk::GL::Config> & config);
			virtual ~ViewPort();

			class MouseState
			{
				public:
					int		_buttons;	// bit fields representing pressed buttons
					int		_dragging;	// bit fields representing movement between button click/release
					Vector2	_position;	// last known screen coordinates of mouse
			};

			typedef enum PolyRenderMode
			{
				POLY_RENDER_POINTS = GL_POINT,
				POLY_RENDER_LINES = GL_LINE,
				POLY_RENDER_FILL = GL_FILL
			} PolyRenderMode;

			typedef enum PolyShadeMode
			{
				POLY_SHADE_FLAT,
				POLY_SHADE_SMOOTH
			} PolyShadeMode;

			typedef enum PolyCullMode
			{
				POLY_CULL_NONE,
				POLY_CULL_FRONT = GL_FRONT,
				POLY_CULL_BACK = GL_BACK,
				POLY_CULL_BOTH = GL_FRONT_AND_BACK
			} PolyCullMode;

			typedef enum VisibleFilter
			{
				SHOW_GRID = 1,
				SHOW_CAMERA,
				SHOW_HANDLE,
				SHOW_LIGHT,
				SHOW_MESH
			} VisibleFilter;

			/*
			e.g. zoom:
			alt + left click + drag
			*/
			typedef enum CameraModes
			{
				NULL_CAMERA_MODE,
				ZOOM_CAMERA_MODE,
				TRUCK_CAMERA_MODE,
				PAN_CAMERA_MODE
			} CameraModes;


			void activeCamera(const std::string &);
			void invalidate(void);

			void cameraMode(CameraModes mode);
			CameraPtr camera(void) const;

			Vector3 unproject(const Vector2 & point);
			Vector3 project(const Vector3 & point);

//		private:
			unsigned int		_viewID;
			unsigned int		_size[2]; // pixel width & height
			bool				_active;
			PolyRenderMode		_frontRenderMode;
			PolyRenderMode		_backRenderMode;
			PolyShadeMode		_shadeMode;
			PolyCullMode		_cullMode;
			unsigned int		_showFlags;

		protected:
			// event handlers
			virtual void on_realize(void);
			virtual bool on_configure_event(GdkEventConfigure * event);
			virtual bool on_expose_event(GdkEventExpose * event);

			virtual bool on_button_press_event(GdkEventButton * event);
			virtual bool on_button_release_event(GdkEventButton * event);
			virtual bool on_motion_notify_event(GdkEventMotion * event);

			virtual bool on_map_event(GdkEventAny * event);
			virtual bool on_unmap_event(GdkEventAny * event);
			virtual bool on_visibility_notify_event(GdkEventVisibility * event);
			virtual bool on_idle(void);

		private:
			void select_object(int hit_id);
			void selection_hit_test(void);

			void idle_add(void);
			void idle_remove(void);

			void update(void);

			void billboard(void);

			void draw_selectable_objects(void);
			void draw_decorations(void);
			void draw(void);

		private:
			CameraPtr				_camera;
			Color3					_backgroundColor;
			ConstructionPlane		_grid;
			MouseState				_mouse;
			std::string				_name;
			unsigned int			_fontList;
			unsigned int			_fontHeight;
			unsigned int			_fontWidth;
			CameraModes				_cameraMode;
			SigC::Connection 		_idleConnection;
			bool 					_animate;
			Glib::RefPtr<Gdk::GC> 	_bgc;

			ArcBall					_arcball;
	};


}; // end namespace v3D

#endif // INCLUDED_V3D_VIEWPORT_H
