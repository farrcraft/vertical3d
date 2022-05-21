/**
 * (c) Joshua Farr <j.wgasa@gmail.com>
 */

#include "BitmapFont.h"
#include "../image/ImageFactory.h"
#include "../image/Texture.h"

#include <fstream>
#include <sstream>

using namespace v3D;

std::pair<std::string, std::string> tokenize(const std::string & token)
{
	size_t pos = token.find('=');
	std::string key = token.substr(0, pos);
	std::string value = token.substr(pos + 1);
	std::pair<std::string, std::string> kv(key, value);
	return kv;
}

BitmapFont::BitmapFont(const std::string & path, std::string & name)
{
	std::string filename = path + name + std::string(".fnt");
	loadCharset(filename);
	filename = path + charset_.fileName_;
	if (!loadTexture(filename))
	{
		std::string err = std::string("Error opening font image: ") + filename;
		throw std::runtime_error(err);
	}
}

void BitmapFont::loadCharset(const std::string & filename)
{
	std::ifstream file(filename.c_str(), std::ios::in | std::ios::binary);
	if (!file)
	{
		std::string err = std::string("Error opening font file: ") + filename;
		throw std::runtime_error(err);
	}

	std::string line;
	std::string token;

	while (!file.eof())
	{
		std::stringstream str;
		std::getline(file, line);
		str << line;
		str >> token;
		// first token in line indicates the type of line - one of: info, common, page, chars, char
		if (token == "info") // font info
		{
		}
		else if (token == "common") // common data
		{
			while (!str.eof())
			{
				str >> token;
				std::pair<std::string, std::string> kv = tokenize(token);
				std::stringstream converter;
				converter << kv.second;
				if (kv.first == "lineHeight")
				{
					converter >> charset_.lineHeight_;
				}
				else if (kv.first == "base")
				{
					converter >> charset_.base_;
				}
				else if (kv.first == "scaleW")
				{
					converter >> charset_.width_;
				}
				else if (kv.first == "scaleH")
				{
					converter >> charset_.height_;
				}
				else if (kv.first == "pages")
				{
					converter >> charset_.pages_;
				}
				else if (kv.first == "packed")
				{
				}
				else if (kv.first == "alphaChnl")
				{
				}
				else if (kv.first == "redChnl")
				{
				}
				else if (kv.first == "greenChnl")
				{
				}
				else if (kv.first == "blueChnl")
				{
				}
			}
		}
		else if (token == "page") // page # & associated bitmap filename
		{
			while (!str.eof())
			{
				str >> token;
				std::pair<std::string, std::string> kv = tokenize(token);
				if (kv.first == "id")
				{
				}
				else if (kv.first == "file")
				{
					charset_.fileName_ = kv.second.substr(1, kv.second.length() - 2); // get raw filename from quoted value
				}
			}
		}
		else if (token == "chars") // # of chars in font
		{
			while (!str.eof())
			{
				str >> token;
				std::pair<std::string, std::string> kv = tokenize(token);
				std::stringstream converter;
				converter << kv.second;
				if (kv.first == "count")
				{
					/*
					size_t chars;
					converter >> chars;
					charset_.chars_.resize(chars);
					*/
				}
			}
		}
		else if (token == "char") // character data
		{
			unsigned short id = 0;
			CharDescriptor desc;
			while (!str.eof())
			{
				str >> token;
				std::pair<std::string, std::string> kv = tokenize(token);
				std::stringstream converter;
				converter << kv.second;
				if (kv.first == "id")
				{
					converter >> id;
				}
				else if (kv.first == "x")
				{
					converter >> desc.x_;
				}
				else if (kv.first == "y")
				{
					converter >> desc.y_;
				}
				else if (kv.first == "width")
				{
					converter >> desc.width_;
				}
				else if (kv.first == "height")
				{
					converter >> desc.height_;
				}
				else if (kv.first == "xoffset")
				{
					converter >> desc.xOffset_;
				}
				else if (kv.first == "yoffset")
				{
					converter >> desc.yOffset_;
				}
				else if (kv.first == "xadvance")
				{
					converter >> desc.xAdvance_;
				}
				else if (kv.first == "page")
				{
					converter >> desc.page_;
				}
				else if (kv.first == "chnl")
				{
				}
			}
			charset_.chars_[id] = desc;
		}
	}
}

BitmapFont::CharDescriptor BitmapFont::character(char c)
{
	return charset_.chars_[c];
}

boost::shared_ptr<Texture> BitmapFont::texture()
{
	return texture_;
}

unsigned short BitmapFont::charsetWidth() const
{
	return charset_.width_;
}

unsigned short BitmapFont::charsetHeight() const
{
	return charset_.height_;
}

unsigned short BitmapFont::lineHeight() const
{
	return charset_.lineHeight_;
}

bool BitmapFont::loadTexture(const std::string & filename)
{
	v3D::ImageFactory factory;
	boost::shared_ptr<v3D::Image> image;

	try
	{
		image = factory.read(filename);
	}
	catch (std::string & e)
	{
		return false;
	}

	texture_.reset(new Texture(image));

	return true;
}
