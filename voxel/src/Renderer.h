/**
 * Voxel Engine
 *
 * (c) Joshua Farr <j.wgasa@gmail.com>
 *
 */

#pragma once

#include "voxel/Voxel.h"
#include "voxel/MeshBuilder.h"

#include <vertical3d/hookah/Window.h>
#include <vertical3d/gl/Program.h>

#include <boost/shared_ptr.hpp>

class Scene;
class AssetLoader;
class DebugOverlay;
class ChunkBufferPool;

/**
 * Main engine renderer
 */
class Renderer
{
	public:
		/**
		 * Default Constructor
		 */
		Renderer(boost::shared_ptr<Scene> & scene, boost::shared_ptr<AssetLoader> & loader);

		/**
		 * Draw the frame
		 */
		void draw(Hookah::Window * window);
		/**
		 * Resize the frame
		 */
		void resize(int width, int height);

		void tick(unsigned int delta);

		void debug(bool status);

	private:
		std::map<std::string, boost::shared_ptr<v3D::Program> > programs_;
		boost::shared_ptr<Scene> scene_;
		bool debug_;
		boost::shared_ptr<DebugOverlay> debugOverlay_;
		boost::shared_ptr<ChunkBufferPool> pool_;
		MeshBuilder builder_;
};
