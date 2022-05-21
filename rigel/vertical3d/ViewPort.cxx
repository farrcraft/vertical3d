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
#include <cmath>

#include <GL/glu.h>

#include <libv3dcore/Project.h>

#include "ViewPort.h"
#include "Window.h"

using namespace v3D;

unsigned int baseViewID = 0;


/*
4 bytes = 2^32 = 4,294,967,296
3 bytes = 2^24 =    16,777,216
*/
const unsigned int v3D::SELECT_NAME_OBJECT_LIMIT = 16777216;
const unsigned int v3D::SELECT_NAME_MANIPULATOR_LIMIT = SELECT_NAME_OBJECT_LIMIT + 32;

const unsigned int v3D::SELECT_NAME_X_MANIPULATOR = SELECT_NAME_OBJECT_LIMIT + 1;
const unsigned int v3D::SELECT_NAME_Y_MANIPULATOR = SELECT_NAME_OBJECT_LIMIT + 2;
const unsigned int v3D::SELECT_NAME_Z_MANIPULATOR = SELECT_NAME_OBJECT_LIMIT + 3;


ViewPort::ViewPort(const Glib::RefPtr<const Gdk::GL::Config> & glconfig) : 
					_camera(new Camera()), _frontRenderMode(POLY_RENDER_LINES), 
					_backRenderMode(POLY_RENDER_LINES), _shadeMode(POLY_SHADE_FLAT), 
					_cullMode(POLY_CULL_NONE), _cameraMode(NULL_CAMERA_MODE),
					_showFlags(SHOW_GRID|SHOW_CAMERA|SHOW_HANDLE|SHOW_LIGHT|SHOW_MESH),
					_animate(false), _active(false)
{
	_viewID = baseViewID++;

	set_gl_capability(glconfig);
	add_events (Gdk::POINTER_MOTION_MASK	|
				Gdk::BUTTON_MOTION_MASK		|
				Gdk::BUTTON1_MOTION_MASK	|
				Gdk::BUTTON2_MOTION_MASK	|
				Gdk::BUTTON3_MOTION_MASK	|
				Gdk::BUTTON_PRESS_MASK		|
				Gdk::BUTTON_RELEASE_MASK	|
				Gdk::VISIBILITY_NOTIFY_MASK);
}

ViewPort::~ViewPort()
{
	delete _camera;
}

CameraPtr ViewPort::camera(void) const
{
	return _camera;
}

void ViewPort::invalidate(void)
{
	Gdk::Rectangle allocation = get_allocation();
	get_window()->invalidate_rect(allocation, false);

	queue_draw();
}

void ViewPort::update(void)
{
	// update window synchronously (fast)
	get_window()->process_updates(false);
}

void ViewPort::idle_add(void)
{
	if (!_idleConnection.connected())
		_idleConnection = Glib::signal_idle().connect(sigc::mem_fun(*this, &ViewPort::on_idle), GDK_PRIORITY_REDRAW);
}

void ViewPort::idle_remove(void)
{
	if (_idleConnection.connected())
		_idleConnection.disconnect();
}

bool ViewPort::on_idle(void)
{
	invalidate();
	update();
	return true;
}

/***
 *** The "unmap_event" signal handler. Any processing required when the
 *** OpenGL-capable drawing area is unmapped should be done here.
 ***/
bool ViewPort::on_unmap_event(GdkEventAny * event)
{
	idle_remove();
	return true;
}

/***
 *** The "map_event" signal handler. Any processing required when the
 *** OpenGL-capable drawing area is mapped should be done here.
 ***/
bool ViewPort::on_map_event (GdkEventAny * event)
{
	if (_animate)
		idle_add();

	return true;
}

/***
 *** The "visibility_notify_event" signal handler. Any processing required
 *** when the OpenGL-capable drawing area is visually obscured should be
 *** done here.
 ***/
bool ViewPort::on_visibility_notify_event (GdkEventVisibility * event)
{
	if (_animate)
	{
		if (event->state == GDK_VISIBILITY_FULLY_OBSCURED)
			idle_remove();
		else
			idle_add();
	}

	return true;
}

void ViewPort::activeCamera(const std::string & camera)
{
	if (!_camera)
		return;
	std::vector<CameraProfile>::iterator it = Window::instance()->profiles().begin();
	for (; it != Window::instance()->profiles().end(); it++)
	{
		if ((*it).name() == camera)
		{
			_camera->profile(*it);
			_name = _camera->name();
			return;
		}
	}
}


/***
 *** The "realize" signal handler. All the OpenGL initialization
 *** should be performed here, such as default background color,
 *** certain states etc.
 ***/
