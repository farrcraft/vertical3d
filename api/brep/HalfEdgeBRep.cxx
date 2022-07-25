/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
 **/

#include <iostream>
#include <algorithm>
#include <cassert>

#include "HalfEdgeBRep.h"

using namespace v3D;

const unsigned int HalfEdgeBRep::INVALID_ID = (1 << 31);


HalfEdgeBRep::edge_iterator::edge_iterator() : _brep(0), _firstEdgeID(INVALID_ID), _edge(0)
{
}

HalfEdgeBRep::edge_iterator::edge_iterator(HalfEdgeBRep * brep, unsigned int faceID)
{
	reset(brep, faceID);
}

HalfEdgeBRep::edge_iterator::~edge_iterator()
{
}

void HalfEdgeBRep::edge_iterator::reset(HalfEdgeBRep * brep, unsigned int faceID)
{
	_brep = brep;
	Face * face = _brep->face(faceID);
	if (!face)
	{
		_edge = 0;
		return;
	}
	_firstEdgeID = face->edge();
	_edge = _brep->edge(_firstEdgeID);
}

HalfEdge * HalfEdgeBRep::edge_iterator::operator * ()
{
	return _edge;
}

HalfEdgeBRep::edge_iterator HalfEdgeBRep::edge_iterator::operator++ (int)
{
	if (!_edge)
		return *this;
	unsigned int nextEdgeID = _edge->next();
	if (nextEdgeID == _firstEdgeID)
	{
		_edge = 0;
		return *this;
	}
	_edge = _brep->edge(nextEdgeID);

	return *this;
}

HalfEdgeBRep * HalfEdgeBRep::edge_iterator::brep(void) const
{
	return _brep;
}

HalfEdgeBRep::vertex_iterator::vertex_iterator()
{
}

HalfEdgeBRep::vertex_iterator::vertex_iterator(HalfEdgeBRep * brep, unsigned int faceID)
{
	reset(brep, faceID);
}

HalfEdgeBRep::vertex_iterator::~vertex_iterator()
{
}

Vertex * HalfEdgeBRep::vertex_iterator::operator * ()
{
	if ((*_iterator) == 0)
		return 0;
	return (_iterator.brep()->vertex((*_iterator)->vertex()));
}

HalfEdgeBRep::vertex_iterator HalfEdgeBRep::vertex_iterator::operator++ (int)
{
	if ((*_iterator) == 0)
		return *this;
	_iterator++;
	return *this;
}

void HalfEdgeBRep::vertex_iterator::reset(HalfEdgeBRep * brep, unsigned int faceID)
{
	_iterator.reset(brep, faceID);
}


HalfEdgeBRep::HalfEdgeBRep() : _selected(false)
{
}

HalfEdgeBRep::~HalfEdgeBRep()
{
}

Vector3 HalfEdgeBRep::center(unsigned int faceID)
{
	float nverts = 0.0;
	Vector3 mid(0.0, 0.0, 0.0);

	vertex_iterator it(this, faceID);
	Vertex * vert;
	// get edge vertices
	Vector3 pt;
	for (; *it != 0; it++)
	{
		vert = *it;
		pt = vert->point();
		mid += pt;
		nverts += 1.0;
	}

	mid /= nverts;

	return mid;
}

void HalfEdgeBRep::faceUV(unsigned int faceID, Vector3 & u, Vector3 & v)
{
	edge_iterator it(this, faceID);
	if (*it == 0)
		return;

	HalfEdge * edge = *it;
	if (!edge)
		return;

	Vector3 norm;

	unsigned int vert = edge->vertex();
	assert(vert != INVALID_ID);
	unsigned int pair = edge->next();//edge->pair();
	assert(pair != INVALID_ID);
	unsigned int pair_vert = _edges[pair].vertex();
	assert(pair_vert != INVALID_ID);
	u = vertex(vert)->point() - vertex(pair_vert)->point();
	it++;
	edge = *it;
	assert(edge != 0);
	vert = edge->vertex();
	assert(vert != INVALID_ID);
	pair = edge->next();//edge->pair();
	assert(pair != INVALID_ID);
	pair_vert = _edges[pair].vertex();
	assert(pair_vert != INVALID_ID);
	v = vertex(vert)->point() - vertex(pair_vert)->point();

	norm = u.cross(v);
	norm.normalize();
	v = u.cross(norm);
	u = v.cross(norm);
	u.normalize();
	v.normalize();
}

unsigned int HalfEdgeBRep::addVertex(const Vector3 & v)
{
	unsigned int index = 0;
	for (; index < _vertices.size(); index++)
	{
		if (_vertices[index] == v)
			return index;
	}
	Vertex vert(v);
	_vertices.push_back(vert);
	return (_vertices.size() - 1);
}

