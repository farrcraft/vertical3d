How does the Odyssey rendering pipeline work


How should the UI rendering system fit into that

RenderEngine
	Scene
	drawFrame()
		frame = Scene.collect() // collect all of this frame's rendering operations from the current scene
		frame -> draw

Frame
	[] Operations


The Odyssey system collects a series of rendering operations during each tick and then runs the operation in a final draw step.
It is a 2D engine that uses SDL rendering primitives

Pong uses a 3D engine
It uses a canvas to draw shapes onto (ball, paddles, etc) and a font renderer to write text each tick and 
then uploads the resulting vertex buffers / shader programs in the draw step.

How will this translate to an ECS system?

There would be a RenderingSystem (maybe this owns one of the v3d::render::realtime::Engine)

Renderable is a generic component type, but it doesn't tell us how to actually render the entity

Different kinds of potential components:
Sprite
Canvas
TextureFont (all of the individual render::realtime::operation classes)