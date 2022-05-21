#include "MicroPolygonGrid.h"

using namespace v3D;
using namespace v3D::Moya;

MicroPolygonGrid::MicroPolygonGrid()
{
}

MicroPolygonGrid::~MicroPolygonGrid()
{
}

Vertex MicroPolygonGrid::vertex(unsigned int i, unsigned int j) const
{
	return _grid[i][j];
}

MicroPolygon MicroPolygonGrid::microPolygon(unsigned int i, unsigned int j) const
{
	/*
		poly is composed of points on grid:
		[i][j]
		[i][j+1]
		[i+1][j+1]
		[i+1][j]
	*/
	MicroPolygon p;
	p[0] = _grid[i][j];
	p[1] = _grid[i][j+1];
	p[2] = _grid[i+1][j+1];
	p[3] = _grid[i+1][j];
	return p;
}

void MicroPolygonGrid::addVertex(const Vertex & vert, unsigned int i, unsigned int j)
{
	_grid[i][j] = vert;
}
