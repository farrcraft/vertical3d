/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "TextureFont.h"

#include <boost/shared_ptr.hpp>

#include <vector>
#include <string>

namespace v3d::font {
	class TextureFontCache {
		public:
			TextureFontCache(unsigned int width, unsigned int height, unsigned int depth);
			~TextureFontCache();

			void charcodes(const wchar_t * charcodes);

			boost::shared_ptr<TextureFont> load(const std::string & filename, float size);
			bool remove(boost::shared_ptr<TextureFont> font);

			boost::shared_ptr<v3d::image::TextureAtlas> atlas();

		private:
			boost::shared_ptr<v3d::image::TextureAtlas> atlas_;
			std::vector<boost::shared_ptr<TextureFont> > fonts_;
			wchar_t * cache_;
	};
};