void ViewPort::on_realize(void)
{
	// call base
	Gtk::DrawingArea::on_realize();
	// get window
	Glib::RefPtr<Gdk::GL::Window> glwindow = get_gl_window();

	if (!glwindow->gl_begin(get_gl_context()))
		return;

	// font creation code
	_fontList = glGenLists(128);
	Glib::ustring label = Window::instance()->layout()->_font;
	Pango::FontDescription font_desc(label);
	Glib::RefPtr<Pango::Font> font = Gdk::GL::Font::use_pango_font(font_desc, 0, 128, _fontList);
	if (!font)
	{
		std::cerr << "error loading view font " << label << std::endl;
		exit(EXIT_FAILURE);
	}
	Pango::FontMetrics font_metrics = font->get_metrics();
	_fontHeight = font_metrics.get_ascent() + font_metrics.get_descent();
	_fontHeight = PANGO_PIXELS(_fontHeight);
	_fontWidth = font_metrics.get_approximate_char_width();
	_fontWidth = PANGO_PIXELS(_fontWidth);

	// stuff for drawing the active viewport border
	_bgc = Gdk::GC::create(get_window());
	Gdk::Color bc;
	Glib::RefPtr<Gdk::Colormap> bcmp = Gdk::Colormap::get_system();
	bc.set_red(65535);
	bc.set_green(0);
	bc.set_blue(0);
	bcmp->alloc_color(bc);
	_bgc->set_foreground(bc);

	glEnable(GL_DEPTH_TEST);

	glwindow->gl_end();
}

/***
 *** The "configure_event" signal handler. Any processing required when
 *** the OpenGL-capable drawing area is re-configured should be done here.
 *** Almost always it will be used to resize the OpenGL viewport when
 *** the window is resized.
 ***/
bool ViewPort::on_configure_event(GdkEventConfigure * event)
{
	// get window
	Glib::RefPtr<Gdk::GL::Window> glwindow = get_gl_window();

	GLfloat width = get_width();
	GLfloat height = get_height();

	if (!glwindow->gl_begin(get_gl_context()))
		return false;

	// map NDC coordinates to window coordinates
	glViewport (0, 0, static_cast<int>(width), static_cast<int>(height));

	// save viewport size
	_size[0] = static_cast<unsigned int>(width);
	_size[1] = static_cast<unsigned int>(height);

	_arcball.bounds(width, height);

	// set camera pixel aspect ratio
	float aspect;
	aspect = static_cast<float>(width) / static_cast<float>(height);
	_camera->pixelAspect(aspect);

	glwindow->gl_end();

	return true;
}

/***
 *** The "expose_event" signal handler. All the OpenGL re-drawing should
 *** be done here. This is repeatedly called as the painting routine
 *** every time the 'expose'/'draw' event is signalled.
 ***/
bool ViewPort::on_expose_event(GdkEventExpose * event)
{
	// get window
	Glib::RefPtr<Gdk::GL::Window> glwindow = get_gl_window();

	/*** OpenGL BEGIN ***/
	if (!glwindow->gl_begin(get_gl_context()))
		return false;

	// set clear color to backgroundColor
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	draw();

	// draw viewport label

	// text color
	glColor3f (0.0, 0.1, 0.8);

	// disable lighting
	glDisable(GL_LIGHTING);

	// push clean matrices onto the stack
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	// set text position
	float text_w = (_fontWidth * _name.length()) / 2.0;
	float offset = (text_w / 2.0) * (2.0 / _size[0]);
	float text_h = (_fontHeight / 2.0) * (2.0 / _size[1]);
	glRasterPos3f(-offset, -1.0 + text_h, 0.0);

	// render text
	glListBase(_fontList);
	glCallLists(_name.length(), GL_UNSIGNED_BYTE, _name.c_str());

	// restore matrix stacks
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	// re-enable lighting
	glEnable(GL_LIGHTING);

	// sync
	glwindow->wait_gl();

	// Swap buffers
	if (glwindow->is_double_buffered ())
		glwindow->swap_buffers ();
	else
		glFlush ();

	glwindow->gl_end();

	// draw border if viewport is active (and is not minimized)
	if (_active && _size[0] > 0 && _size[1] > 0)
	{
		glwindow->draw_rectangle(_bgc, 0, 0, 0, _size[0] - 1, _size[1] - 1);
	}

	// sync
	glwindow->wait_gdk();

	return true;
}

void ViewPort::cameraMode(CameraModes mode)
{
	if (_cameraMode != mode)
		_cameraMode = mode;
	else
		_cameraMode = NULL_CAMERA_MODE;
}

/***
 *** The "motion_notify_event" signal handler. Any processing required when
 *** the OpenGL-capable drawing area is under drag motion should be done here.
 ***/
