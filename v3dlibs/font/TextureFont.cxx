/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "TextureFont.h"
#include "../image/TextureAtlas.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_STROKER_H
#include FT_LCD_FILTER_H

#include <cmath>

#include <boost/log/trivial.hpp>

using namespace v3d::font;

class TextureFont::Freetype
{
	public:
		bool loadFace(const std::string & filename, float size);

		void release();

		FT_Library library_;
		FT_Face face_;
};


void TextureFont::Freetype::release()
{
	FT_Done_Face(face_);
	FT_Done_FreeType(library_);
}

bool TextureFont::Freetype::loadFace(const std::string & filename, float size)
{
	// initialize freetype library
	FT_Error error;
	if ((error = FT_Init_FreeType(&library_)) != 0)
	{
		BOOST_LOG_TRIVIAL(error) << "Error initializing freetype library!";
		return false;
	}

	// load font file
	if ((error = FT_New_Face(library_, filename.c_str(), 0, &face_)) != 0)
	{
		BOOST_LOG_TRIVIAL(error) << "Error creating new freetype face!";
		FT_Done_FreeType(library_);
		return false;
	}

	// select charmap
	if ((error = FT_Select_Charmap(face_, FT_ENCODING_UNICODE)) != 0)
	{
		BOOST_LOG_TRIVIAL(error) << "Error selecting freetype charmap!";
		release();
		return false;
	}

	// set char size
	//size *= 100.0f;
	size_t hres = 64;
	if ((error = FT_Set_Char_Size(face_, (int)(size*64), 0, 72*hres, 72)) != 0)
	{
		BOOST_LOG_TRIVIAL(error) << "Error setting freetype char size!";
		release();
		return false;
	}

	// set transform matrix
	FT_Matrix matrix = { 
		(int)((1.0/hres) * 0x10000L),
		(int)((0.0)      * 0x10000L),
		(int)((0.0)      * 0x10000L),
		(int)((1.0)      * 0x10000L) 
	};
	FT_Set_Transform(face_, &matrix, NULL);

	return true;
}

TextureFont::TextureFont(boost::shared_ptr<TextureAtlas> atlas, const std::string & filename, float size) :
	atlas_(atlas),
	filename_(filename),
	size_(size),
	height_(0),
	ascender_(0),
	descender_(0),
	outline_(OUTLINE_TYPE_NONE),
	outlineThickness_(0.0f),
	hinting_(1),
	lcdFiltering_(1)
{
	lcdWeights_[0] = 0x10;
	lcdWeights_[1] = 0x40;
	lcdWeights_[2] = 0x70;
	lcdWeights_[3] = 0x40;
	lcdWeights_[4] = 0x10;

	freetype_.reset(new Freetype());
	if (!freetype_->loadFace(filename_, size_ * 100))
	{
		return;
	}

	// 64 * 64 because of 26.6 encoding AND the transform matrix used
	// in texture_font_load_face (hres = 64)
	underlinePosition_ = freetype_->face_->underline_position / static_cast<float>(64.0f*64.0f) * size_;
	underlinePosition_ = underlinePosition_ < 0.0f ? std::ceil(underlinePosition_ - 0.5f) : std::floor(underlinePosition_ + 0.5f);
	if (underlinePosition_ > -2.0f)
	{
		underlinePosition_ = -2.0f;
	}

	underlineThickness_ = freetype_->face_->underline_thickness / static_cast<float>(64.0f*64.0f) * size_;
	underlineThickness_ = underlineThickness_ < 0.0f ? std::ceil(underlineThickness_ - 0.5f) : std::floor(underlineThickness_ + 0.5f);
	if (underlineThickness_ < 1.0f)
	{
		underlineThickness_ = 1.0f;
	}

	FT_Size_Metrics metrics = freetype_->face_->size->metrics; 
	ascender_ = (metrics.ascender >> 6) / 100.0f;
	descender_ = (metrics.descender >> 6) / 100.0f;
	height_ = (metrics.height >> 6) / 100.0f;
	linegap_ = height_ - ascender_ + descender_;

	freetype_->release();
}

