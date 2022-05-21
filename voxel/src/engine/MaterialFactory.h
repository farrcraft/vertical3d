/**
 * Voxel Engine
 *
 * (c) Joshua Farr <j.wgasa@gmail.com>
 *
 */

#pragma once

#include <vertical3d/gl/Program.h>
#include <boost/shared_ptr.hpp>
#include <glm/glm.hpp>

class MaterialFactory
{
	public:
		MaterialFactory(boost::shared_ptr<v3D::Program> program);

		void create(const std::string & name, const glm::vec3 & color);

	private:
		boost::shared_ptr<v3D::Program> program_;
};
