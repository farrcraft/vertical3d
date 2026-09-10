/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/image/Factory.h>
#include <api/render/offline/rib/Reader.h>
#include <talyn/libtalyn/RIBHandler.h>
#include <talyn/libtalyn/RenderContext.h>

#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

#include <boost/lexical_cast.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/program_options.hpp>
#include <boost/make_shared.hpp>

void usage() {
    std::cout << "usage:" << "\n";
    std::cout << "talyn --file filename --outfile filename --silent -q --debug -w 640 -h 480" << "\n";
    std::cout << "  --debug     - enable debugging output" << "\n";
    std::cout << "  --silent    - don't print the render progress" << "\n";
    std::cout << "  --quiet     - don't print informational messages" << "\n";
    std::cout << "  --file      - input filename of scene to be rendered" << "\n";
    std::cout << "  --outfile   - target filename of rendered image" << "\n";
    std::cout << "  --width     - override scene formatting for the width of the target rendered image" << "\n";
    std::cout << "  --height    - override scene formatting for the height of the target rendered image" << "\n";
}

namespace {

/**
 * What the command line asked for. --debug and --quiet are accepted and read nowhere:
 * nothing in this renderer varies on them.
 **/
struct Options final {
    bool silent = false;
    std::string infile;
    std::string outfile;
    unsigned int width = 0;
    unsigned int height = 0;
};

/**
 * @return false when the command line asked for the usage rather than for a render
 **/
bool parseOptions(int argc, char * argv[], Options * options) {
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
        std::cout << opts_desc << "\n";
        return false;
    }

    if (var_map.count("silent")) {
        options->silent = true;
    }
    if (var_map.count("file")) {
        options->infile = var_map["file"].as<std::string>();
    }
    if (var_map.count("outfile")) {
        options->outfile = var_map["outfile"].as<std::string>();
    }
    if (var_map.count("width")) {
        options->width = var_map["width"].as<unsigned int>();
    }
    if (var_map.count("height")) {
        options->height = var_map["height"].as<unsigned int>();
    }

    if (options->infile.empty()) {
        usage();
        return false;
    }
    return true;
}

int run(int argc, char * argv[]) {
    Options options;
    if (!parseOptions(argc, argv, &options)) {
        return EXIT_SUCCESS;
    }
    const bool silent = options.silent;
    const std::string& infile = options.infile;
    const std::string& outfile = options.outfile;
    const unsigned int width = options.width;
    const unsigned int height = options.height;

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
        v3d::render::offline::rib::Reader reader(logger);
        if (!reader.read(filepath, &handler)) {
            std::cout << "error reading rib file - " << reader.error() << "\n";
            exit(EXIT_FAILURE);
        }
        // the file parsed, but it may have asked for a camera this renderer cannot build
        if (!handler.error().empty()) {
            std::cout << "cannot render this scene - " << handler.error() << "\n";
            exit(EXIT_FAILURE);
        }
    } else {
        std::cout << "unable to determine file type!" << "\n";
        exit(EXIT_FAILURE);
    }

    // override any file size specs if they were provided on the command line
    if (width > 0 && height > 0) {
        rc->format(width, height);
    } else if (width > 0 || height > 0) {
        // format() sizes both dimensions at once and the scene's own value for the other one
        // is not readable back, so a lone override cannot be honoured
        std::cout << "--width and --height must be given together to override the scene" << "\n";
    }

    if (!silent) {
        // flushed rather than left to the buffer: the render that follows it is the
        // whole run, and a progress line nobody sees until it ends is not one
        std::cout << "Rendering scene file: " << filepath << "\n" << std::flush;
    }

    // actually do the rendering
    rc->render();

    boost::shared_ptr<v3d::render::offline::FrameBuffer> fb = rc->framebuffer();
    if (!fb) {
        // the framebuffer is allocated by the scene's Format request, and a scene that names
        // no format leaves nothing to write
        std::cout << "scene did not set an image format!" << "\n";
        exit(EXIT_FAILURE);
    }

    if (!silent) {
        std::cout << "Rendering framebuffer..." << "\n" << std::flush;
    }
    // framebuffer conversion to a writable image
    boost::shared_ptr<v3d::image::Image> image = fb->image(4);
    if (!outfile.empty()) {
        if (!silent) {
            std::cout << "Writing image file: " << outfile << "\n";
        }
        v3d::image::Factory factory(logger);
        if (!factory.write(outfile, image)) {
            std::cout << "error writing file!" << "\n";
            exit(EXIT_FAILURE);
        }
    }
    return EXIT_SUCCESS;
}

};  // namespace

int main(int argc, char * argv[]) {
    // the option parser and the reader both report by throwing, and an exception leaving
    // main is an abort with no message in it. The handler reports through stdio rather
    // than the stream the rest of the file writes to: a last resort that can itself throw
    // is not one
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

