/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

namespace v3D
{

	class HalfEdge
	{
		public:
			HalfEdge();
			HalfEdge(unsigned int vertexID);
			HalfEdge(const HalfEdge & e);
			~HalfEdge();

			bool operator == (const HalfEdge & e);
			HalfEdge & operator = (const HalfEdge & e);

			bool selected(void) const;
			void selected(bool sel);

			unsigned int vertex(void) const;
			unsigned int face(void) const;
			unsigned int pair(void) const;
			unsigned int next(void) const;

			void vertex(unsigned int vertexID);
			void face(unsigned int faceID);
			void pair(unsigned int edgeID);
			void next(unsigned int edgeID);

		private:
			unsigned int	_vertexID;	// vertex at end of half edge
			unsigned int	_faceID;	// face to left of edge
			unsigned int	_pairID;	// symetric half edge
			unsigned int	_nextID;	// next CCW half edge 
			bool			_selected;
	};

}; // end namespace v3D
