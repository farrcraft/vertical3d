/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vector>

#include <libv3dtypes/AABBox.h>
#include <libv3dgraph/Node.h>
#include <libv3dgraph/Transform.h>

#include "Vertex.h"
#include "Edge.h"
#include "Face.h"

namespace v3D
{

	/*
		http://www.baumgart.org/winged-edge/winged-edge.html
		http://www.cs.mtu.edu/~shene/COURSES/cs3621/NOTES/model/winged-e.html

		winged edge data structure
		each edge stores:
			- two vertices
			- (two) adjacent (left & right) faces
			- neighboring edges (wings)

		each face stores:
			- the first incident edge

		edge vertex stores:
			- the first incident edge

		4 wings:
		top left		next ccw
		top right		prev cw
		bottom left		next cw
		bottom right	prev ccw

		left face		next face
		right face		prev face

		top vertex		previous vertex
		bottom vertex	next vertex
	*/

	class WingedEdgeBRep : public DAG::Node, public DAG::Transform
	{
		public:
			WingedEdgeBRep();
			~WingedEdgeBRep();

			static const unsigned int INVALID_ID;

			class edge_iterator
			{
				public:
					edge_iterator();
					edge_iterator(WingedEdgeBRep * brep, unsigned int faceID);
					~edge_iterator();
		
					Edge * operator * ();
					edge_iterator operator++ (int);
		
					void reset(WingedEdgeBRep * brep, unsigned int faceID);
					WingedEdgeBRep * brep(void) const;

				private:
					Edge *				_edge;
					unsigned int		_firstEdgeID;
					bool				_winding;
					WingedEdgeBRep *	_brep;
			};


			class vertex_iterator
			{
				public:
					vertex_iterator();
					vertex_iterator(WingedEdgeBRep * brep, unsigned int faceID);
					~vertex_iterator();

					Vertex * operator * ();
					vertex_iterator operator++ (int);

					void reset(WingedEdgeBRep * brep, unsigned int faceID);

				private:
					edge_iterator	_iterator;
					bool			_nextVertex;
			};


			Edge * edge(unsigned int edgeID);
			Face * face(unsigned int faceID);
			Vertex * vertex(unsigned int vertexID);

			AABBox bound(void) const;

			bool selected(void) const;
			void selected(bool sel);

			/*
				winding == true - ccw - face on left side of first edge
						  false - cw  - face on right side of first edge
			*/
			void addFace(const std::vector<Vector3> & vertices, const Vector3 & normal, bool winding);
			void addEdge(const Vector3 & leftPoint, const Vector3 & rightPoint);

			void splitEdge(unsigned int edgeID, const Vector3 & point);
			void extrudeFace(unsigned int faceID);
			void splitFace(unsigned int faceID, unsigned int leftEdgeID, unsigned int rightEdgeID, const Vector3 & leftPoint, const Vector3 & rightPoint);

			Vector3 center(unsigned int faceID);
			void faceUV(unsigned int faceID, Vector3 & u, Vector3 & v);

			unsigned int vertexCount(void) const;
			unsigned int edgeCount(void) const;
			unsigned int faceCount(void) const;

			// derived from DAG::Transform
			virtual void translation(const Vector3 & t);
			virtual Vector3 translation(void) const;

			unsigned int addVertex(const Vertex & v);
			unsigned int addEdge(const Edge & e);
			unsigned int addFace(const Face & f);

		protected:
			unsigned int addVertex(const Vector3 & v);
			unsigned int addEdge(unsigned int leftVertex, unsigned int rightVertex);

		private:
			std::vector<Vertex>	_vertices;
			std::vector<Face>	_faces;
			std::vector<Edge>	_edges;
			bool				_selected;
	};

	typedef WingedEdgeBRep Mesh;
	typedef Mesh * MeshPtr;

}; // end namespace v3D\
