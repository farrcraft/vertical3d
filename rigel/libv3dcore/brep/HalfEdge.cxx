/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
 **/

#include "HalfEdge.h"

using namespace v3D;

const unsigned int INVALID_ID = (1 << 31);

HalfEdge::HalfEdge() : _vertexID(INVALID_ID), _pairID(INVALID_ID), _nextID(INVALID_ID), _faceID(INVALID_ID)
{
}

HalfEdge::HalfEdge(unsigned int vertexID) : _vertexID(vertexID), _pairID(INVALID_ID), _nextID(INVALID_ID), _faceID(INVALID_ID)
{
}

HalfEdge::HalfEdge(const HalfEdge & e)
{
	*this = e;
}

HalfEdge::~HalfEdge()
{
}

bool HalfEdge::operator == (const HalfEdge & e)
{
	return (_vertexID = e._vertexID && 
			_faceID == e._faceID &&
			_nextID == e._nextID &&
			_pairID == e._pairID);
}

HalfEdge & HalfEdge::operator = (const HalfEdge & e)
{
	_selected = e._selected;
	_vertexID = e._vertexID;
	_faceID = e._faceID;
	_nextID = e._nextID;
	_pairID = e._pairID;

	return *this;
}

bool HalfEdge::selected(void) const
{
	return _selected;
}

void HalfEdge::selected(bool sel)
{
	_selected = sel;
}

unsigned int HalfEdge::vertex(void) const
{
	return _vertexID;
}

unsigned int HalfEdge::face(void) const
{
	return _faceID;
}

unsigned int HalfEdge::pair(void) const
{
	return _pairID;
}

unsigned int HalfEdge::next(void) const
{
	return _nextID;
}

void HalfEdge::vertex(unsigned int vertexID)
{
	_vertexID = vertexID;
}

void HalfEdge::face(unsigned int faceID)
{
	_faceID = faceID;
}

void HalfEdge::pair(unsigned int edgeID)
{
	_pairID = edgeID;
}

void HalfEdge::next(unsigned int edgeID)
{
	_nextID = edgeID;
}
