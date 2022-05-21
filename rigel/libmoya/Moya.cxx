/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
 **/

#include <iostream>

#include "Moya.h"
// #include "RibWriter.h"

using namespace v3D::Moya;


RenderEngine::RenderEngine() : _bucketWidth(16), _bucketHeight(16), _gridSize(256), _shadingRate(1.0)
{
}

RenderEngine::~RenderEngine()
{
}

RenderEngine & RenderEngine::instance(void)
{
	static RenderEngine instance;
	return instance;
}

unsigned int RenderEngine::bucketWidth(void) const
{
	return _bucketWidth;
}

unsigned int RenderEngine::bucketHeight(void) const
{
	return _bucketHeight;
}

unsigned int RenderEngine::gridSize(void) const
{
	return _gridSize;
}

float RenderEngine::shadingRate(void) const
{
	return _shadingRate;
}

/*
	maps to RiBegin()
*/
void RenderEngine::createRenderContext(std::string name)
{
	// should probably go somewhere else since it could be displayed multiple times here...
	if (_contexts.size() == 0)
		std::cout << "The RenderMan (R) Interface Procedures and Protocol are:" << std::endl <<
					 "Copyright 1988, 1989, Pixar" << std::endl << 
					 "All Rights Reserved" << std::endl;

	RenderContext context(name);
	_contexts.push_back(context);
}

void RenderEngine::destroyActiveRenderContext(void)
{
	_contexts.pop_back();
}

RenderContext & RenderEngine::activeRenderContext(void)
{
	if (_contexts.size() == 0)
		createRenderContext("");
	return _contexts[_contexts.size() - 1];
}

/*
	maps to RiWorldBegin()
	freezes all rendering options, world to camera transformation 
	is set to current transormation, current transformation is set 
	to identity
*/
void RenderEngine::prepareWorld(void)
{
	// do we always need to do this or only when a projection hasn't explicitly been set?
	activeRenderContext().projection("");

	// establish the world coordinate system
	// save the existing transform as the camera coordinate system
	activeRenderContext().saveCoordinateSystem("camera");

	std::cerr << "RenderEngine::prepareWorld - camera coordinate system marked." << std::endl;

	activeRenderContext().prepareWorld();

	std::cerr << "RenderEngine::prepareWorld - view options frozen." << std::endl;
}

/*
	maps to RiWorldEnd()
	once rendering is done, objects, lights and other stuff set
	after prepareWorld() are destroyed and the memory reclaimed
	saving the output file is done here also.
	the results might not even include a rendered image depending
	on the context. we might just output a rib file.
*/
void RenderEngine::render(void)
{
	// if output stream exists
	// echo RiWorldEnd RIB command to output stream

	std::cerr << "RenderEngine::render - begin rendering active context." << std::endl;

	// do reyes second pass
	activeRenderContext().render();

	std::cerr << "RenderEngine::render - finished rendering active context." << std::endl;

	// write out to RIB
	/*
	RibWriter writer;
	writer.write(getActiveRenderContext());
	*/

	// render each polygon in the current context
	/*
	for (unsigned int i = 0; i < getActiveRenderContext().getPolygonCount(); i++)
  {
		renderPolygon(getActiveRenderContext().getPolygon(i));
  }	
  */
}

/*
	coordinate systems:
		"object"		coordinate system current primitive is defined in.
						modeling transformation converts from object to world coordinates.
		"world"			standard reference coordinate system. camera transformation
						converts from world to camera coordinates.
		"camera"		coordinate system with the vantage point at origin and direction
						of view along positive z-axis. projection and screen transformation
						convert from camera to screen coordinates
		"screen"		2-d normalized coordinate system corresponding to image plane.
						raster transformation converts to raster coordinates.
		"raster"		raster or pixel coordinate system. are of 1 in coordinate system
						corresponds to area of single pixel. coordinate system is inherited
						from display or set by selecting resolution of desired image.
		"NDC"			normalized device coordinates. like "raster" but normalized so
						x and y both run from 0 to 1 across whole (uncropped) image,
						with (0,0) at upper left of image and (1,1) at lower right (regardless
						of actual aspect ratio).

	default projection is orthographic.
	coordinates are mapped left-handed - +x points right, +y points up, +z points inward.
	
	- before any transformations, the current transformation matrix contains the identity matrix
	as the screen transformation.
	- usually the first transformation command is RiProjection, which appends the projection matrix
	onto the screen transformation, saves it, and reinitializes the current transformation matrix
	as the identity camera transformation.
	- after the camera coordinate system is established, future transformations move the world
	coordinate system relative to the camera coordinate system. 
	- when RiWorldBegin is called, the current transformation matrix is saved as the camera 
	transformation.
	- subsequent transformations inside RiWorldBegin-RiWorldEnd establish different object coordinate
	systems.
	
	culling math:
		plane equation:
			A*x + B*y + C*z + D = 0
		point (px, py, pz) distance to plane equation:
			d = A *px + B*py + C*pz + D
			
*/
void RenderEngine::renderPolygon(const PolygonPtr & poly)
{
	/*
		transform to screen/camera space
	*/
	// is polygon within the view frustum?
	// is the polygon facing the camera?
	// cull polygon completely or clip to view frustum

}

/*
void Moya::setImageResolution(int xres, int yres, float aspect)
{
	assert(_contexts.size() > 0);
	_contexts[_contexts.size() - 1]._xres = xres;
	_contexts[_contexts.size() - 1]._yres = yres;
	_contexts[_contexts.size() - 1]._pixelAspect = aspect;
}

void Moya::setClipping(float near, float far)
{
}
*/
void RenderEngine::setCurrentColor(Color3 color)
{
}

void RenderEngine::destroyWorld(void)
{
}
/*
void Moya::addPolygon(polygon poly)
{
	_contexts[_contexts.size() - 1].getPolygonList().push_back(poly);
}
*/
