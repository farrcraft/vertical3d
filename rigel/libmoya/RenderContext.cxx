/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
 **/

#include <iostream>
#include <cmath>

#include <libv3dtypes/AABBox.h>

#include "RenderContext.h"
#include "Moya.h"
#include "Frustum.h"

using namespace v3D;
using namespace v3D::Moya;

RenderContext::RenderContext() : _xres(320), _yres(240), _pixelAspect(1.0), 
				_projection("orthographic"), _shutterOpen(0.0), _shutterClose(0.0)
{
	initialize();
}

RenderContext::RenderContext(const std::string & name) : _name(name), _xres(320), _yres(240), _pixelAspect(1.0), 
				_projection("orthographic"), _shutterOpen(0.0), _shutterClose(0.0)
{
	initialize();
}

RenderContext::~RenderContext()
{
}

void RenderContext::initialize(void)
{
	// initialize the predefined coordinate systems to defaults (identity matrix)
	Matrix4 def;
	def.identity();
	_coordinateSystems["object"] = def;
	_coordinateSystems["world"]  = def;
	_coordinateSystems["camera"] = def;
	_coordinateSystems["screen"] = def;
	_coordinateSystems["raster"] = def;
	_coordinateSystems["NDC"] 	 = def;

	_transform.identity();

	// initialize the z buffer

	// initialize buckets - done in prepareWorld()
}

/*
called from RenderEngine::prepareWorld()
corresponds to RiWorldBegin()
the view options are frozen beyond this call
the framebuffer will be allocated here
*/
void RenderContext::prepareWorld(void)
{
	// set raster transformation
	Matrix4 raster;
	raster.identity();
	/*
		screen transform maps to the canonical volume [-1, 1]
		raster transform scales to [0, xres] x [0, yres]
	*/
	real_t x = _xres / 2.0;
	real_t y = _yres / 2.0;
	raster.scale(x, y, 1.0);
	raster.translate(1.0, 1.0, 0.0);
	// mark the raster coordinate system
	_coordinateSystems["raster"] = raster;


	unsigned int screen[2];
	screen[0] = _xres;
	screen[1] = _yres;
	unsigned int bucket[2];
	bucket[0] = RenderEngine::instance().bucketWidth();
	bucket[1] = RenderEngine::instance().bucketHeight();

	std::cerr << "RenderContext::prepareWorld - allocating framebuffer. image size = [" << screen[0] << " x " << screen[1] << "]. bucket size = [" << bucket[0] << " x " << bucket[1] << "]" << std::endl;

	_frameBuffer = new FrameBuffer(bucket, screen);
}

unsigned int RenderContext::imageWidth(void) const
{
	return _xres;
}

unsigned int RenderContext::imageHeight(void) const
{
	return _yres;
}

float RenderContext::pixelAspect(void) const
{
	return _pixelAspect;
}

/*
set a named projection transformation matrix
the combination of the projection and screen transformation matrices
move between camera and screen coordinate space
*/
void RenderContext::projection(std::string name, float fov)
{
	if (name.size() == 0)
	{
		name = "orthographic";
	}

	// name is orthographic, perspective, or empty
	// only perspective uses fov
	_projection = name;

	Matrix4 projection;
	// build the projection matrix
	if (name == "perspective")
	{
		projection.identity();
	}
	else if (name == "orthographic")
	{
		std::cerr << "RenderContext::projection - enabling orthographic projection:" << std::endl;
		/*
			orthographic projection looks something like:

			[ 2/(xright - xleft)	0						0					-((xright + xleft)/(xright-xleft))	]
			[ 		0				2/(ytop - ybottom)		0					-((ytop+ybottom)/(ytop-ybottom))	]
			[		0				0						-2/(zfront-zback)	(zfront+zback)/(zfront-zback)		]
			[		0				0						0					1									]

			this is almost the same as used in v3D::Core::Camera::createProjection()
			the only difference is that tx and ty are negated.
		*/

		/*
			[2 / (right-left)	0					0				-tx	]
			[0					2 / (bottom-top)	0				-ty	]
			[0					0					2/(far-near)	-tz	]
			[0					0					0				1	]

			tx = (right + left) / (right - left)
			ty = (top + bottom) / (top - bottom)
			tz = (far + near) / (far - near)
		[  0,  1,  2,  3 ]
		[  4,  5,  6,  7 ]
		[  8,  9, 10, 11 ]
		[ 12, 13, 14, 15 ]

		m[0] = 2 / screen window width
		m[5] = 2 / screen window height
		*/

		real_t left = -1.0;
		real_t right = 1.0; //_xres;
		real_t top = 1.0;
		real_t bottom = -1.0; //_yres;
		real_t far = _far;
		real_t near = _near;

		real_t tx = ((right + left) / (right - left));
		real_t ty = ((top + bottom) / (top - bottom));
		real_t tz = (far + near) / (far - near);

		projection[0]  = 2.0 / (right - left);
		projection[1]  = 0.0;
		projection[2]  = 0.0;
		projection[3]  = -tx;
		projection[4]  = 0.0;
		projection[5]  = 2.0 / (top - bottom);
		projection[6]  = 0.0;
		projection[7]  = -ty;
		projection[8]  = 0.0;
		projection[9]  = 0.0;
		projection[10] = -2.0 / (far - near);
		projection[11] = -tz;
		projection[12] = 0.0;
		projection[13] = 0.0;
		projection[14] = 0.0;
		projection[15] = 1.0;

		std::cerr << projection;
	}
	else // implementation-specific projection
	{
		// unsupported projections default to orthographic
	}

	// set transform to screen transformation
	

	// append projection matrix to current transformation matrix
	_transform *= projection;
	// save as screen coordinate system
	saveCoordinateSystem("screen");
	std::cerr << "marked system = " << std::endl << _coordinateSystems["screen"];
	// reinitialize current transformation to indentity matrix
	_transform.identity();
	// current transformation matrix is now the camera coordinate system
}

