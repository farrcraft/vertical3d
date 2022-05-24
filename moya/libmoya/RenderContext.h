/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Polygon.h"
#include "FrameBuffer.h"

#include "../../v3dlibs/type/Matrix4.h"

#include <vector>
#include <map>
#include <string>

namespace v3D
{

	namespace Moya
	{

		/**
		 *	holds the current graphics state
		 *	multiple contexts may exist at once, but only one is ever active at any
		 *	time
		 */
		class RenderContext
		{
			public:
				RenderContext();
				RenderContext(const std::string & name);
				~RenderContext();
	
				typedef std::vector<boost::shared_ptr<Polygon> > PolygonList;

				/**
				 *	maps to RiWorldBegin()
				 *	freezes all rendering options, world to camera transformation 
				 *	is set to current transormation, current transformation is set to identity
				 *	the view options are frozen beyond this call
				 *	the framebuffer will be allocated here
				 */
				void prepareWorld();
				/**
				 *	maps to RiWorldEnd()
				 *	once rendering is done, objects, lights and other stuff set
				 *	after prepareWorld() are destroyed and the memory reclaimed
				 *	saving the output file is done here also.
				 *	the results might not even include a rendered image depending
				 *	on the context. we might just output a rib file.
				 */
				void render();
	
				// get methods
				unsigned int imageWidth() const;
				unsigned int imageHeight() const;
				float pixelAspect() const;
	
				// manipulators
				/**
				 *	maps to RiFormat(xres, yres, aspect)
				 *	sets the pixel resolution and aspect ratio of the image to 
				 *	be rendered
				 *	default values will be used when not called
				 */
				void imageResolution(int xres, int yres, float aspect);
				/**
				 *	maps to RiClipping(hither, yon)
				 *	sets the position of the near and far clipping planes
				 */
				void clipping(float near, float far);

				void projection(std::string name, float fov = 90.0);

				void pushTransform();
				void popTransform();
				/**
				 *	maps to RiCoordinateSystem()
				 *	saves the current transformation as a custom named coordinate system
				 *	default reserved names: object, world, camera, screen, raster, NDC
				 */
				void saveCoordinateSystem(const std::string & name);
				/**
				 *	maps to RiCoordSysTransform()
				 *	replaces the current transformation with the named coordinate system
				 *	behavior is undefined if name isn't a reserved name or previously saved name
				 *
				 *	[FIXME]: should these *not* start with "set" ?
				 */
				void setCoordinateSystem(const std::string & name);
				void setIdentityTransform();
				void setTransform(const v3d::type::Matrix4 & trans);

				void translate(float dx, float dy, float dz);
				void rotate(float angle, float dx, float dy, float dz);
				void scale(float sx, float sy, float sz);

				/**
				 *	maps to RiPolygon()
				 *	polygon will be placed into a starting bucket when it is initially added
				 */
				void addPolygon(boost::shared_ptr<Polygon> poly);

				/**
				 *	Get the matrix for a named coordinate system.
				 */
				v3d::type::Matrix4 coordinateSystem(const std::string & name);

				/*
				unsigned int getPolygonCount(void) const;
				PolygonPtr getPolygon(unsigned int idx) const;
				*/

				unsigned int bucketWidth() const;
				unsigned int bucketHeight() const;
				unsigned int gridSize() const;
				float shadingRate() const;

			protected:
				void initialize();

			private:
				std::string		  		_name;
				//vector<PolygonPtr>  	_polygons;
				std::vector<v3d::type::Matrix4>	_transforms;
				std::map<std::string, v3d::type::Matrix4>	_coordinateSystems;
				boost::shared_ptr<FrameBuffer>	_frameBuffer;
				// camera options
				unsigned int			_xres;
				unsigned int			_yres;
				float					_pixelAspect;
				float					_crop[4];		// region of raster that is rendered. defaults [0,1,0,1]
				float					_frameAspect;	// default: 4/3
				float					_screen[4];		// screen coordinates (after projection) of area to be rendered. default: [-4/3, 4/3, -1, 1]
				std::string				_projection;	// type of projection - default: "orthographic"
				v3d::type::Matrix4 _transform;		// world to camera transformation matrix / current transformation matrix
				float					_near;			// near clipping plane. default: epsilon
				float					_far;			// far clipping plane. default: infinity
				// other clipping planes
				float					_fStop;			// for depth of field. default: infinity
				float					_focalLength;
				float					_focalDistance;
				float					_shutterOpen;	// default: 0
				float					_shutterClose;	// default: 0


				/*
					the framebuffer is divided up into buckets of n-by-m size
					minimum grid size is:
						bucket size / shading rate = grid size
						e.g.
							12x12/4=36
							16x16/1=256
					the bucket size is specified in separate width and height values
					shading rate: smaller values = higher frequency shading rate (higher quality)
					shading rate = 1 = 1 sample per pixel = 256 grid size = 16x16 micropolys
					shading rate = .25 = 4 samples per pixel = 1024 grid size = 32x32 micropolys
					pixel size of grid is?
					32x32 * .25 = 8x8 pixel grid
					16x16 * 1 = 16x16 pixel grid
				*/
				unsigned int				_bucketWidth;	// default = 16, 16
				unsigned int				_bucketHeight;
				unsigned int				_gridSize;		// default = 256
				float						_shadingRate;
		};

	}; // end namespace Moya

}; // end namespace v3D