bool ViewPort::on_motion_notify_event (GdkEventMotion * event)
{
	if (event->state & GDK_BUTTON1_MASK)
	{
		// set mouse drag flag
		_mouse._dragging |= (1 << 1);

		if (_cameraMode == ZOOM_CAMERA_MODE)
		{
			/* do camera zooming/dollying
				ortho cameras zoom
				perspective cameras dolly
				event should have current x and y 
				viewport mouse should have previous x and y
			*/
			float delta;
			delta = event->x - _mouse._position[0];
			float factor;
			if (_camera->orthographic())
			{
				factor = (_camera->orthoZoom() * 2.0 * _camera->pixelAspect()) / _size[0];
				_camera->zoom(-delta * factor);
			}
			else
			{
				factor = 0.125;
				_camera->dolly(delta * factor);
			}
		}
		else if (_cameraMode == TRUCK_CAMERA_MODE)
		{
			float delta_x, delta_y;
			delta_x = event->x - _mouse._position[0];
			delta_y = event->y - _mouse._position[1];
			float factor;
			if (delta_x != 0.0)
			{	if (_camera->orthographic())
					factor = (_camera->orthoZoom() * 2.0 * _camera->pixelAspect()) / _size[0];
				else
					factor = 0.125;
				_camera->truck(delta_x * -factor);
			}
			if (delta_y != 0.0)
			{
				if (_camera->orthographic())
					factor = (_camera->orthoZoom() * 2.0) / _size[1];
				else
					factor = 0.125;
				_camera->pedestal(delta_y * factor);
			}
		}
		else if (_cameraMode == PAN_CAMERA_MODE)
		{
			if (!_camera->orthographic())
			{
				Quaternion rot;
				rot = _arcball.drag(_mouse._position);
				Quaternion cam_rot, new_rot;
				cam_rot = _camera->rotation();
				if (rot[0] != 0.0 && rot[1] != 0.0 && rot[2] != 0.0 && rot[3] != 0.0)
				{
					new_rot = rot * cam_rot;
					_camera->rotation(new_rot);
				}
			}
		}
		else if (Project::instance()._selection)
		{
			if (Window::instance()->_activeManipulator)
			{
				Vector2 mouse_delta;
				mouse_delta[0] = event->x - _mouse._position[0];
				mouse_delta[1] = event->y - _mouse._position[1];
				Window::instance()->_activeManipulator->transform(mouse_delta);
			}
		}
	}

	if (event->state & GDK_BUTTON2_MASK)
	{
	}

	if (event->state & GDK_BUTTON3_MASK)
	{
	}

	if (Project::instance().activeTool() != 0)
	{
		Project::instance().activeTool()->motion(static_cast<int>(event->x), static_cast<int>(event->y));
	}

	// capture current mouse position
	_mouse._position.set(event->x, event->y);

	Window::instance()->redraw();

	return false;
}

/***
 *** The "button_press_event" signal handler. Any processing required when
 *** mouse buttons (only left and middle buttons) are pressed on the OpenGL-
 *** capable drawing area should be done here.
 ***/
bool ViewPort::on_button_press_event (GdkEventButton * event)
{
	_mouse._buttons |= (1 << event->button);
	_mouse._position.set(event->x, event->y);

	if (Project::instance().activeTool())
		Project::instance().activeTool()->button(event->button, true, static_cast<int>(event->x), static_cast<int>(event->y));

	if (event->button == 1)
	{
		if ((_cameraMode == PAN_CAMERA_MODE) && !_camera->orthographic())
		{
			_arcball.click(_mouse._position);
		}
	
		return true;
	}

	if (event->button == 2)
	{
		/*** Fill in the details here. ***/

		return true;
	}

	if (event->button == 3)
	{
		Window::instance()->menu()->popup(event->button, event->time);
		return true;
	}

	return false;
}

bool ViewPort::on_button_release_event (GdkEventButton * event)
{
	_mouse._buttons &= ~(1 << event->button);
	_mouse._position.set(event->x, event->y);

	if (Project::instance().activeTool())
		Project::instance().activeTool()->button(event->button, false, static_cast<int>(event->x), static_cast<int>(event->y));

	if (event->button == 1)
	{
		if (!(_mouse._dragging & (1 << event->button)) && _active &&
			(Window::instance()->activeToolMode() == Window::SELECT_TOOL_MODE || 
			 Window::instance()->activeToolMode() == Window::TRANSLATE_TOOL_MODE ||
			 Window::instance()->activeToolMode() == Window::ROTATE_TOOL_MODE ||
			 Window::instance()->activeToolMode() == Window::SCALE_TOOL_MODE))
		{
			selection_hit_test();
		}
		else if (!(_mouse._dragging & (1 << event->button)) && !_active)
		{
			// set this view to active if it isn't already
			Window::instance()->activeView(_viewID);
		}
		else
		{
			_mouse._dragging &= ~(1 << event->button);
		}
		return true;
	}

	if (event->button == 2)
	{
		/*** Fill in the details here. ***/
		return true;
	}
	return false;
}

