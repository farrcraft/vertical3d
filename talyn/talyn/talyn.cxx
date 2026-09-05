/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include <iostream>
#include <string>

#include "../libtalyn/RIBHandler.h"
#include "../libtalyn/RenderContext.h"

#include "../../api/image/Factory.h"
#include "../../api/render/offline/RIBReader.h"

#include <boost/lexical_cast.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/program_options.hpp>
#include <boost/make_shared.hpp>

void usage() {
    std::cout << "usage:" << std::endl;
    std::cout << "talyn --file filename --outfile filename --silent -q --debug -w 640 -h 480" << std::endl;
    std::cout << "  --debug     - enable debugging output" << std::endl;
    std::cout << "  --silent    - don't print the render progress" << std::endl;
    std::cout << "  --quiet     - don't print informational messages" << std::endl;
    std::cout << "  --file      - input filename of scene to be rendered" << std::endl;
    std::cout << "  --outfile   - target filename of rendered image" << std::endl;
    std::cout << "  --width     - override scene formatting for the width of the target rendered image" << std::endl;
    std::cout << "  --height    - override scene formatting for the height of the target rendered image" << std::endl;
}

int main(int argc, char * argv[]) {
    // setup option parser
    boost::program_options::options_description opts_desc("Allowed options");
    opts_desc.add_options()
        ("help", "produce help message")
        ("debug", "enable debugging output")
        ("silent", "don't print the render progress")
        ("quiet", "don't print informational messages")
        ("width", boost::program_options::value<unsigned int>(), "override scene formatting for the width of the target rendered image")
        ("height", boost::program_options::value<unsigned int>(), "override scene formatting for the height of the target rendered image")
        ("file", boost::program_options::value<std::string>(), "input filename of scene to be rendered")
        ("outfile", boost::program_options::value<std::string>(), "target filename of rendered image");

    // parse options
    boost::program_options::variables_map var_map;
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, opts_desc), var_map);
    boost::program_options::notify(var_map);

    // process options
    if (var_map.count("help")) {
        std::cout << opts_desc << std::endl;
        exit(EXIT_SUCCESS);
    }

    bool debug = false;
    bool silent = false;
    bool quiet = false;
    std::string infile;
    std::string outfile;
    unsigned int height = 0;
    unsigned int width = 0;

    if (var_map.count("silent")) {
        silent = true;
    }
    if (var_map.count("quiet")) {
        quiet = true;
    }
    if (var_map.count("debug")) {
        debug = true;
    }
    if (var_map.count("file")) {
        infile = var_map["file"].as<std::string>();
    }
    if (var_map.count("outfile")) {
        outfile = var_map["outfile"].as<std::string>();
    }
    if (var_map.count("width")) {
        width = var_map["width"].as<unsigned int>();
    }
    if (var_map.count("height")) {
        height = var_map["height"].as<unsigned int>();
    }

    if (infile.empty()) {
        usage();
        exit(EXIT_SUCCESS);
    }

    // establish new render context
    boost::shared_ptr<v3d::talyn::RenderContext> rc(new v3d::talyn::RenderContext());

    // construct full fs path of source scene file
    boost::filesystem::path full_path = boost::filesystem::system_complete(infile);
    std::string filepath = full_path.string();

    // determine the filetype of infile based on file extension
    std::string ext = infile.substr(infile.length() - 3);

    auto logger = boost::make_shared<v3d::log::Logger>();

    if (ext == "rib") {  // .rib for renderman formatted files.
        v3d::talyn::RIBHandler handler(rc);
        v3d::render::offline::RIBReader reader(logger);
        if (!reader.read(filepath, &handler)) {
            std::cout << "error reading rib file - " << reader.error() << std::endl;
            exit(EXIT_FAILURE);
        }
        // the file parsed, but it may have asked for a camera this renderer cannot build
        if (!handler.error().empty()) {
            std::cout << "cannot render this scene - " << handler.error() << std::endl;
            exit(EXIT_FAILURE);
        }
    } else {
        std::cout << "unable to determine file type!" << std::endl;
        exit(EXIT_FAILURE);
    }

    // override any file size specs if they were provided on the command line
    if (width > 0 && height > 0) {
        rc->format(width, height);
    } else if (width > 0 || height > 0) {
        // format() sizes both dimensions at once and the scene's own value for the other one
        // is not readable back, so a lone override cannot be honoured
        std::cout << "--width and --height must be given together to override the scene" << std::endl;
    }

    if (!silent) {
        std::cout << "Rendering scene file: " << filepath << std::endl;
    }

    // actually do the rendering
    rc->render();

    boost::shared_ptr<v3d::render::offline::FrameBuffer> fb = rc->framebuffer();
    if (!fb) {
        // the framebuffer is allocated by the scene's Format request, and a scene that names
        // no format leaves nothing to write
        std::cout << "scene did not set an image format!" << std::endl;
        exit(EXIT_FAILURE);
    }

    if (!silent) {
        std::cout << "Rendering framebuffer..." << std::endl;
    }
    // framebuffer conversion to a writable image
    boost::shared_ptr<v3d::image::Image> image = fb->image(4);
    if (!outfile.empty()) {
        if (!silent) {
            std::cout << "Writing image file: " << outfile << std::endl;
        }
        v3d::image::Factory factory(logger);
        if (!factory.write(outfile, image)) {
            std::cout << "error writing file!" << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    return EXIT_SUCCESS;
}

