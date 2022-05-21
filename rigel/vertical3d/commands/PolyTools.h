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
#ifndef INCLUDED_POLYTOOLS_H
#define INCLUDED_POLYTOOLS_H

#include <libv3dcommand/Tool.h>
#include <libv3dtypes/Vector3.h>

namespace v3D
{

	class SplitEdgeTool : public Tool
	{
		public:
			SplitEdgeTool();
			virtual ~SplitEdgeTool();

			virtual bool exec(const std::string & cmd);

			virtual bool motion(int x, int y);
			virtual bool button(unsigned int num, bool press, int x, int y);

			virtual bool draw(void);

		private:
			Vector3			_vertex;
			unsigned int	_edge;
			bool			_draw;
	};
	
	class SplitFaceTool : public Tool
	{
		public:
			SplitFaceTool();
			virtual ~SplitFaceTool();

			virtual bool exec(const std::string & cmd);
	};

	class ExtrudeFaceTool : public Tool
	{
		public:
			ExtrudeFaceTool();
			virtual ~ExtrudeFaceTool();

			virtual bool exec(const std::string & cmd);
	};

}; // end namespace v3D

#endif // INCLUDED_POLYTOOLS_H
