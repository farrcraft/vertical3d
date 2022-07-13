/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

/**
 * TODO:
 * ( ) base rendering algorithm
 * ( ) render background color
 * ( ) input scene file format (xml-based)
 *		( ) read image dimensions from scene file
 *		( ) read scene background color from scene file
 * ( ) support renderman api
 * ( ) ray intersection algorithms
 * ( ) read sphere object from scene file
 * ( ) render sphere
 * ( ) render simple polygon
 * ( ) phong shading
 * ( ) blinn shading
 * ( ) gourad shading
 * ( ) texture mapping
 * ( ) render reflections
 * ( ) render shadows
 * ( ) render point light
 * ( ) render spot light
 * ( ) render fog
 * ( ) bump mapping
 * ( ) camera definitions
 * ( ) heightfields
 * ( ) photon mapping
 * ( ) specular highlighting
 * ( ) phong highlighting
 * ( ) radiosity
 * ( ) render plane
 * ( ) index of refraction
 * ( ) antialiasing
 * ( ) adaptive supersampling
 * ( ) spacial subdivisions
 * ( ) smooth triangles
 * ( ) transparency
 * ( ) lighting models
 * ( ) integrate with moya
 *
 * 
 * The intermediate goal for talyn is that it function as a standalone raytracer. It should eventually
 * become the raytracing component of moya. It may make sense to build up the core functionality of
 * moya at the same time. This could either be done in parallel or each could be built as a modular
 * rendering plugin. The common rendering code could be located in a libv3drender library.
 *
 * The immediate short term goal is to get a bare essential renderer running and properly functioning. 
 * It should be a fairly minimal implementation of the core raytracing algorithm. Once this is done, it 
 * should be possible to build up the same functionality in moya. This should probably be done in a completely
 * new branch from a fresh code base separate from the original moya project. Common code should be carved 
 * out of talyn then. Any useful pieces from the old moya code base can be rolled in as necessary.
 *
 * How the two renders are unified into the common rendering interface will depend partly on how different
 * their results are. One possibility is that ray tracing is supported only through the shading language's
 * ray tracing capabilities (e.g. the trace shader method).
 *
 * There are two different renderers: 
 *		talyn - raytracing algoritm
 *		moya - reyes algorithm
 * They are both available through the renderman interface.
 */

/*
http://fuzzyphoton.tripod.com/rtalgo.htm
http://www.siggraph.org/education/materials/HyperGraph/raytrace/rtracewr.htm
http://books.google.com/books?id=bBOxUmw83jUC&pg=PA160&lpg=PA160&dq=raytracing+algorithm&source=web&ots=BipKf5Qm84&sig=--x6d4rP-OPcALY81HXgtKubVR8&hl=en&sa=X&oi=book_result&resnum=7&ct=result#PPA160,M1
*/

#include <iostream>

#include "RIBReader.h"
#include "RenderContext.h"

#include "../../v3dlibs/image/Factory.h"

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
        width = var_map["height"].as<unsigned int>();
    }

    if (infile.empty()) {
        usage();
        exit(EXIT_SUCCESS);
    }

    // establish new render context
    boost::shared_ptr<Talyn::RenderContext> rc(new Talyn::RenderContext());

    // construct full fs path of source scene file
    boost::filesystem::path full_path = boost::filesystem::system_complete(infile);
    std::string filepath = full_path.string();

    // determine the filetype of infile based on file extension
    std::string ext = infile.substr(infile.length() - 3);

    if (ext == "rib") {  // .rib for renderman formatted files.
        Talyn::RIBReader reader(rc);
        if (!reader.read(filepath)) {
            std::cout << "error reading rib file!" << std::endl;
            exit(EXIT_FAILURE);
        }
    } else {
        std::cout << "unable to determine file type!" << std::endl;
        exit(EXIT_FAILURE);
    }

    // override any file size specs if they were provided on the command line
    if (width > 0 && height > 0) {
        rc->format(width, height);
    }

    if (!silent) {
        std::cout << "Rendering scene file: " << filepath << std::endl;
    }

    // actually do the rendering
    rc->render();

    boost::shared_ptr<Talyn::FrameBuffer> fb = rc->framebuffer();

    if (!silent) {
        std::cout << "Rendering framebuffer..." << std::endl;
    }
    // framebuffer conversion to a writable image
    boost::shared_ptr<v3d::image::Image> image = fb->render();
    auto logger = boost::make_shared<v3d::core::Logger>();
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

