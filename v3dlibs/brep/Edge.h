/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

namespace v3D
{

	class Edge
	{
		public:
			Edge();
			Edge(const Edge & e);
			Edge(unsigned int prevVertexID, unsigned int nextVertexID);
			~Edge();

			bool operator == (const Edge & e);
			Edge & operator = (const Edge & e);

			bool selected(void) const;
			void selected(bool sel);

			unsigned int prevVertex(void) const;
			unsigned int nextVertex(void) const;
			unsigned int prevFace(void) const;
			unsigned int nextFace(void) const;
			unsigned int prevCWEdge(void) const;
			unsigned int nextCWEdge(void) const;
			unsigned int prevCCWEdge(void) const;
			unsigned int nextCCWEdge(void) const;
			void prevVertex(unsigned int vertexID);
			void nextVertex(unsigned int vertexID);
			void prevFace(unsigned int faceID);
			void nextFace(unsigned int faceID);
			void prevCWEdge(unsigned int edgeID);
			void nextCWEdge(unsigned int edgeID);
			void prevCCWEdge(unsigned int edgeID);
			void nextCCWEdge(unsigned int edgeID);

		private:
			unsigned int		_prevVertexID;	// vertices
			unsigned int 		_nextVertexID;
			unsigned int		_prevFaceID;	// adjacent faces
			unsigned int		_nextFaceID;
			unsigned int		_prevCWEdgeID;	// wings
			unsigned int		_nextCWEdgeID;
			unsigned int		_prevCCWEdgeID;
			unsigned int		_nextCCWEdgeID;
			bool				_selected;
	};

}; // end namespace v3D
