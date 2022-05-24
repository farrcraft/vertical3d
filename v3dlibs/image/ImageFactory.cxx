/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "ImageFactory.h"

#include "reader/TGAReader.h"
#include "reader/BMPReader.h"
#include "reader/PNGReader.h"
#include "reader/JPEGReader.h"

#include "writer/TGAWriter.h"
#include "writer/BMPWriter.h"
#include "writer/PNGWriter.h"
#include "writer/JPEGWriter.h"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/log/trivial.hpp>
#include <boost/make_shared.hpp>

#include <iostream>

using namespace v3d::image;

ImageFactory::ImageFactory() {
	add("tga", boost::make_shared<reader::TGAReader>());
	add("bmp", boost::make_shared<reader::BMPReader>());
	add("png", boost::make_shared<reader::PNGReader>());
	add("jpg", boost::make_shared<reader::JPEGReader>());

	add("tga", boost::make_shared<writer::TGAWriter>());
	add("bmp", boost::make_shared<writer::BMPWriter>());
	add("png", boost::make_shared<writer::PNGWriter>());
	add("jpg", boost::make_shared<writer::JPEGWriter>());
}

void ImageFactory::add(const std::string & name, const boost::shared_ptr<ImageReader> & reader) {
	readers_[name] = reader;
}

void ImageFactory::add(const std::string & name, const boost::shared_ptr<ImageWriter> & writer) {
	writers_[name] = writer;
}

bool ImageFactory::write(const std::string & filename, const boost::shared_ptr<Image> & img) {
	boost::filesystem::path full_path = boost::filesystem::system_complete(filename);

	std::string ext = filename.substr(filename.length() - 3);
	std::map<std::string, boost::shared_ptr<ImageWriter> >::iterator it = writers_.find(ext);
	if (it != writers_.end()) {
		boost::shared_ptr<ImageWriter> writer = (*it).second;
		return writer->write(full_path.string(), img);
	}
	return false;
}

boost::shared_ptr<Image> ImageFactory::read(const std::string & filename) {
	boost::filesystem::path full_path = boost::filesystem::system_complete(filename);
	std::string filepath = full_path.string();

	std::string ext = filename.substr(filename.length() - 3);

	BOOST_LOG_TRIVIAL(debug) << "ImageFactory::read - reading file [" << filename << "] with reader bound to extension [" << ext << "] from path [" << filepath << "]";
	
	std::map<std::string, boost::shared_ptr<ImageReader> >::iterator it = readers_.find(ext);
	if (it != readers_.end()) {
		boost::shared_ptr<ImageReader> reader = (*it).second;
		return reader->read(filepath);
	}
	boost::shared_ptr<Image> empty_ptr;
	BOOST_LOG_TRIVIAL(debug) << "ImageFactory::read - no reader exists for detected image format!";
	return empty_ptr;
}