/*
	maps to RiFormat(xres, yres, aspect)
	sets the pixel resolution and aspect ratio of the image to be rendered
	default values will be used when not called
*/
void RenderContext::imageResolution(int xres, int yres, float aspect)
{
	_xres = xres;
	_yres = yres;
	_pixelAspect = aspect;
}

/*
	maps to RiClipping(hither, yon)
	sets the position of the near and far clipping planes
*/
void RenderContext::clipping(float near, float far)
{
	_near = near;
	_far = far;
}

void RenderContext::pushTransform(void)
{
	_transforms.push_back(_transform);
}

void RenderContext::popTransform(void)
{
	_transforms.pop_back();
}

void RenderContext::saveCoordinateSystem(const std::string & name)
{
	_coordinateSystems[name] = _transform;
}

void RenderContext::setCoordinateSystem(const std::string & name)
{
	// make sure name is a valid coordinate system

	_transform = _coordinateSystems[name];
}

void RenderContext::setIdentityTransform(void)
{
	_transform.identity();
}

void RenderContext::setTransform(const Matrix4 & trans)
{
	 _transform = trans;
}

Matrix4 RenderContext::coordinateSystem(const std::string & name)
{
	return _coordinateSystems[name];
}

void RenderContext::translate(float dx, float dy, float dz)
{
	_transform.translate(dx, dy, dz);
}

void RenderContext::rotate(float angle, float dx, float dy, float dz)
{
}

void RenderContext::scale(float sx, float sy, float sz)
{
}

