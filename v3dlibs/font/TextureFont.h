/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <glm/glm.hpp>
#include <boost/shared_ptr.hpp>

#include <vector>
#include <string>

namespace v3d::font {

	class TextureAtlas;

	class TextureFont {
		public:
			typedef struct {
				wchar_t charcode_;
				float kerning_;
			} Kerning;

			typedef enum
			{
				OUTLINE_TYPE_NONE = 0,
				OUTLINE_TYPE_LINE = 1,
				OUTLINE_TYPE_INNER = 2,
				OUTLINE_TYPE_OUTER = 3
			} OutlineType;

			typedef struct {
				unsigned int id_;
				wchar_t charcode_;
				unsigned int width_;
				unsigned int height_;
				glm::ivec2 offset_;
				glm::vec2 advance_;
				glm::vec2 st_[2];
				std::vector<Kerning> kerning_;
				OutlineType outline_;
				float outlineThickness_;
			} Glyph;

			TextureFont(boost::shared_ptr<TextureAtlas> atlas, const std::string & filename, float size);

			boost::shared_ptr<Glyph> glyph(wchar_t charcode);
			float kerning(boost::shared_ptr<Glyph>, wchar_t charcode);

			bool loadGlyphs(const wchar_t * charcodes);

			float size() const;
			std::string filename() const;

			float ascender() const;
			float descender() const;
			float height() const;
			float linegap() const;
			float underlinePosition() const;
			float underlineThickness() const;

		protected:
			void generateKerning();
			boost::shared_ptr<Glyph> createGlyph();

		private:
			std::vector<boost::shared_ptr<Glyph> > glyphs_;
			boost::shared_ptr<TextureAtlas> atlas_;
			std::string filename_;
			float size_;
			int hinting_;
			OutlineType outline_;
			float outlineThickness_;
			int lcdFiltering_;
			unsigned char lcdWeights_[5];
			float height_;
			float linegap_;
			float ascender_;
			float descender_;
			float underlinePosition_;
			float underlineThickness_;

			class Freetype;
			boost::shared_ptr<Freetype> freetype_;
	};
};