void ViewPort::draw_selectable_objects(void)
{
	if (!(_showFlags & SHOW_MESH))
		return;

	/*
		viewport member data accessed in this method...
		front/back render mode
	*/
	if (!Project::instance().activeScene())
		return;

	std::vector<MeshPtr> & meshes = Project::instance().activeScene()->meshes();
	std::vector<MeshPtr>::iterator it = meshes.begin();
	for (; it != meshes.end(); it++)
	{
		MeshPtr mesh = *it;
		/*
			to draw each mesh:

			draw mesh normally
			using selected or unselected mesh color
			either in wireframe or filled mode

			if filled mode and selected
			draw wireframe over filled version

			if subobject selection mode active
			draw component handles

			if subobject selection mode active
			draw selected components in selected color

			if selected and manipulator mode active
			draw manipulator centered at selection middle
		*/

		/*
			each selectable object that is rendered should begin with a 
			call to glLoadName(int_id) to bind an id used for selection hit testing
			if the object is already selected, don't bind a name
			this prevents reselecting already selected objects
		*/
		if (!mesh->selected())
			glLoadName(mesh->id());

		// apply object transform
		glPushMatrix();
		glMultMatrixf(mesh->matrix().transpose().data());

		// first pass - draw mesh normally
		glPolygonMode(GL_FRONT, _frontRenderMode);
		glPolygonMode(GL_BACK, _backRenderMode);
		glPolygonOffset(1.0, 1.0);

//		std::cerr << "draw first pass start" << std::endl;

		// draw mesh faces
		unsigned int face_count = 0;
//		std::cerr << "draw face count = " << mesh->faceCount() << " edge count = " << mesh->edgeCount() << " vertex count = " << mesh->vertexCount() << std::endl;
		for (; face_count < mesh->faceCount(); face_count++)
		{
			// bind an id to use for hit-testing this face
			if (mesh->selected() && Window::instance()->selectMasks() == Window::SELECT_MASK_FACES)
				glLoadName(SELECT_NAME_MANIPULATOR_LIMIT + face_count + 1);

			// set color used for drawing the face
			if (mesh->selected())
			{
				if (_frontRenderMode != POLY_RENDER_FILL)
					glColor3f(0.7, 0.8, 1.0);
				else
					glColor3f(0.65, 0.65, 0.65);
			}
			else
				glColor3f(0.1, 0.1, 0.5);

			// set drawing mode for face - filled polygons or wireframes
			if (_frontRenderMode != POLY_RENDER_FILL)
				glBegin(GL_LINE_LOOP);
			else
				glBegin(GL_POLYGON);

//			std::cerr << "draw vertices start" << std::endl;

			Mesh::vertex_iterator it(mesh, face_count);
			Vertex * vertex;
			for (; *it != 0; it++)
			{
				Vector3 v;
				vertex = *it;
				v = vertex->point();
//				std::cerr << "draw point " << v << std::endl;
				glVertex3f(v[0], v[1], v[2]);
			}

			// done drawing face
			glEnd();
		}
//		std::cerr << "draw first pass end" << std::endl;

		// remaining passes only apply to selected meshes
		if (mesh->selected())
		{
//			std::cerr << "draw last pass start" << std::endl;
			// keep track of where to draw the transform manipulator
			Vector3 manip_center(0.0, 0.0, 0.0);
			unsigned int npoints = 0;

			// second pass - draw wireframe mesh
	
			// if not in wireframe display mode
			// overdraw the polygon a second time using a different color in wireframe mode
			// this gives the effect of a shaded object with visible wireframe geometry
			if (_frontRenderMode == POLY_RENDER_FILL)
			{
				glColor3f(0.7, 0.8, 1.0);
				glPolygonMode(GL_FRONT, GL_LINE);
				glPolygonMode(GL_BACK, GL_LINE);
				// disable polygon offset
				glPolygonOffset(0.0, 0.0);

				// draw mesh faces
				unsigned int face_count = 0;
				for (; face_count < mesh->faceCount(); face_count++)
				{
					glBegin(GL_POLYGON);
		
					Mesh::vertex_iterator it(mesh, face_count);
					Vertex * vertex;
					for (; *it != 0; it++)
					{
						Vector3 v;
						vertex = *it;
						v = vertex->point();
						glVertex3f(v[0], v[1], v[2]);
					}

					// done drawing face
					glEnd();
				}
			}

			// third pass - draw selection handles

			// draw face selection handle
			if (Window::instance()->selectMasks() == Window::SELECT_MASK_FACES)
			{
				// draw mesh faces
				unsigned int face_count = 0;
				for (; face_count < mesh->faceCount(); face_count++)
				{
					// bind an id to use for hit-testing this face
					glLoadName(SELECT_NAME_MANIPULATOR_LIMIT + face_count + 1);

					glColor3f(1.0, 0.8, 0.7);

					// find the midpoint of the polygon to use as the handle center
					Vector3 mid = mesh->center(face_count);

					// center manipulator at handle
					if (mesh->face(face_count)->selected())
					{
						manip_center += mid;
						npoints++;
					}

					// calculate the polygon normal and the u, v direction vectors
					Vector3 u, v;
					mesh->faceUV(face_count, u, v);

					// scale down the u & v direction vectors to use as a step value
					u *= 0.025;
					v *= 0.025;
	
					// draw a selection handle for the polygon (face)
					glPolygonMode(GL_FRONT, GL_FILL);
					glBegin(GL_POLYGON);
					Vector3 handle;
					handle = mid - u - v;
					std::cerr << "handle = " << handle << "mid = " << mid << " u = " << u << " v = " << v << std::endl;
/* FIXME: bug somewhere in this block...
*/
					glVertex3f(handle[0], handle[1], handle[2]);
					handle = mid + u - v;
					glVertex3f(handle[0], handle[1], handle[2]);
					handle = mid + u + v;
					glVertex3f(handle[0], handle[1], handle[2]);
					handle = mid - u + v;
					glVertex3f(handle[0], handle[1], handle[2]);
/* */
					glEnd();
				}
			}
			// draw vertex selection handles
			if (Window::instance()->selectMasks() == Window::SELECT_MASK_POINTS)
			{
				unsigned int face_count = 0;
				for (; face_count < mesh->faceCount(); face_count++)
				{
					// calculate the polygon normal and the u, v direction vectors
					Vector3 u, v;
					mesh->faceUV(face_count, u, v);

					// scale down the u & v direction vectors to use as a step value
					u *= 0.0125;
					v *= 0.0125;

					glPolygonMode(GL_FRONT, GL_FILL);
					glColor3f(1.0, 0.8, 0.7);

					// for each vertex in polygon
					Vector3 handle;
					unsigned int vertex_count = 0;
					for (; vertex_count < mesh->faceCount(); vertex_count++)
					{
						// bind an id to use for hit-testing this vertex
						glLoadName(SELECT_NAME_MANIPULATOR_LIMIT + vertex_count + 1);

						Vertex * vertex = mesh->vertex(vertex_count);
						Vector3 mid = vertex->point();

						// center manipulator at handle
						if (vertex->selected())
						{
							manip_center += mid;
							npoints++;
						}

						// draw a selection handle for this vertex in the polygon
						handle = mid - u - v;
						glBegin(GL_POLYGON);
						glVertex3f(handle[0], handle[1], handle[2]);
						handle = mid + u - v;
						glVertex3f(handle[0], handle[1], handle[2]);
						handle = mid + u + v;
						glVertex3f(handle[0], handle[1], handle[2]);
						handle = mid - u + v;
						glVertex3f(handle[0], handle[1], handle[2]);
						glEnd();
					}
				}
			}

			// fourth pass - draw selected components

			// draw selected faces
			if (Window::instance()->selectMasks() == Window::SELECT_MASK_FACES)
			{
				glColor3f(0.7, 0.5, 0.4);
				// for each polygon in mesh
				unsigned int face_count = 0;
				for (; face_count < mesh->faceCount(); face_count++)
				{
					if (!mesh->face(face_count)->selected())
						continue;

					if (_frontRenderMode != POLY_RENDER_FILL)
						glBegin(GL_LINE_LOOP);
					else
						glBegin(GL_POLYGON);

					// for each vertex in polygon
					Mesh::vertex_iterator it(mesh, face_count);
					Vertex * vertex;
					for (; *it != 0; it++)
					{
						Vector3 v;
						vertex = *it;
						v = vertex->point();
						glVertex3f(v[0], v[1], v[2]);
					}

					glEnd();
				}
			}

			// draw selected edges
			if (Window::instance()->selectMasks() == Window::SELECT_MASK_EDGES)
			{
				unsigned int edge_count = 0;
				for (; edge_count < mesh->edgeCount(); edge_count++)
				{
					HalfEdge * edge = mesh->edge(edge_count);
					Vector3 v1 = mesh->vertex(edge->vertex())->point();
					Vector3 v2 = mesh->vertex(mesh->edge(edge->next())->vertex())->point();

					if (edge->selected())
					{
						// center manipulator at handle
						manip_center += v1;
						manip_center += v2;
						npoints += 2;

						glColor3f(0.7, 0.5, 0.4);
					}
					else
						glColor3f(0.7, 0.8, 1.0);

					glLoadName(SELECT_NAME_MANIPULATOR_LIMIT + edge_count + 1);

					glBegin(GL_LINES);
					glVertex3f(v1[0], v1[1], v1[2]);
					glVertex3f(v2[0], v2[1], v2[2]);
					glEnd();
				}
			}

			// draw selected vertices
			if (Window::instance()->selectMasks() == Window::SELECT_MASK_POINTS)
			{
				unsigned int face_count = 0;
				for (; face_count < mesh->faceCount(); face_count++)
				{
					// calculate the polygon normal and the u, v direction vectors
					Vector3 u, v;
					mesh->faceUV(face_count, u, v);

					// scale down the u & v direction vectors to use as a step value
					u *= 0.0125;
					v *= 0.0125;

					glPolygonMode(GL_FRONT, GL_FILL);
					glColor3f(0.7, 0.5, 0.4);

					// for each vertex in polygon
					Vector3 handle, mid;
					Mesh::vertex_iterator it(mesh, face_count);
					Vertex * vertex;
					for (; *it != 0; it++)
					{
						vertex = *it;
						if (!vertex->selected())
							continue;

						mid = vertex->point();
						handle = mid - u - v;
						glBegin(GL_POLYGON);
						glVertex3f(handle[0], handle[1], handle[2]);
						handle = mid + u - v;
						glVertex3f(handle[0], handle[1], handle[2]);
						handle = mid + u + v;
						glVertex3f(handle[0], handle[1], handle[2]);
						handle = mid - u + v;
						glVertex3f(handle[0], handle[1], handle[2]);
						glEnd();
					}
				}
			}

			// last pass - draw transform manipulators
			glPopMatrix();
	
			// draw transform manipulators
			if (Window::instance()->_activeManipulator)
			{
				glPushMatrix();
				// use the mesh transform matrix without the scale portion
				Matrix4 m;
				Vector3 trans = mesh->translation();
				// if a subobject mode is active, translate to the center of selection
				if (npoints > 0)
					manip_center /= static_cast<float>(npoints);
				trans += manip_center;

				Quaternion rot = mesh->rotation();
				m.identity();
				m.translate(trans[0], trans[1], trans[2]);
				if (_camera->orthographic())
				{
					float zoom = _camera->orthoZoom() / 6.0;
					m.scale(zoom, zoom, zoom);
				}
				m *= rot.matrix();
				glMultMatrixf(m.transpose().data());
				glDisable(GL_DEPTH_TEST);
	
				Window::instance()->_activeManipulator->draw();
	
				glEnable(GL_DEPTH_TEST);
	
				glPopMatrix();
			}
//			std::cerr << "draw last pass end" << std::endl;
		}
	}

	if (Project::instance().activeTool())
		Project::instance().activeTool()->draw();
}

