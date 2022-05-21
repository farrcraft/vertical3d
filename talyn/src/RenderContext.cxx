#include "RenderContext.h"

using namespace Talyn;

RenderContext::RenderContext()
{
}

void RenderContext::format(unsigned int width, unsigned int height)
{
	// allocate a new 4 channel framebuffer object
	framebuffer_.reset(new FrameBuffer(width, height, 4));
}

void RenderContext::render()
{
	/*
	rendering algorithm:

	for each pixel in the framebuffer
		generate ray R with origin O at viewing position & passing through point on viewing plane for the current pixel
		pixel color = trace (R, 0)
		render pixel color into framebuffer

	trace (ray, depth)
		distance = maximum distance
		hit object = null
		for each object in scene
			calculate distance from intersection of R & current object
			if intersection distance < distance
				hit object = current object
				distance = intersection distance
		if hit object is null
			return background color
		color = black
		for each light source in scene
			for each object in scene
				if object blocks light between light source origin & intersection point
					attenuate light intensity by transmittivity of object
			calculate color of hit object at intersection point with attenuated light intensity
			add calculated color to final color
		if depth < maximum depth
			generate reflection ray
			generate refraction ray
			trace (reflection, depth + 1)
			add color * object reflectivity to final color 
			trace (refraction, depth + 1)
			add color * object transmittivity to final color
		return color
	*/
}

boost::shared_ptr<FrameBuffer> RenderContext::framebuffer() const
{
	return framebuffer_;
}