unsigned int HalfEdgeBRep::addEdge(unsigned int vertexID)
{
	HalfEdge e(vertexID);
	/*
	unsigned int index = 0;
	for (; index < _edges.size(); index++)
	{
		if (_edges[index] == e)
		{
			return index;
		}
	}
	*/
	_edges.push_back(e);
	return (_edges.size() - 1);
}

void HalfEdgeBRep::addFace(const std::vector<Vector3> & vertices, const Vector3 & normal)
{
	std::cerr << "adding face" << std::endl;
	// add vertices
	std::vector<unsigned int> indices;
	unsigned int vertexID;
	std::vector<Vector3>::const_iterator it = vertices.begin();
	for (; it != vertices.end(); it++)
	{
		vertexID = addVertex(*it);
		std::cerr << "vertex " << vertexID << " = " << *it << std::endl;
		indices.push_back(vertexID);
	}

	// add edges
	unsigned int index = 0;
	unsigned int edgeID;
	/*
	unsigned int firstEdgeID = INVALID_ID;
	unsigned int lastEdgeID = INVALID_ID;
	*/
	std::vector<unsigned int> edges;
	for (; index < indices.size(); index++)
	{
		edgeID = addEdge(indices[index]);
		std::cerr << "edge " << edgeID << " = " << indices[index] << std::endl;
		/*
		if (index != 0)
			_edges[edgeID].pair(indices[index - 1]);
		if (lastEdgeID != INVALID_ID)
			_edges[lastEdgeID].next(edgeID);

		if (index == (indices.size() - 1))
		{
			_edges[firstEdgeID].pair(edgeID);
			_edges[edgeID].next(firstEdgeID);
		}

		if (firstEdgeID == INVALID_ID)
			firstEdgeID = edgeID;
		lastEdgeID = edgeID;
		*/
		edges.push_back(edgeID);
	}

	// add face
	unsigned int faceID;
	Face f(normal, edges[0]);
	_faces.push_back(f);
	faceID = (_faces.size() - 1);

	std::cerr << "building edges" << std::endl;
	// finish constructing edges
	HalfEdge * ep;

	for (index = 0; index < edges.size(); index++)
	{
		ep = edge(edges[index]);
		assert(ep != 0);
		ep->face(faceID);
		unsigned int next;
		if (index != edges.size() - 1)
			next = edges[index + 1];
		else
			next = edges[0];
		ep->next(next);
	}

	for (index = 0; index < edges.size(); index++)
	{
		ep = edge(edges[index]);
		assert(ep != 0);
		unsigned int pair = INVALID_ID;
		if (index != 0)
			pair = findPair(edges[index], edges[index - 1]);
		else
			pair = findPair(edges[index], edges[edges.size() - 1]);
		//assert(pair != INVALID_ID);
		if (pair != INVALID_ID)
		{
			ep->pair(pair);
			edge(pair)->pair(edges[index]);
		}
		std::cerr << "edge " << edges[index] << " pair = " << pair << std::endl;
	}
	std::cerr << "done building face" << std::endl;
}

unsigned int HalfEdgeBRep::findPair(unsigned int edgeID, unsigned int prevEdgeID)
{
	std::cerr << "finding pair for " << edgeID << ", " << prevEdgeID << std::endl;
	// this edge's pair points to the previous edge/vertex in this face
	// find the edge that points to the previous edge's vertex
	// and whose next edge points to this edge's vertex

	unsigned int index = 0;
	// pairs never share the same face
	unsigned int face = _edges[edgeID].face();
	for (; index < _edges.size(); index++)
	{
		if (_edges[index].face() != face &&
			_edges[index].vertex() == _edges[prevEdgeID].vertex())
		{
			// find potential pair's prev edge
			unsigned int pair_prev = _edges[index].next();
			assert(pair_prev != INVALID_ID);
			for (; _edges[pair_prev].next() != index; )
			{
				pair_prev = _edges[pair_prev].next();
				assert(pair_prev != INVALID_ID);
			}
			std::cerr << "prev pair = " << pair_prev << std::endl;
			if (_edges[pair_prev].vertex() == _edges[edgeID].vertex())
				return index;
		}
	/* #2
		if (_edges[index].face() != face &&
			_edges[index].vertex() == _edges[edgeID].vertex())
		{
			unsigned int next = _edges[index].next();
			assert(next != INVALID_ID);
			if (_edges[next].vertex() == _edges[prevEdgeID].vertex())
				return index;
		}
	*/
		/* #1
		if (index != edgeID && index != prevEdgeID &&
			_edges[index].vertex() == _edges[prevEdgeID].vertex())
		{
			if (_edges[index].next() != INVALID_ID)
			{
				if (_edges[_edges[index].next()].vertex() == _edges[edgeID].vertex())
					return index;
			}
		}
		*/
	}
	return INVALID_ID;
}

