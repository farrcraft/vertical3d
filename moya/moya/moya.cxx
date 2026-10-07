/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/log/Logger.h>
#include <api/render/offline/Sampling.h>
#include <api/render/offline/rib/Reader.h>
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

/**
 * The picture, grid and bucket sizes the command line names, each checked before it is used.
 * A size the renderer refuses is reported, and the scene's own size or the default is kept.
 **/
void sizes(const boost::program_options::variables_map& var_map, v3d::moya::RIBHandler* handler) {
    const bool width = var_map.count("width") > 0;
    const bool height = var_map.count("height") > 0;
    // a picture within the largest side can still be larger than memory holds, and main
    // reports the allocation that fails
    if (width && height) {
        const int x = var_map["width"].as<int>();
        const int y = var_map["height"].as<int>();
        if (!handler->resolution(x, y)) {
            std::cout << "--width and --height have to be between 1 and " << v3d::render::offline::largestResolution
                << ", so the scene's Format is used" << "\n";
        }
    } else if (width || height) {
        std::cout << "--width and --height have to be given together, so the scene's Format is used" << "\n";
    }
    // the command line sets the grid and bucket sizes before the scene is read, so a scene
    // that names its own with Option "limits" replaces them
    const unsigned int largest = v3d::moya::RIBHandler::largestLimit;
    if (var_map.count("grid")) {
        const int grid = var_map["grid"].as<int>();
        if (grid < 1 || static_cast<unsigned int>(grid) > largest) {
            std::cout << "--grid has to be between 1 and " << largest << ", so the default is used" << "\n";
        } else {
            handler->context().gridSize(static_cast<unsigned int>(grid));
        }
    }
    if (var_map.count("bucket")) {
        const int bucket = var_map["bucket"].as<int>();
        if (bucket < 1 || static_cast<unsigned int>(bucket) > largest) {
            std::cout << "--bucket has to be between 1 and " << largest << ", so the default is used" << "\n";
        } else {
            handler->context().bucketSize(static_cast<unsigned int>(bucket), static_cast<unsigned int>(bucket));
        }
    }
}

int run(int argc, char *argv[]) {
    // setup option parser
    boost::program_options::options_description opts_desc("Allowed options");
    opts_desc.add_options()
        ("help", "produce help message")
        ("version", "display version info")
        ("file", boost::program_options::value<std::string>(), "input filename to be rendered")
        ("output", boost::program_options::value<std::string>(), "filename to be written")
        ("width", boost::program_options::value<int>(), "image width, replacing the scene's Format; needs --height")
        ("height", boost::program_options::value<int>(), "image height, replacing the scene's Format; needs --width")
        ("silent", "print no progress line")
        ("grid", boost::program_options::value<int>(),
            "micropolygon grid size, used unless the scene sets one with Option \"limits\"")
        ("bucket", boost::program_options::value<int>(),
            "square bucket size in pixels, used unless the scene sets one with Option \"limits\"");

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
    // start and end of a file - so the handler creates and destroys the context
    boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);

    if (!outfile.empty()) {
        handler.output(outfile);
    }
    sizes(var_map, &handler);

    // flushed rather than left to the buffer: the render that follows takes the rest of the
    // run, and the progress line has to appear before it starts
    if (!var_map.count("silent")) {
        std::cout << "Rendering scene file: " << infile << "\n" << std::flush;
    }

    v3d::render::offline::rib::Reader reader(logger);
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
    // the stream the rest of the file writes to, because a last-resort handler must not throw
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
