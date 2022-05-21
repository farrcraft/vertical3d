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
#ifndef INCLUDED_V3D_VIEWLAYOUT_H
#define INCLUDED_V3D_VIEWLAYOUT_H

#include <libxml++/parsers/domparser.h>

#include "ViewPort.h"

namespace v3D
{

	class ViewLayout
	{
		public:
			ViewLayout();
			~ViewLayout();

			typedef enum LayoutType
			{
				QUAD_VIEW,
				DOUBLE_HORIZ_VIEW,
				DOUBLE_VERT_VIEW,
				TRIPLE_HORIZ_LEFT_SPLIT,
				TRIPLE_HORIZ_RIGHT_SPLIT,
				TRIPLE_VERT_TOP_SPLIT,
				TRIPLE_VERT_BOTTOM_SPLIT,
				TRIPLE_HORIZ_SPLIT,
				TRIPLE_VERT_SPLIT,
				SINGLE_VIEW
			} LayoutType;

			Gtk::Widget * build(xmlpp::Element * elem);
			Gtk::Widget * build_children(xmlpp::Element * elem);
			void invalidate(void);
			void pop(ViewPort * active_view);


			std::string					_font;
			std::vector<ViewPort *>		_views;

		private:
			std::vector<Gtk::Paned *>	_panes;
			LayoutType					_layout;
			unsigned int				_width;
			unsigned int				_height;
			Glib::RefPtr<Gdk::GL::Config> _glconfig;
	};

}; // end namespace v3D

#endif // INCLUDED_V3D_VIEWLAYOUT_H