/*
	maps to RiPolygon()
	this covers the initial pass of the reyes architecture
*/
void RenderContext::addPolygon(const PolygonPtr & poly)
{
	// if an output stream exists
	// echo RiPolygon RIB command to output stream

//	_polygons.push_back(poly);

	// bound polygon in eye space
	/* 
		if this is just based off of poly vertices, the bound will be in object space
		we'll need to apply the current modeling transformation to get from object space to world space
		and then the camera transformation will need to be applied to get into eye space
		we should probably just transform the poly to eye space first since any future calculations
		on this poly will be done in eye space or beyond.
	*/
	AABBox bound = poly->bound();

	Vector3 bound_max = bound.max();
	Vector3 bound_min = bound.min();

	std::cerr << "RenderContext::addPolygon - object space bounds are " << bound_min << " x " << bound_max << std::endl;
	/*
		convert bound to eye space coordinates
		first multiply by modeling transformation to get world coordinates
		next multiply by camera transformation to get camera/eye coordinates

		for now we're just taking the current transform.
		later we'll probably need to concatenate the _transforms matrix stack too
	*/
	std::cerr << "RenderContext::addPolygon - current transform matrix: " << std::endl << _transform;
	std::cerr << "RenderContext::addPolygon - camera transform matrix: " << std::endl << _coordinateSystems["camera"];
	bound_max = _transform * bound_max;
	bound_max = _coordinateSystems["camera"].transpose() * bound_max;
	bound_min = _transform * bound_min;
	bound_min = _coordinateSystems["camera"].transpose() * bound_min;

	// camera transform might've flipped some components of min & max
	if (bound_min[0] > bound_max[0])
		swap(bound_min[0], bound_max[0]);
	if (bound_min[1] > bound_max[1])
		swap(bound_min[1], bound_max[1]);
	if (bound_min[2] > bound_max[2])
		swap(bound_min[2], bound_max[2]);

	std::cerr << "RenderContext::addPolygon - eye space bounds are " << bound_min << " x " << bound_max << std::endl;

	// do hither-yon cull
	if ((bound_max[2] > _far && bound_min[2] > _far) 	// bound is completely outside far plane (too far away for the camera to see)
	 || (bound_max[2] < _near && bound_min[2] < _near))	// bound is completely outside near plane (effectively behind camera)
	{
		// cull poly
		std::cerr << "RenderContext::addPolygon - outside hither/yon. polygon was culled." << std::endl;
		// no need to continue since poly won't be rendered
		return;
	}

	// set initial diceable state of the primitive
	poly->diceable(true);

	// do hither e plane test & mark undiceable if necessary
	/*
		anything that spans hither or yon will make it past the first cull step above.
		those conditions are:
		(bound_max[2] < _far && bound_min[2] > _far)	not possible. max > min
		(bound_max[2] > _far && bound_min[2] < _far)	crosses far plane
		(bound_max[2] > _near && bound_min[2] < _near)	crosses near plane
		(bound_max[2] < _near && bound_min[2] > _near)	not possible. max > min
		
		in the e plane test we are only concerned with the third condition
	*/
	if (bound_max[2] > _near && bound_min[2] < _near)	// crosses near plane
	{
		// if bound_min[2] also crosses the eye plane, we'll need to split it 
		// (mark undiceable so it will be split on the next pass)
		if (bound_min[2] < 0.0)
		{
			std::cerr << "RenderContext::addPolygon - spanner - marking undiceable. polygon will be split." << std::endl;
			poly->diceable(false);
		}
		// else poly is in front of the eye plane and still projectable so continue
	}

	// convert bounds to screen space
	/*
		to get from eye to screen space apply the projection and screen transformations
		the projection matrix is appended to the current transformation when RiProjection() is called
		the current transformation is then saved as the screen transformation (with original screen & 
		projection matrices already combined).
		we don't want to transform the primitive here, just the bounds. 
		we'll still need eye space in the second pass.
	*/
	//poly->transform(_coordinateSystems["screen"]);
	// screen space to reyes means renderman raster space
	
	//bound = poly->bound();
	Matrix4 screen = _coordinateSystems["screen"];
	screen *= _coordinateSystems["raster"];
	bound_min = screen * bound_min;
	bound_max = screen * bound_max;

	// screen transform might've flipped some components of min & max
	if (bound_min[0] > bound_max[0])
		swap(bound_min[0], bound_max[0]);
	if (bound_min[1] > bound_max[1])
		swap(bound_min[1], bound_max[1]);
	if (bound_min[2] > bound_max[2])
		swap(bound_min[2], bound_max[2]);

	bound.extents(bound_min, bound_max);

	std::cerr << "RenderContext::addPolygon - screen transform matrix: " << std::endl << _coordinateSystems["screen"];
	std::cerr << "RenderContext::addPolygon - raster transform matrix: " << std::endl << _coordinateSystems["raster"];
	std::cerr << "RenderContext::addPolygon - screen + raster transform matrix: " << std::endl << screen;
	std::cerr << "RenderContext::addPolygon - screen space bounds are " << bound_min << " x " << bound_max << std::endl;

	// do viewing frustum cull
	Frustum frustum(screen);
	if (frustum.intersect(bound) == Frustum::OUTSIDE) // poly is entirely outside frustum
	{
		std::cerr << "RenderContext::addPolygon - outside frustum. polygon was culled." << std::endl;
		return;
	}

	/*
		if the primitive is spanning the e plane then we already know it needs split
		but we still need to determine if any other primitive that hasn't already 
		been culled will need to be split.
	*/
	if (poly->diceable())
	{
		/*
			if our primitive's raster space bound is bigger
			than the maximum grid size then it should be split
		*/
		float size = RenderEngine::instance().gridSize();
		size = sqrt(size) * RenderEngine::instance().shadingRate();
		Vector3 bound_size = bound_max - bound_min;
		std::cerr << "RenderContext::addPolygon - bound size = " << bound_size << " size = " << size << std::endl;
		if ((bound_size[0] > size) || (bound_size[1] > size))
		{
			std::cerr << "RenderContext::addPolygon - bounds exceed grid size. poly will be split." << std::endl;
			poly->diceable(false);
		}
	}

	/*
		if poly is diceable then we don't need to split it
		dicing is done in eye space so we go ahead and transform the poly's
		vertices now.
	 */
	if (poly->diceable())
	{
		for (unsigned int i = 0; i < poly->vertexCount(); i++)
		{
			Matrix4 em = _transform * _coordinateSystems["camera"].transpose();
			Vertex pv = poly->vertex(i);
			pv.point(em * pv.point());
			(*poly)[i] = pv;
		}
	}

	// put the polygon in a bucket
	/*
		this is the last step of the first pass
		we just need to know which bucket to put the primitive in
		to do this we need the upper left corner of the primitive's bound
		buckets work in raster space (buckets are evenly tiled in pixel space)
		up to now our bound is only in screen space
		
		NOTE: reyes does not have raster or ndc space. raster space is the same as screen space
			  so when we move to screen space above, we should really be moving to what 
			  RenderMan calls raster space.
	*/
	_frameBuffer->addPrimitive(poly, bound);
	// nothing left to do in the first pass for this primitive
}
/*
unsigned int RenderContext::getPolygonCount(void) const
{
  return _polygons.size();
}

PolygonPtr RenderContext::getPolygon(unsigned int idx) const
{
  assert(idx >= 0 && idx < _polygons.size());
  return _polygons[idx];
}
*/

/*
	perform the second reyes pass
*/
void RenderContext::render(void)
{
	std::cerr << "RenderContext::render - begin rendering framebuffer." << std::endl;

	_frameBuffer->render();

	std::cerr << "RenderContext::render - finished rendering framebuffer." << std::endl;
}
