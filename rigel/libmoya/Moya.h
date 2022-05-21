
/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <libv3dtypes/Color3.h>

#include "RenderContext.h"

namespace v3D
{

	namespace Moya
	{

		/**
			the base of the renderer is the RenderEngine class.
			there is only ever a single global instance available (gotten with the getInstance() method)
			this class maintains a stack of render contexts
			it also provides a mapping to a few of the RenderMan C API calls


			From the RenderMan FAQ (http://www.faqs.org/faqs/graphics/renderman-faq/):
			
			Q: What do I have to do in order to call my software "RenderMan compliant"?
			--------------------------------------------------------------------------

				You must support all of the required features, including all of
			the geometric primitive types.  You must implement all of the API
			calls or RIB requests, even for features you don't support (i.e. at
			least have function stubs so your library accepts the calls).  You
			must not allow information to be sent through any alternative calls
			not defined by the interface, and you may not use any alternate
			language for programmable shading.

				If you write a RenderMan compliant renderer, you must obtain a
			no-charge license from Pixar (in writing).  You basically attest that
			your software is compliant with the standard and that you won't abuse
			Pixar's trademark and copyrights.

				Modeling programs which use the RI standard (i.e. output RIB or
			make the API calls) may do so without a license, but must display
			Pixar's copyrights as follows:

				The RenderMan (R) Interface Procedures and Protocol are:
					Copyright 1988, 1989, Pixar
					All Rights Reserved


			Moya *is not* a "RenderMan compliant" renderer.
			Although it supports the RI standard, having the Moya API violates the
			"one true API" clause. The Pixar copyright notice is printed on stdout 
			whenever a new render context is pushed onto an empty stack.
		*/
		class RenderEngine
		{
			public:
				static RenderEngine & instance(void);

				/**
					maps to RiBegin(name)
					creates a new active render context and puts it on the end of the stack.
					most calls require an active context (excluding RiErrorHandler, 
					RiOption, and RiContext)
					@param name a renderer or implementation specific name or the name of a RIB file to write. 
								an empy string implies default values
				*/
				void createRenderContext(std::string name);
				/**
					maps to RiEnd()
					pops the last context off the stack
				*/
				void destroyActiveRenderContext(void);
				/**
					maps to RiWorldBegin()
					freezes all rendering options, world to camera transformation 
					is set to current transormation, current transformation is set 
					to identity
				*/
				void prepareWorld(void);
				/**
					maps to RiWorldEnd()
					once rendering is done, objects, lights and other stuff set
					after prepareWorld() are destroyed and the memory reclaimed
					saving the output file is done here also.
					the results might not even include a rendered image depending
					on the context. we might just output a rib file.
				*/
				void render(void);
				/**
					maps to RiColor(color)
					sets the current color
					@param color the color to set.
				*/
				void setCurrentColor(Color3 color);

				RenderContext & activeRenderContext(void);

				unsigned int bucketWidth(void) const;
				unsigned int bucketHeight(void) const;
				unsigned int gridSize(void) const;
				float shadingRate(void) const;

			protected:
				RenderEngine();
				~RenderEngine();

				/*
					called from render() to perform cleanup.
					on return, renderer state should be something like it was
					when prepareWorld() was called.
				*/
				void destroyWorld(void);

				void renderPolygon(const PolygonPtr & poly);

			private:
				std::vector<RenderContext>	_contexts;
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
				//vector<matrix4>			_transforms;
		};

	}; // end namespace Moya

}; // end namespace v3D