// accomplish a billboard by clearing the rotation part of the modelview
void ViewPort::billboard(void)
{
	float temp[16] = 
	{
		1.0f, 0.0f, 0.0f, 0.0f, 
		0.0f, 1.0f, 0.0f, 0.0f, 
		0.0f, 0.0f, 1.0f, 0.0f, 
		0.0f, 0.0f, 0.0f, 1.0f
	};
	float m[16];

	glGetFloatv(GL_MODELVIEW_MATRIX, m);
	temp[0] = m[0];
	temp[1] = m[4];
	temp[2] = m[8];
	temp[4] = m[1];
	temp[5] = m[5];
	temp[6] = m[9];
	temp[8] = m[2];
	temp[9] = m[6];
	temp[10] = m[10];
	glMultMatrixf(temp);
}

Vector3 ViewPort::unproject(const Vector2 & point)
{
	// get the viewport dimensions
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	// get the z value from the depth buffer at the given screen coordinates
	GLfloat z;
	glReadPixels(static_cast<int>(point[0]), static_cast<int>(point[1]), 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &z);
	Vector3 p(point[0], point[1], z);

	return _camera->unproject(p, viewport);
}

Vector3 ViewPort::project(const Vector3 & point)
{
	// get the viewport dimensions
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	return _camera->project(point, viewport);
}

