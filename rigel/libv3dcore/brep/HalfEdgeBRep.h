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
#include "HalfEdge.h"
#include "Face.h"

namespace v3D
{

	class HalfEdgeBRep : public DAG::Node, public DAG::Transform
	{
		public:
			HalfEdgeBRep();
			~HalfEdgeBRep();

			static const unsigned int INVALID_ID;

			class edge_iterator
			{
				public:
					edge_iterator();
					edge_iterator(HalfEdgeBRep * brep, unsigned int faceID);
					~edge_iterator();
		
					HalfEdge * operator * ();
					edge_iterator operator++ (int);
		
					void reset(HalfEdgeBRep * brep, unsigned int faceID);
					HalfEdgeBRep * brep(void) const;

				private:
					HalfEdge *		_edge;
					unsigned int	_firstEdgeID;
					HalfEdgeBRep *	_brep;
			};

			class vertex_iterator
			{
				public:
					vertex_iterator();
					vertex_iterator(HalfEdgeBRep * brep, unsigned int faceID);
					~vertex_iterator();

					Vertex * operator * ();
					vertex_iterator operator++ (int);

					void reset(HalfEdgeBRep * brep, unsigned int faceID);

				private:
					edge_iterator	_iterator;
			};

			HalfEdge * edge(unsigned int edgeID);
			Face * face(unsigned int faceID);
			Vertex * vertex(unsigned int vertexID);

			AABBox bound(void) const;

			bool selected(void) const;
			void selected(bool sel);

			void addFace(const std::vector<Vector3> & vertices, const Vector3 & normal);
			void addEdge(const Vector3 & point);

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
			unsigned int addEdge(const HalfEdge & e);
			unsigned int addFace(const Face & f);

		protected:
			unsigned int addVertex(const Vector3 & v);
			unsigned int addEdge(unsigned int vertexID);
			unsigned int findPair(unsigned int edgeID, unsigned int prevEdgeID);

		private:
			std::vector<Vertex>		_vertices;
			std::vector<Face>		_faces;
			std::vector<HalfEdge>	_edges;
			bool					_selected;
	};

	typedef HalfEdgeBRep Mesh;
	typedef Mesh * MeshPtr;

}; // end namespace v3D