boost::shared_ptr<TextureFont::Glyph> TextureFont::createGlyph()
{
	boost::shared_ptr<Glyph> glyph(new Glyph);

	glyph->id_        = 0;
	glyph->width_     = 0;
	glyph->height_    = 0;
	glyph->outline_	  = OUTLINE_TYPE_NONE;
	glyph->outlineThickness_ = 0.0;
	glyph->offset_.x  = 0;
	glyph->offset_.y  = 0;
	glyph->advance_.x = 0.0f;
	glyph->advance_.y = 0.0f;
	glyph->st_[0].s   = 0.0f;
	glyph->st_[0].t   = 0.0f;
	glyph->st_[1].s   = 0.0f;
	glyph->st_[1].t   = 0.0f;

	return glyph;
}

boost::shared_ptr<TextureFont::Glyph> TextureFont::glyph(wchar_t charcode)
{
	wchar_t lineCode = static_cast<wchar_t>(-1);

	for (unsigned int i = 0; i < glyphs_.size(); ++i)
	{
		if ((glyphs_[i]->charcode_ == charcode) &&
			((charcode == lineCode) ||
			((glyphs_[i]->outline_ == outline_) &&
			(glyphs_[i]->outlineThickness_ == outlineThickness_))))
		{
			return glyphs_[i];
		}
	}
	boost::shared_ptr<Glyph> glyph;
	// -1 is used for line drawing (overline, underline, strikethrough) and background
	if (charcode == lineCode)
	{
		glm::ivec4 region = atlas_->region(5, 5);

		if (region.x < 0)
		{
			BOOST_LOG_TRIVIAL(debug) << "Texture atlas is full!";
			return glyph;
		}
		glyph = createGlyph();
		static unsigned char data[4*4*3] = 
		{
			-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
            -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
			-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
			-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
		};

		atlas_->region(region.x, region.y, 4, 4, data, 0);

		float width = static_cast<float>(atlas_->width());
		float height = static_cast<float>(atlas_->height());

		glyph->charcode_ = lineCode;

		glyph->st_[0].s = (region.x+2)/width;
		glyph->st_[0].t = (region.y+2)/height;
		glyph->st_[1].s = (region.x+3)/width;
		glyph->st_[1].t = (region.y+3)/height;

		glyphs_.push_back(glyph);

		return glyph;
	}

	wchar_t buffer[2] = {0,0};
	buffer[0] = charcode;

	if (loadGlyphs(buffer))
	{
		return glyphs_[glyphs_.size()-1];
	}

	return glyph;
}

