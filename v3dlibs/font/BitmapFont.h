/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "../image/Texture.h"

#include <boost/shared_ptr.hpp>

#include <map>
#include <string>

namespace v3d::font {
	class BitmapFont {
		public:
			BitmapFont(const std::string & path, std::string & name);

			struct CharDescriptor {
				unsigned short x_;
				unsigned short y_;
				unsigned short width_;
				unsigned short height_;
				short xOffset_;
				unsigned short yOffset_;
				unsigned short xAdvance_;
				unsigned short page_;
				unsigned short channel_;
			};

			struct Charset {
				unsigned short lineHeight_;
				unsigned short base_;
				unsigned short width_;
				unsigned short height_;
				unsigned short pages_;
				std::string fileName_;
				std::map<unsigned short, CharDescriptor> chars_;
			};

			unsigned short charsetWidth() const;
			unsigned short charsetHeight() const;
			unsigned short lineHeight() const;

			CharDescriptor character(char c);
			boost::shared_ptr<v3d::image::Texture> texture();

		protected:
			void loadCharset(const std::string & filename);
			bool loadTexture(const std::string & filename);

		private:
			Charset charset_;
			boost::shared_ptr<v3d::image::Texture> texture_;
	};
};