HalfEdge * HalfEdgeBRep::edge(unsigned int edgeID)
{
	if (edgeID < _edges.size())
		return &_edges[edgeID];
	return 0;
}

Face * HalfEdgeBRep::face(unsigned int faceID)
{
	if (faceID < _faces.size())
		return &_faces[faceID];
	return 0;
}

Vertex * HalfEdgeBRep::vertex(unsigned int vertexID)
{
	if (vertexID < _vertices.size())
		return &_vertices[vertexID];
	return 0;
}

unsigned int HalfEdgeBRep::vertexCount(void) const
{
	return _vertices.size();
}

unsigned int HalfEdgeBRep::edgeCount(void) const
{
	return _edges.size();
}

unsigned int HalfEdgeBRep::faceCount(void) const
{
	return _faces.size();
}

unsigned int HalfEdgeBRep::addVertex(const Vertex & v)
{
	_vertices.push_back(v);
	return (_vertices.size() - 1);
}

unsigned int HalfEdgeBRep::addEdge(const HalfEdge & e)
{
	_edges.push_back(e);
	return (_edges.size() - 1);
}

unsigned int HalfEdgeBRep::addFace(const Face & f)
{
	_faces.push_back(f);
	return (_faces.size() - 1);
}

void HalfEdgeBRep::splitEdge(unsigned int edgeID, const Vector3 & point)
{
	unsigned int vertexID;
	_vertices.push_back(point);
	vertexID = _vertices.size() - 1;

	HalfEdge newEdge = _edges[edgeID];
	// edge goes from PVT to point
	// newEdge goes from point to NVT
	newEdge.vertex(vertexID);

	unsigned newEdgeID;
	_edges.push_back(newEdge);
	newEdgeID = _edges.size() - 1;

	newEdge.pair(edgeID);
	newEdge.next(_edges[edgeID].next());
	_edges[edgeID].next(newEdgeID);
}

// calculate object-space bounds of mesh
AABBox HalfEdgeBRep::bound(void) const
{
	AABBox extents;
	if (_vertices.size() == 0)
		return extents;
	Vector3 min, max;
	min = _vertices[0].point();
	max = min;
	Vector3 vt;
	for (unsigned int index = 1; index < _vertices.size(); index++)
	{
		vt = _vertices[index].point();
		if (vt[0] < min[0])
			min[0] = vt[0];
		if (vt[1] < min[1])
			min[1] = vt[1];
		if (vt[2] < min[2])
			min[2] = vt[2];

		if (vt[0] > max[0])
			max[0] = vt[0];
		if (vt[1] > max[1])
			max[1] = vt[1];
		if (vt[2] > max[2])
			max[2] = vt[2];
	}
	extents.extents(min, max);
	return extents;
}

bool HalfEdgeBRep::selected(void) const
{
	return _selected;
}

void HalfEdgeBRep::selected(bool sel)
{
	_selected = sel;
	if (!_selected)
	{
		// make sure components are unselected also
		unsigned int vertex_count = 0;
		for (; vertex_count < _vertices.size(); vertex_count++)
		{
			_vertices[vertex_count].selected(false);
		}
		unsigned int face_count = 0;
		for (; face_count < _faces.size(); face_count++)
		{
			_faces[face_count].selected(false);
		}
		unsigned int edge_count = 0;
		for (; edge_count < _edges.size(); edge_count++)
		{
			_edges[edge_count].selected(false);
		}
	}
}

Vector3 HalfEdgeBRep::translation(void) const
{
	return DAG::Transform::translation();
}

void HalfEdgeBRep::translation(const Vector3 & t)
{
	bool component_translate = false;

	// translate selected vertices
	unsigned int vertex_count = 0;
	for (; vertex_count < _vertices.size(); vertex_count++)
	{
		if (_vertices[vertex_count].selected())
		{
			component_translate = true;
			_vertices[vertex_count].point(_vertices[vertex_count].point() + t);
		}
	}

	// translate selected faces
	unsigned int face_count = 0;
	for (; face_count < _faces.size(); face_count++)
	{
		if (_faces[face_count].selected())
		{
			component_translate = true;
			vertex_iterator it(this, face_count);
			for (; *it != 0; it++)
			{
				Vertex * vert = *it;
				vert->point(vert->point() + t);
			}
		}
	}

	// translate selected edges
	unsigned int edge_count = 0;
	for (; edge_count < _edges.size(); edge_count++)
	{
		if (_edges[edge_count].selected())
		{
			component_translate = true;
			_vertices[_edges[edge_count].vertex()].point(_vertices[_edges[edge_count].vertex()].point() + t);
			_vertices[_edges[edge_count].vertex()].point(_vertices[_edges[edge_count].vertex()].point() + t);
		}
	}

	// translate object
	if (!component_translate)
		DAG::Transform::translation(t);
}
