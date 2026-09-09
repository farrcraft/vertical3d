/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/offline/RIBReader.h>
#include <moya/libmoya/RIBHandler.h>
#include <moya/libmoya/Renderer.h>

#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

#include <boost/make_shared.hpp>
#include <boost/program_options.hpp>

namespace {

int run(int argc, char *argv[]) {
    // setup option parser
    boost::program_options::options_description opts_desc("Allowed options");
    opts_desc.add_options()
        ("help", "produce help message")
        ("version", "display version info")
        ("file", boost::program_options::value<std::string>(), "input filename to be rendered")
        ("output", boost::program_options::value<std::string>(), "filename to be written")
        ("grid", boost::program_options::value<int>(), "micropolygon grid size")
        ("bucket", boost::program_options::value<int>(), "nXm pixel bucket size");

    // parse options
    boost::program_options::variables_map var_map;
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, opts_desc), var_map);
    boost::program_options::notify(var_map);

    // process options
    if (var_map.count("help")) {
        std::cout << opts_desc << "\n";
        exit(EXIT_SUCCESS);
    }

    if (var_map.count("version")) {
        std::cout << "Moya v0.0.1" << "\n";
        std::cout << "The RenderMan (R) Interface Procedures and Protocol are:" << "\n" <<
                     "Copyright 1988, 1989, Pixar" << "\n" <<
                     "All Rights Reserved" << "\n";

        exit(EXIT_SUCCESS);
    }

    std::string infile;
    std::string outfile;

    if (var_map.count("file")) {
        infile = var_map["file"].as<std::string>();
    }
    if (var_map.count("output")) {
        outfile = var_map["output"].as<std::string>();
    }

    if (infile.empty()) {
        std::cout << opts_desc << "\n";
        exit(EXIT_SUCCESS);
    }

    // RiBegin and RiEnd have no RIB equivalent - the standard says they are implied at the
    // start and end of a file - so the handler is what creates and destroys the context
    boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);

    if (!outfile.empty()) {
        handler.output(outfile);
    }
    // the grid and bucket sizes a scene names are Option "limits", and the command line
    // overrides them by being applied first and re-applied after
    if (var_map.count("grid")) {
        handler.context().gridSize(static_cast<unsigned int>(var_map["grid"].as<int>()));
    }
    if (var_map.count("bucket")) {
        const unsigned int size = static_cast<unsigned int>(var_map["bucket"].as<int>());
        handler.context().bucketSize(size, size);
    }

    // flushed rather than left to the buffer: the render that follows it is the whole
    // run, and a progress line nobody sees until the picture is written is not one
    std::cout << "Rendering scene file: " << infile << "\n" << std::flush;

    v3d::render::offline::RIBReader reader(logger);
    if (!reader.read(infile, &handler)) {
        std::cout << "error reading rib file - " << reader.error() << "\n";
        exit(EXIT_FAILURE);
    }

    // the picture was written by RiWorldEnd, which is where the RI standard puts it
    return EXIT_SUCCESS;
}

};  // namespace

int main(int argc, char *argv[]) {
    // the option parser and the reader both report by throwing, and an exception leaving
    // main is an abort with no message in it. The handler reports through stdio rather than
    // the stream the rest of the file writes to: a last resort that can itself throw is not
    // one
    try {
        return run(argc, argv);
    } catch (const std::exception& error) {
        std::fputs("error: ", stderr);
        std::fputs(error.what(), stderr);
        std::fputs("\n", stderr);
        return EXIT_FAILURE;
    } catch (...) {
        std::fputs("error: unrecognised failure\n", stderr);
        return EXIT_FAILURE;
    }
}
