/**
 * (c) Joshua Farr <j.wgasa@gmail.com>
 *
 */
#pragma once

#include <glm/glm.hpp>
#include <boost/shared_ptr.hpp>
#include <vector>

namespace v3D
{
	class Image;

	class TextureAtlas
	{
		public:
			TextureAtlas(unsigned int width, unsigned int height, unsigned int depth);
			~TextureAtlas();

			unsigned int width() const;
			unsigned int height() const;
			unsigned int depth() const;
			unsigned int id() const;

			boost::shared_ptr<Image> image();
			void write(const std::string & filename);

			glm::ivec4 region(unsigned int width, unsigned int height);
			void region(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned char * data, unsigned int stride);

		protected:
			int fit(unsigned int index, unsigned int width, unsigned int height);
			void merge();

		private:
			std::vector<glm::ivec3> nodes_;
			unsigned int width_;
			unsigned int height_;
			unsigned int depth_;
			size_t used_;
			unsigned int id_;
			boost::shared_ptr<Image> image_;
	};

};