bool TextureFont::loadGlyphs(const wchar_t * charcodes)
{
	if (!freetype_->loadFace(filename_, size_))
	{
		return false;
	}
	unsigned int missed = 0;
	for (unsigned int i = 0; i < wcslen(charcodes); ++i)
	{
		FT_UInt glyphIndex = FT_Get_Char_Index(freetype_->face_, charcodes[i]);
		FT_Int32 flags = 0;
		if (outline_ != OUTLINE_TYPE_NONE)
		{
			flags |= FT_LOAD_NO_BITMAP;
		}
		else
		{
			flags |= FT_LOAD_RENDER;
		}

		if (!hinting_)
		{
			flags |= FT_LOAD_NO_HINTING | FT_LOAD_NO_AUTOHINT;
		}
		else
		{
			flags |= FT_LOAD_FORCE_AUTOHINT;
		}

		if (atlas_->depth() == 3)
		{
			FT_Library_SetLcdFilter(freetype_->library_, FT_LCD_FILTER_LIGHT);
			flags |= FT_LOAD_TARGET_LCD;
			if(lcdFiltering_)
			{
				FT_Library_SetLcdFilterWeights(freetype_->library_, lcdWeights_);
			}
		}
		FT_Error error;
		if ((error = FT_Load_Glyph(freetype_->face_, glyphIndex, flags)) != 0)
		{
			BOOST_LOG_TRIVIAL(error) << "Error loading glyph!";
			FT_Done_FreeType(freetype_->library_);
			return false;
		}
		FT_Glyph ft_glyph;
		FT_GlyphSlot slot;
		FT_Bitmap ft_bitmap;
		int ft_bitmap_width = 0;
		int ft_bitmap_rows = 0;
		int ft_bitmap_pitch = 0;
		int ft_glyph_top = 0;
		int ft_glyph_left = 0;
		if (outline_ == OUTLINE_TYPE_NONE)
		{
			slot            = freetype_->face_->glyph;
			ft_bitmap       = slot->bitmap;
			ft_bitmap_width = slot->bitmap.width;
			ft_bitmap_rows  = slot->bitmap.rows;
			ft_bitmap_pitch = slot->bitmap.pitch;
			ft_glyph_top    = slot->bitmap_top;
			ft_glyph_left   = slot->bitmap_left;
		}
		else
		{
			FT_Stroker stroker;
			if ((error = FT_Stroker_New(freetype_->library_, &stroker)) != 0)
			{
				BOOST_LOG_TRIVIAL(error) << "Error creating stroker!";
				freetype_->release();
				return false;
			}
			FT_Stroker_Set(
				stroker,
				static_cast<int>(outlineThickness_ *64),
				FT_STROKER_LINECAP_ROUND,
				FT_STROKER_LINEJOIN_ROUND,
				0
			);
			if ((error = FT_Get_Glyph(freetype_->face_->glyph, &ft_glyph)) != 0)
			{
				BOOST_LOG_TRIVIAL(error) << "Error getting glyph!";
				freetype_->release();
				return false;
            }

			if (outline_ == OUTLINE_TYPE_LINE)
			{
				error = FT_Glyph_Stroke(&ft_glyph, stroker, 1);
			}
			else if (outline_ == OUTLINE_TYPE_INNER)
			{
				error = FT_Glyph_StrokeBorder(&ft_glyph, stroker, 0, 1);
			}
			else if (outline_ == OUTLINE_TYPE_OUTER)
			{
				error = FT_Glyph_StrokeBorder(&ft_glyph, stroker, 1, 1);
			}
			if (error)
			{
				BOOST_LOG_TRIVIAL(error) << "Error setting glyph stroke border!";
				freetype_->release();
				return false;
			}

			if(atlas_->depth() == 1)
			{
				if ((error = FT_Glyph_To_Bitmap(&ft_glyph, FT_RENDER_MODE_NORMAL, 0, 1)) != 0)
				{
					BOOST_LOG_TRIVIAL(error) << "Error converting glyph to bitmap!";
					freetype_->release();
					return false;
				}
			}
			else
			{
				if ((error = FT_Glyph_To_Bitmap( &ft_glyph, FT_RENDER_MODE_LCD, 0, 1)) != 0)
				{
					BOOST_LOG_TRIVIAL(error) << "Error converting glyph to bitmap!";
					freetype_->release();
					return false;
				}
			}
			FT_BitmapGlyph ft_bitmap_glyph = (FT_BitmapGlyph) ft_glyph;
			ft_bitmap       = ft_bitmap_glyph->bitmap;
			ft_bitmap_width = ft_bitmap.width;
			ft_bitmap_rows  = ft_bitmap.rows;
			ft_bitmap_pitch = ft_bitmap.pitch;
			ft_glyph_top    = ft_bitmap_glyph->top;
			ft_glyph_left   = ft_bitmap_glyph->left;
			FT_Stroker_Done(stroker);
        }

		// We want each glyph to be separated by at least one black pixel
		unsigned int w = ft_bitmap_width / atlas_->depth() + 1;
		unsigned int h = ft_bitmap_rows + 1;
		glm::ivec4 region = atlas_->region(w, h);
		if (region.x < 0)
		{
			missed++;
			BOOST_LOG_TRIVIAL(error) << "Texture atlas is full!";
			continue;
		}
		w = w - 1;
		h = h - 1;
		unsigned int x = region.x;
		unsigned int y = region.y;
		atlas_->region(x, y, w, h, ft_bitmap.buffer, ft_bitmap.pitch);

		boost::shared_ptr<Glyph> glyph = createGlyph();
		glyph->charcode_ = charcodes[i];
		glyph->width_    = w;
		glyph->height_   = h;
		glyph->outline_  = outline_;
		glyph->outlineThickness_ = outlineThickness_;
		glyph->offset_.x = ft_glyph_left;
		glyph->offset_.y = ft_glyph_top;
		size_t width = atlas_->width();
		size_t height = atlas_->height();
		glyph->st_[0].s  = x / (float)width;
		glyph->st_[0].t  = y / (float)height;
		glyph->st_[1].s  = (x + glyph->width_) / (float)width;
		glyph->st_[1].t  = (y + glyph->height_) / (float)height;

		// Discard hinting to get advance
		FT_Load_Glyph(freetype_->face_, glyphIndex, FT_LOAD_RENDER | FT_LOAD_NO_HINTING);
		slot = freetype_->face_->glyph;
		glyph->advance_.x = slot->advance.x/64.0f;
		glyph->advance_.y = slot->advance.y/64.0f;

		glyphs_.push_back(glyph);

		if (outline_ != OUTLINE_TYPE_NONE)
		{
			FT_Done_Glyph(ft_glyph);
		}
	}

	generateKerning();
	freetype_->release();

	//return missed;
	return true;
}

