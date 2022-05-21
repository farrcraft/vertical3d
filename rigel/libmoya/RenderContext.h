/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <vector>
#include <map>

#include <libv3dtypes/Matrix4.h>

#include "Polygon.h"
#include "FrameBuffer.h"

namespace v3D
{

	namespace Moya
	{

		/*
			holds the current graphics state
			multiple contexts may exist at once, but only one is ever active at any
			time
		*/
		class RenderContext
		{
			public:
				RenderContext();
				RenderContext(const std::string & name);
				~RenderContext();
	
				typedef std::vector<PolygonPtr> PolygonList;

				/*
					called from RenderEngine::prepareWorld()
					corresponds to RiWorldBegin()
					the view options are frozen beyond this call
					the framebuffer will be allocated here
				*/
				void prepareWorld(void);
				// the RenderEngine::render() method will call this method on the active RC
				void render(void);
	
				// get methods
				unsigned int imageWidth(void) const;
				unsigned int imageHeight(void) const;
				float pixelAspect(void) const;
	
				// manipulators
				/*
					maps to RiFormat(xres, yres, aspect)
					sets the pixel resolution and aspect ratio of the image to 
					be rendered
					default values will be used when not called
				*/
				void imageResolution(int xres, int yres, float aspect);
				/*
					maps to RiClipping(hither, yon)
					sets the position of the near and far clipping planes
				*/
				void clipping(float near, float far);

				void projection(std::string name, float fov = 90.0);

				void pushTransform(void);
				void popTransform(void);
				/*
					maps to RiCoordinateSystem()
					saves the current transformation as a custom named coordinate system
					default reserved names: object, world, camera, screen, raster, NDC
				*/
				void saveCoordinateSystem(const std::string & name);
				/*
					maps to RiCoordSysTransform()
					replaces the current transformation with the named coordinate system
					behavior is undefined if name isn't a reserved name or previously saved name

					should these *not* start with "set" ?
				*/
				void setCoordinateSystem(const std::string & name);
				void setIdentityTransform(void);
				void setTransform(const Matrix4 & trans);
	
				void translate(float dx, float dy, float dz);
				void rotate(float angle, float dx, float dy, float dz);
				void scale(float sx, float sy, float sz);

				/*
					maps to RiPolygon()
					polygon will be placed into a starting bucket when it is initially added
				*/
				void addPolygon(const PolygonPtr & poly);

				/*
					Get the matrix for a named coordinate system.
				*/
				Matrix4 coordinateSystem(const std::string & name);

				/*
				unsigned int getPolygonCount(void) const;
				PolygonPtr getPolygon(unsigned int idx) const;
				*/

			protected:
				void initialize(void);

			private:
				std::string		  		_name;
				//vector<PolygonPtr>  	_polygons;
				std::vector<Matrix4>	_transforms;
				std::map<std::string, Matrix4>	_coordinateSystems;
				FrameBufferPtr			_frameBuffer;
				// camera options
				unsigned int			_xres;
				unsigned int			_yres;
				float					_pixelAspect;
				float					_crop[4];		// region of raster that is rendered. defaults [0,1,0,1]
				float					_frameAspect;	// default: 4/3
				float					_screen[4];		// screen coordinates (after projection) of area to be rendered. default: [-4/3, 4/3, -1, 1]
				std::string				_projection;	// type of projection - default: "orthographic"
				Matrix4					_transform;		// world to camera transformation matrix / current transformation matrix
				float					_near;			// near clipping plane. default: epsilon
				float					_far;			// far clipping plane. default: infinity
				// other clipping planes
				float					_fStop;			// for depth of field. default: infinity
				float					_focalLength;
				float					_focalDistance;
				float					_shutterOpen;	// default: 0
				float					_shutterClose;	// default: 0
		};

	}; // end namespace Moya

}; // end namespace v3D