void ViewPort::draw_decorations(void)
{
	// turn off lighting
	glDisable(GL_LIGHTING);

	// focus on a small area in the lower left corner of the viewport
	int w = 75;
	int h = 75;
	glViewport (0, 0, w, h);

	// clear the projection
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	// build a fixed orthographic projection
	Matrix4 projection;
	float near = _camera->near();
	float far = _camera->far();
	
	float left = -1.0;
	float right = 1.0;
	float top = 1.0;
	float bottom = -1.0;
	float tx = (right + left) / (right - left);
	float ty = (top + bottom) / (top - bottom);
	float tz = (far + near) / (far - near);

	projection[0] = 2.0 / (right - left);
	projection[1] = projection[2] = 0.0;
	projection[3] = tx;
	projection[4] = 0.0;
	projection[5] = 2.0 / (top - bottom);
	projection[6] = 0.0;
	projection[7] = ty;
	projection[8] = projection[9] = 0.0;
	projection[10] = -2.0 / (far - near);
	projection[11] = tz;
	projection[12] = projection[13] = projection[14] = 0.0;
	projection[15] = 1.0;

	// put the projection onto the stack
	Matrix4 p2 = projection.transpose();
	glMultMatrixf(p2.data());

	// clear the modelview matrix
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	// since we use a right-handed coordinate space internally
	// we need to flip z when working with gl
	glScalef(1.0, 1.0, -1.0);
	Matrix4 view;
	view.identity();
	view.translate(0.0, 0.0, -1.0);
	view *= _camera->rotation().matrix().transpose();
	glMultMatrixf(view.transpose().data());

	// draw the x axis line
	glBegin(GL_LINES);
	// x axis color is red
	glColor3f(1.0, 0.0, 0.0);
	glVertex3f(0.0, 0.0, 0.0);
	glVertex3f(0.5, 0.0, 0.0);
	glEnd();

	// y axis (green)
	glBegin(GL_LINES);
	glColor3f(0.0, 1.0, 0.0);
	glVertex3f(0.0, 0.0, 0.0);
	glVertex3f(0.0, 0.5, 0.0);
	glEnd();

	// z axis (blue)
	glBegin(GL_LINES);
	glColor3f(0.0, 0.0, 1.0);
	glVertex3f(0.0, 0.0, 0.0);
	glVertex3f(0.0, 0.0, 0.5);
	glEnd();

	// draw the x label
	glColor3f(1.0, 0.0, 0.0);

	glPushMatrix();
	glTranslatef(0.5, 0.0, 0.1);
	billboard();
	glScalef(0.5, 0.5, 0.5);

	glBegin(GL_LINES);

	glVertex3f(0.0, 0.0, 0.0);
	glVertex3f(0.33, 0.5, 0.0);

	glVertex3f(0.33, 0.0, 0.0);
	glVertex3f(0.0, 0.5, 0.0);

	glEnd();
	glPopMatrix();

	// y label
	glColor3f(0.0, 1.0, 0.0);

	glPushMatrix();
	glTranslatef(0.0, 0.5, 0.1);
	billboard();
	glScalef(0.5, 0.5, 0.5);

	glBegin(GL_LINES);

	glVertex3f(0.0, 0.0, 0.0);
	glVertex3f(0.33, .5, 0.0);

	glVertex3f(0.16, 0.25, 0.0);
	glVertex3f(0.0, 0.5, 0.0);

	glEnd();
	glPopMatrix();

	// z label
	glColor3f(0.0, 0.0, 1.0);

	glPushMatrix();
	glTranslatef(0.0, 0.0, 0.5);
	billboard();
	glScalef(0.5, 0.5, 0.5);

	glBegin(GL_LINES);

	glVertex3f(0.0, 0.0, 0.0);
	glVertex3f(0.33, 0.0, 0.0);

	glVertex3f(0.0, 0.5, 0.0);
	glVertex3f(0.33, 0.5, 0.0);

	glVertex3f(0.0, 0.0, 0.0);
	glVertex3f(0.33, 0.5, 0.0);

	glEnd();
	glPopMatrix();

	// unwind the matrix stacks
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	// reset the viewport window
	glViewport (0, 0, _size[0], _size[1]);
	// re-enable lighting
	glEnable(GL_LIGHTING);
}

