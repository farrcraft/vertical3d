#include "../libmoya/RenderMan.h"

#include <cstdlib>
#include <iostream>
#include <boost/program_options.hpp>

int main (int argc, char *argv[])
{
	log4cxx::BasicConfigurator::configure();

	/*
		CLI Options:
			-f / --file	scene.rib
			-o / --output file.ext
			- 									accept input from stdin
			-v / --version					display version and exit
			-h / --help						display help message and exit
	 		-b / --bucket n m				set bucket size to n-by-m pixels
	 		-g / --grid size				set grid size to size
	*/
	// setup option parser
	boost::program_options::options_description opts_desc("Allowed options");
	opts_desc.add_options()
		("help", "produce help message")
		("version", "display version info")
		("file", boost::program_options::value<std::string>(), "input filename to be rendered")
		("output", boost::program_options::value<std::string>(), "filename to be written")
		("grid", boost::program_options::value<int>(), "micropolygon grid size")
		("bucket", boost::program_options::value<int>(), "nXm pixel bucket size")
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

	if (var_map.count("version"))
	{
		std::cout << "Moya v0.0.1" << std::endl;
		std::cout << "The RenderMan (R) Interface Procedures and Protocol are:" << std::endl <<
					 "Copyright 1988, 1989, Pixar" << std::endl << 
					 "All Rights Reserved" << std::endl;

		exit(EXIT_SUCCESS);
	}

	std::string infile;
	std::string outfile;
	std::string bucket_size;
	int grid_size = -1;

	if (var_map.count("file"))
	{
		infile = var_map["file"].as<std::string>();
	}
	if (var_map.count("output"))
	{
		outfile = var_map["output"].as<std::string>();
	}

	/*
	if (infile.empty())
	{
		std::cout << opts_desc << std::endl;
		exit(EXIT_SUCCESS);
	}
	*/

	if (var_map.count("grid"))
	{
		grid_size = var_map["grid"].as<int>();
	}

	if (var_map.count("bucket"))
	{
		bucket_size = var_map["bucket"].as<std::string>();
	}

	// get scene description
	// render scene
	// output rendered image
	/*
		using the Moya API directly:
	
			Moya & moya = Moya::getInstance();
			moya.createRenderContext("");
			moya.prepareWorld();
			??? moya.surface("plastic");
			??? moya.sphere(1, -1, 1, 360);
			moya.render();
			moya.destroyWorld();

		or with the RenderMan C API:
	*/
	RiBegin(RI_NULL);
	//RiBegin("poly.rib");
	RiWorldBegin();
	RiSurface("plastic");
	RtPoint points[4] = { 0.0, 1.0, 0.0,	0.0, 1.0, 1.0,
						  0.0, 0.0, 1.0,	0.0, 0.0, 0.0 };
	RiPolygon(4, RI_P, (RtPointer)points, RI_NULL);
	/*
		gourad shaded:
			RtColor colors[4];
			RiPolygon(4, "P", (RtPointer)points, "Cs", (RtPointer)colors, RI_NULL);

		phong shaded:
			RtPoint normals[4];
			RiPolygon(4, "P", (RtPointer)points, "N", (RtPointer)normals, RI_NULL);
	*/
	//RiSphere(1, -1, 1, 360);
	RiWorldEnd();
	RiEnd();

	return EXIT_SUCCESS;
}
