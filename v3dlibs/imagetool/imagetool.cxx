/**
 * image manipulation tool
 * (c) 2008 Joshua Farr
 * 
 * TODO:
 * (*) main
 * (*) getopt
 * (*) usage
 * (*) determine filetype from filename extension
 * (*) display image info
 * (*) verify reading bmp works properly by writing bmp after reading
 * (*) read 24bit tga
 * (*) read jpeg
 * (*) write jpeg
 * (*) write bmp
 *		(*) write 24bit bmp
 *		(*) write 32bit bmp
 * ( ) write tga
 *		(*) write 24 bit tga
 *		( ) write 24 bit rle tga
 * ( ) read bmp
 *		( ) read 8 bit bmp
 *		( ) read 16 bit bmp
 *		(*) read 24 bit bmp
 *		( ) read 32 bit bmp
 * (*) read png
 *		(*) read 32 bit png
 * (*) write png
 */

#include <iostream>
#include <boost/program_options.hpp>

#include "../image/ImageFactory.h"


int main (int argc, char *argv[])
{
	// setup option parser
	boost::program_options::options_description opts_desc("Allowed options");
	opts_desc.add_options()
		("help", "produce help message")
		("info", "display image info")
		("silent", "don't display any output")
		("file", boost::program_options::value<std::string>(), "input image filename to be read")
		("outfile", boost::program_options::value<std::string>(), "image filename to be written")
	;

	// parse options
	boost::program_options::variables_map var_map;
	boost::program_options::store(boost::program_options::parse_command_line(argc, argv, opts_desc), var_map);
	boost::program_options::notify(var_map);

	// process options
	if (var_map.count("help"))
	{
		std::cout << opts_desc << std::endl;
		exit(EXIT_SUCCESS);
	}

	bool info = false;
	bool silent = false;
	bool sync = false;
	std::string infile;
	std::string outfile;

	if (var_map.count("info"))
	{
		info = true;
	}
	if (var_map.count("silent"))
	{
		silent = true;
	}
	if (var_map.count("file"))
	{
		infile = var_map["file"].as<std::string>();
	}
	if (var_map.count("outfile"))
	{
		outfile = var_map["outfile"].as<std::string>();
	}

	if (infile.empty())
	{
		std::cout << opts_desc << std::endl;
		exit(EXIT_SUCCESS);
	}

	v3d::image::ImageFactory factory;
	boost::shared_ptr<v3d::image::Image> image;

	if (!silent)
	{
		std::cout << "Reading: " << infile << std::endl;
		try
		{
			image = factory.read(infile);
		}
		catch (std::string & e)
		{
			std::cout << "error reading image! - " << e << std::endl;
			exit(EXIT_FAILURE);
		}
		if (!image)
		{
			std::cout << "error reading file!" << std::endl;
			exit(EXIT_FAILURE);
		}
	}
	if (info)
	{
		std::cout << "Source image width: " << image->width() << std::endl;
		std::cout << "Source image height: " << image->height() << std::endl;
		std::cout << "Source image bpp: " << image->bpp() << std::endl;
	}

	if (sync)
	{
		std::cout << "Writing: " << outfile << std::endl;
		if (!factory.write(outfile, image))
		{
			std::cout << "error writing file!" << std::endl;
			exit(EXIT_FAILURE);
		}
	}

	return EXIT_SUCCESS;
}
