/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "RenderContext.h"

namespace v3d::moya {
	/**
		the base of the renderer is the RenderEngine class.
		this class maintains a stack of render contexts

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
	class Renderer
	{
		public:
			Renderer();
			~Renderer();

			/**
				*	maps to RiBegin(name)
				*  creates a new active render context and puts it on the end of the stack.
				*	most calls require an active context (excluding RiErrorHandler, 
				*	RiOption, and RiContext)
				*	@param name a renderer or implementation specific name or the name of a RIB file to write. 
				*				an empy string implies default values
				*/
			void createRenderContext(const std::string & name);
			/**
				*	maps to RiEnd()
				*	pops the last context off the stack
				*/
			void destroyActiveRenderContext(void);
			/**
				* Get the active rendering context
				*/
			RenderContext & activeRenderContext(void);

		private:
			std::vector<RenderContext>	_contexts;
	};
};
