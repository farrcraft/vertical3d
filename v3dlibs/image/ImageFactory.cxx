#include "ImageFactory.h"

#include "readers/TGAReader.h"
#include "readers/BMPReader.h"
#include "readers/PNGReader.h"
#include "readers/JPEGReader.h"

#include "writers/TGAWriter.h"
#include "writers/BMPWriter.h"
#include "writers/PNGWriter.h"
#include "writers/JPEGWriter.h"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/log/trivial.hpp>

#include <iostream>

using namespace v3D;

ImageFactory::ImageFactory()
{
	boost::shared_ptr<ImageReader> reader;
	reader.reset(new TGAReader());
	add("tga", reader);
	reader.reset(new BMPReader());
	add("bmp", reader);
	reader.reset(new PNGReader());
	add("png", reader);
	reader.reset(new JPEGReader());
	add("jpg", reader);

	boost::shared_ptr<ImageWriter> writer;
	writer.reset(new TGAWriter());
	add("tga", writer);
	writer.reset(new BMPWriter());
	add("bmp", writer);
	writer.reset(new PNGWriter());
	add("png", writer);
	writer.reset(new JPEGWriter());
	add("jpg", writer);
}

ImageFactory::~ImageFactory()
{
}

void ImageFactory::add(const std::string & name, const boost::shared_ptr<ImageReader> & reader)
{
	readers_[name] = reader;
}

void ImageFactory::add(const std::string & name, const boost::shared_ptr<ImageWriter> & writer)
{
	writers_[name] = writer;
}

bool ImageFactory::write(const std::string & filename, const boost::shared_ptr<Image> & img)
{
	boost::filesystem::path full_path = boost::filesystem::system_complete(filename);

	std::string ext = filename.substr(filename.length() - 3);
	std::map<std::string, boost::shared_ptr<ImageWriter> >::iterator it = writers_.find(ext);
	if (it != writers_.end())
	{
		boost::shared_ptr<ImageWriter> writer = (*it).second;
		return writer->write(full_path.string(), img);
	}
	return false;
}

boost::shared_ptr<Image> ImageFactory::read(const std::string & filename)
{
	boost::filesystem::path full_path = boost::filesystem::system_complete(filename);
	std::string filepath = full_path.string();

	std::string ext = filename.substr(filename.length() - 3);

	BOOST_LOG_TRIVIAL(debug) << "ImageFactory::read - reading file [" << filename << "] with reader bound to extension [" << ext << "] from path [" << filepath << "]";
	
	std::map<std::string, boost::shared_ptr<ImageReader> >::iterator it = readers_.find(ext);
	if (it != readers_.end())
	{
		boost::shared_ptr<ImageReader> reader = (*it).second;
		return reader->read(filepath);
	}
	boost::shared_ptr<Image> empty_ptr;
	BOOST_LOG_TRIVIAL(debug) << "ImageFactory::read - no reader exists for detected image format!";
	return empty_ptr;
}

