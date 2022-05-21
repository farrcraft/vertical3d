#include "Renderer.h"

#include <iostream>

using namespace v3D::Moya;


Renderer::Renderer()
{
}

Renderer::~Renderer()
{
}

/**
 * Establish a new rendering context.
 * This maps to RiBegin()
*/
void Renderer::createRenderContext(const std::string & name)
{
	// should probably go somewhere else since it could be displayed multiple times here...
	if (_contexts.size() == 0)
	{
		std::cout << "The RenderMan (R) Interface Procedures and Protocol are:" << std::endl <<
					 "Copyright 1988, 1989, Pixar" << std::endl << 
					 "All Rights Reserved" << std::endl;
	}

	RenderContext context(name);
	_contexts.push_back(context);
}

void Renderer::destroyActiveRenderContext(void)
{
	_contexts.pop_back();
}

RenderContext & Renderer::activeRenderContext(void)
{
	if (_contexts.size() == 0)
	{
		createRenderContext("");
	}
	return _contexts[_contexts.size() - 1];
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
/*
void Renderer::renderPolygon(boost::shared_ptr<Polygon> poly)
{
*/
	/*
		transform to screen/camera space
	*/
	// is polygon within the view frustum?
	// is the polygon facing the camera?
	// cull polygon completely or clip to view frustum

//}