void TextureFont::generateKerning()
{
	// For each glyph couple combination, check if kerning is necessary
	// Starts at index 1 since 0 is for the special backgroudn glyph
	for (unsigned int i = 1; i < glyphs_.size(); ++i)
    {
		boost::shared_ptr<Glyph> glyph = glyphs_[i];
		FT_UInt glyphIndex = FT_Get_Char_Index(freetype_->face_, glyph->charcode_);
		glyph->kerning_.clear();

		for (unsigned int j = 1; j < glyphs_.size(); ++j)
		{
			boost::shared_ptr<Glyph> prevGlyph = glyphs_[j];
			FT_UInt prevIndex = FT_Get_Char_Index(freetype_->face_, prevGlyph->charcode_);
			FT_Vector kerning;
			FT_Get_Kerning(freetype_->face_, prevIndex, glyphIndex, FT_KERNING_UNFITTED, &kerning);
			if (kerning.x)
			{
				// 64 * 64 because of 26.6 encoding AND the transform matrix used
				// in loadFace (hres = 64)
                Kerning k = {prevGlyph->charcode_, kerning.x / static_cast<float>(64.0f*64.0f)};
				glyph->kerning_.push_back(k);
			}
		}
	}
}

float TextureFont::kerning(boost::shared_ptr<Glyph> glyph, wchar_t charcode)
{
	for (unsigned int i = 0;  i < glyph->kerning_.size(); ++i)
	{
		if (glyph->kerning_[i].charcode_ == charcode)
		{
			return glyph->kerning_[i].kerning_;
		}
	}
	return 0.0f;
}

float TextureFont::size() const
{
	return size_;
}

std::string TextureFont::filename() const
{
	return filename_;
}

float TextureFont::ascender() const
{
	return ascender_;
}

float TextureFont::descender() const
{
	return descender_;
}

float TextureFont::height() const
{
	return height_;
}

float TextureFont::linegap() const
{
	return linegap_;
}

float TextureFont::underlinePosition() const
{
	return underlinePosition_;
}

float TextureFont::underlineThickness() const
{
	return underlineThickness_;
}