void ViewPort::draw(void)
{
	// default lighting
	GLfloat ambient[]  = { 0.5, 0.5, 0.5, 1.0 };
	GLfloat diffuse[]  = { 1.0, 1.0, 1.0, 1.0 };
	GLfloat position[] = { 2.0, 100.0, 2.0, 1.0 };
	glLightfv(GL_LIGHT1, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT1, GL_POSITION, position);
	glEnable(GL_LIGHT1);
	glEnable(GL_LIGHTING);
	glEnable(GL_COLOR_MATERIAL);

	// initialize projection matrix
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	// append the camera's projection matrix
	_camera->createProjection();
	Matrix4 p = _camera->projection().transpose();
	glMultMatrixf(p.data());

	// modelview contains modeling and viewing transforms
	glMatrixMode(GL_MODELVIEW);
	// start fresh
	glLoadIdentity();

	// since we use a right-handed coordinate space internally
	// we need to flip z when working with gl
	glScalef(1.0, 1.0, -1.0);

	// apply camera transform
	// http://www.opengl.org/resources/faq/technical/viewing.htm
	// http://www.evl.uic.edu/ralph/508S98/coordinates.html
	_camera->createView();
	Matrix4 m = _camera->view().transpose();
	glMultMatrixf(m.data());

	// set clear color to backgroundColor
	glClearColor(0.4, 0.4, 0.4, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	glPolygonMode(GL_FRONT, _frontRenderMode);
	glPolygonMode(GL_BACK, _backRenderMode);
	if (_cullMode != POLY_CULL_NONE)
	{
		glEnable(GL_CULL_FACE);
		glCullFace(_cullMode);
	}
	else
	{
		glDisable(GL_CULL_FACE);
	}

	if (_showFlags & SHOW_GRID)
	{
		glDisable(GL_LIGHTING);
		glPushMatrix();
		_grid.draw(_camera);
		glPopMatrix();
		glEnable(GL_LIGHTING);
	}

	glColor3f(1.0, 1.0, 1.0);

	draw_selectable_objects();
	draw_decorations();
}

void ViewPort::select_object(int hit_id)
{
	// select the object with the given id
	// may also need to un-select other objects depending on modifiers
	std::vector<MeshPtr> & meshes = Project::instance().activeScene()->meshes();
	std::vector<MeshPtr>::iterator it = meshes.begin();
	for (; it != meshes.end(); it++)
	{
		if ((*it)->id() == hit_id)
		{
			(*it)->selected(true);
			Project::instance()._selection = (*it);
		}
		else
		{
			if ((*it)->selected())
				(*it)->selected(false);
		}
	}
}


/*
this should probably be refactored a bit.
add ViewPort::pickMatrix()
most of the camera stuff is already in the normal draw() method
maybe most of it should be pushed into draw() with conditional 
checks for a select render mode.
*/
void ViewPort::selection_hit_test(void)
{
	GLuint selection_buffer[512];
	GLint selection_hits;
	GLint viewport[4];
	// how close to the click point in pixels to consider for selection
	real_t pick_tolerance = 3.0;

	// get viewport dimensions and set selection buffer
	glGetIntegerv(GL_VIEWPORT, viewport);
	glSelectBuffer(512, selection_buffer);

	// switch to selection mode
	glRenderMode(GL_SELECT);
	// initialize the name stack and assert that there will always be
	// at least one entry in it.
	glInitNames();
	glPushName(0);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	// build a pick matrix - based on gluPickMatrix
	real_t sx, sy, tx, ty, width, height, x, y;
	width = pick_tolerance;
	height = pick_tolerance;
	x = _mouse._position[0];
	y = viewport[3] - _mouse._position[1];
	sx = viewport[2] / width;
	sy = viewport[3] / height;
	tx = (viewport[2] + 2.0 * (viewport[0] - x)) / width;
	ty = (viewport[3] + 2.0 * (viewport[1] - y)) / height;
	Matrix4 p;
	p.identity();
	p[0] = sx;
	p[5] = sy;
	p[7] = ty;
	p[3] = tx;
	glMultMatrixf(p.transpose().data());

	// build the camera projection
	_camera->createProjection();
	p = _camera->projection().transpose();
	glMultMatrixf(p.data());

	// build camera viewing matrix
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	// camera is right-handed so flip z axis
	glScalef(1.0, 1.0, -1.0);
	// build the camera view
	_camera->createView();
	Matrix4 m = _camera->view().transpose();
	glMultMatrixf(m.data());

	// draw selectable objects
	draw_selectable_objects();

	// cleanup matrix stacks
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glFlush();

	// switch back to render mode and get the selection hit count
	selection_hits = glRenderMode(GL_RENDER);

	if (selection_hits > 0)
	{
		int hit_name = selection_buffer[3];
		int depth  = selection_buffer[1];
		for (int loop = 1; loop < selection_hits; loop++)
		{
			if ((selection_buffer[loop * 4 + 1] < depth) || (hit_name == 0))
			{
				hit_name = selection_buffer[loop * 4 + 3];
				depth = selection_buffer[loop * 4 + 1];
			}
		}
		/*
			how to make selection more intelligent...
			possible hit name types: object, face, edge, vertex, manipulator
			an object must be selected before any of its components may be selected
		*/
		// use the hit_name to select the object
		if (hit_name <= SELECT_NAME_OBJECT_LIMIT)
		{
			select_object(hit_name);
			//Window::instance()->_activeManipulator->axis(TransformManipulator::CONSTRAINT_NONE);
		}
		else
		{
			if (hit_name > SELECT_NAME_OBJECT_LIMIT && hit_name <= SELECT_NAME_MANIPULATOR_LIMIT)
			{
				if (hit_name == SELECT_NAME_X_MANIPULATOR)
				{
					Window::instance()->_activeManipulator->axis(TransformManipulator::CONSTRAINT_X_AXIS);
				}
				else if (hit_name == SELECT_NAME_Y_MANIPULATOR)
				{
					Window::instance()->_activeManipulator->axis(TransformManipulator::CONSTRAINT_Y_AXIS);
				}
				else if (hit_name == SELECT_NAME_Z_MANIPULATOR)
				{
					Window::instance()->_activeManipulator->axis(TransformManipulator::CONSTRAINT_Z_AXIS);
				}
			}
			else if (hit_name > SELECT_NAME_MANIPULATOR_LIMIT)
			{
				// select an object component based on active selection mask
				if (Window::instance()->selectMasks() == Window::SELECT_MASK_FACES)
				{
					unsigned int id = hit_name - SELECT_NAME_MANIPULATOR_LIMIT;
					// get selected object
					MeshPtr mesh = dynamic_cast<MeshPtr>(Project::instance()._selection);
					// find selected face
					unsigned int face_count = 0;
					for (; face_count < mesh->faceCount(); face_count++)
					{
						Face * face = mesh->face(face_count);
						if ((face_count + 1) == id)
							face->selected(!face->selected());
						else
							face->selected(false);
						
					}
				}
				else if (Window::instance()->selectMasks() == Window::SELECT_MASK_EDGES)
				{
					unsigned int id = hit_name - SELECT_NAME_MANIPULATOR_LIMIT;
					// get selected object
					MeshPtr mesh = dynamic_cast<MeshPtr>(Project::instance()._selection);
					// find selected edge
					unsigned int edge_count = 0;
					for (; edge_count < mesh->edgeCount(); edge_count++)
					{
						HalfEdge * edge = mesh->edge(edge_count);
						if ((edge_count + 1) == id)
							edge->selected(!edge->selected());
						else
							edge->selected(false);
					}
				}
				else if (Window::instance()->selectMasks() == Window::SELECT_MASK_POINTS)
				{
					unsigned int id = hit_name - SELECT_NAME_MANIPULATOR_LIMIT;
					// get selected object
					MeshPtr mesh = dynamic_cast<MeshPtr>(Project::instance()._selection);
					// find selected vertex
					unsigned int vertex_count = 0;
					for (; vertex_count < mesh->vertexCount(); vertex_count++)
					{
						Vertex * vertex = mesh->vertex(vertex_count);
						if ((vertex_count + 1) == id)
							vertex->selected(!vertex->selected());
						else
							vertex->selected(false);
					}
				}
			}
		}
	}
	else
	{
		// deselect all objects
		select_object(-1);
	}
	// force a window redraw so selection changes will be immediately visible
	Window::instance()->redraw();
}

