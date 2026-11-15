/*
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 */

#include <api/log/Logger.h>
#include <api/render/offline/film/Sampling.h>
#include <api/render/offline/rib/Reader.h>
#include <api/version/Version.h>
#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/integrate/Render.h>
#include <moya/libmoya/output/Exr.h>
#include <moya/libmoya/output/ExrPart.h>
#include <moya/libmoya/output/Output.h>
#include <moya/libmoya/output/Outputs.h>
#include <moya/libmoya/ri/RIBHandler.h>
#include <moya/libmoya/ri/SceneClient.h>
#include <moya/libmoya/scene/Description.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>
#include <moya/libmoya/texture/Maker.h>
#include <moya/libmoya/view/Hits.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <initializer_list>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/program_options.hpp>

#include <glm/vec2.hpp>

namespace {

/**
 * The picture, grid and bucket sizes the command line names, each checked before it is used.
 * A size the renderer refuses is reported, and the scene's own size or the default is kept.
 **/
void sizes(const boost::program_options::variables_map& var_map, v3d::moya::ri::RIBHandler* handler) {
    const bool width = var_map.count("width") > 0;
    const bool height = var_map.count("height") > 0;
    // a picture within the largest side can still be larger than memory holds. main catches
    // and reports the allocation that fails
    if (width && height) {
        const int x = var_map["width"].as<int>();
        const int y = var_map["height"].as<int>();
        if (!handler->resolution(x, y)) {
            std::cout << "--width and --height have to be between 1 and " << v3d::render::offline::film::largestResolution
                << ", so the scene's Format is used" << "\n";
        }
    } else if (width || height) {
        std::cout << "--width and --height have to be given together, so the scene's Format is used" << "\n";
    }
    // the command line pins the grid and bucket sizes before the scene is read, so a scene
    // that names its own with Option "limits" keeps the command line's
    const unsigned int largest = v3d::moya::ri::RIBHandler::largestLimit;
    if (var_map.count("grid") != 0) {
        const int grid = var_map["grid"].as<int>();
        if (grid < 1 || static_cast<unsigned int>(grid) > largest) {
            std::cout << "--grid has to be between 1 and " << largest << ", so the scene's Option \"limits\" or the default is used" << "\n";
        } else {
            handler->context().pinGridSize(static_cast<unsigned int>(grid));
        }
    }
    if (var_map.count("bucket") != 0) {
        const int bucket = var_map["bucket"].as<int>();
        if (bucket < 1 || static_cast<unsigned int>(bucket) > largest) {
            std::cout << "--bucket has to be between 1 and " << largest << ", so the scene's Option \"limits\" or the default is used" << "\n";
        } else {
            handler->context().pinBucketSize(static_cast<unsigned int>(bucket), static_cast<unsigned int>(bucket));
        }
    }
}

/**
 * Why the options of --maketexture cannot be used together, or empty when they can.
 *
 * --maketexture reads no scene, so every option that reads or renders one is refused with it,
 * and it names two files. --swrap, --twrap, --filter, --swidth, --twidth and --colorspace say how
 * a texture is made, so each is refused without it.
 **/
std::string textureConflict(const boost::program_options::variables_map& var_map) {
    if (var_map.count("maketexture") == 0) {
        for (const char* const option : { "swrap", "twrap", "filter", "swidth", "twidth", "colorspace" }) {
            if (var_map.count(option) > 0) {
                return "--" + std::string(option) + " can only be given with --maketexture";
            }
        }
        return {};
    }
    for (const char* const option : { "file", "output", "width", "height", "scene", "hits", "paths", "checkpoint", "checkpoint-interval", "resume",
             "denoise", "grid", "bucket" }) {
        if (var_map.count(option) > 0) {
            return "--" + std::string(option) + " cannot be given with --maketexture, which reads no scene";
        }
    }
    if (var_map["maketexture"].as<std::vector<std::string>>().size() != 2) {
        return "--maketexture names two files, the image and the texture made from it";
    }
    return {};
}

/**
 * Why --hits or --paths cannot write the --output named, or empty when it can. --hits writes an
 * EXR file, and --paths an EXR or a PNG file, named in any case.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the mode, then the file it writes
std::string outputConflict(const std::string & mode, std::string name) {
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char letter) { return static_cast<char>(std::tolower(letter)); });
    const auto ends = [&name](const std::string & extension) {
        return name.size() >= extension.size() && name.compare(name.size() - extension.size(), extension.size(), extension) == 0;
    };
    if (mode == "--paths" && !ends(".exr") && !ends(".png")) {
        return "--output has to name an .exr or a .png file with --paths, which writes an EXR or a PNG file";
    }
    if (mode == "--hits" && !ends(".exr")) {
        return "--output has to name an .exr file with --hits, which writes an EXR file";
    }
    return {};
}

/**
 * Why the options cannot be used together, or empty when they can.
 *
 * --scene, --hits and --paths each read the scene into the new core in place of rendering it
 * with the hiders, so only one of them can be given. Each takes the picture's size from the
 * scene's Format, and none renders in grids or buckets. --width, --height, --grid and --bucket
 * are refused with any of them. --scene writes no file, so --output is refused with it. --hits
 * writes an EXR file, so its --output has to end in .exr, in any case. --paths writes an EXR or a
 * PNG file, so its --output has to end in .exr or .png.
 *
 * Only the path tracer keeps a checkpoint, so --checkpoint, --checkpoint-interval and --resume
 * are refused without --paths, and so is --denoise, which denoises the path tracer's picture. The
 * interval and --resume each need --checkpoint, which names the file, and the interval cannot be
 * below zero.
 **/
std::string conflict(const boost::program_options::variables_map& var_map) {
    for (const char* const option : { "checkpoint", "checkpoint-interval", "resume" }) {
        if (var_map.count(option) > 0 && var_map.count("paths") == 0) {
            return "--" + std::string(option) + " can only be given with --paths, whose path tracer keeps a checkpoint";
        }
    }
    for (const char* const option : { "checkpoint-interval", "resume" }) {
        if (var_map.count(option) > 0 && var_map.count("checkpoint") == 0) {
            return "--" + std::string(option) + " needs --checkpoint, which names the checkpoint";
        }
    }
    if (var_map.count("checkpoint-interval") > 0 && var_map["checkpoint-interval"].as<double>() < 0.0) {
        return "--checkpoint-interval cannot be below zero";
    }
    if (var_map.count("denoise") > 0 && var_map.count("paths") == 0) {
        return "--denoise can only be given with --paths, whose path tracer's picture it denoises";
    }
    std::vector<std::string> modes;
    for (const char* const mode : { "scene", "hits", "paths" }) {
        if (var_map.count(mode) > 0) {
            modes.push_back("--" + std::string(mode));
        }
    }
    if (modes.size() > 1) {
        return modes[0] + " and " + modes[1] + " cannot be given together";
    }
    if (modes.empty()) {
        return {};
    }
    const std::string & mode = modes.front();
    const bool scene = mode == "--scene";
    for (const char* const option : { "width", "height", "grid", "bucket" }) {
        if (var_map.count(option) > 0) {
            return "--" + std::string(option) + " cannot be given with " + mode + ", which takes the scene's own Format";
        }
    }
    if (scene && var_map.count("output") > 0) {
        return "--output cannot be given with --scene, which prints the scene and writes no file";
    }
    if (!scene && var_map.count("output") > 0) {
        return outputConflict(mode, var_map["output"].as<std::string>());
    }
    return {};
}

/**
 * Read a file into the new core's scene, and print the scene's description after each frame
 * the file commits, in place of rendering it.
 **/
int describe(const std::string & infile, const boost::shared_ptr<v3d::log::Logger> & logger) {
    v3d::moya::scene::Scene scene(v3d::moya::scene::kinds());
    v3d::moya::ri::SceneClient client(scene, logger);
    client.frame([&scene]() {
        scene.apply();
        std::cout << v3d::moya::scene::describe(scene) << std::flush;
    });
    v3d::render::offline::rib::Reader reader(logger);
    if (!reader.read(infile, &client)) {
        std::cout << "error reading rib file - " << reader.error() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

/**
 * Read a file into the new core's scene, and write its hit view after each frame the file
 * commits, in place of rendering it. A file of several frames leaves the last frame's.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the file read, then the file written
int showHits(const std::string & infile, const std::string & outfile, const boost::shared_ptr<v3d::log::Logger> & logger) {
    v3d::moya::scene::Scene scene(v3d::moya::scene::kinds());
    v3d::moya::ri::SceneClient client(scene, logger);
    v3d::moya::device::CpuDevice device;
    client.frame([&]() {
        scene.apply();
        v3d::moya::view::writeHits(outfile, v3d::moya::view::hits(device, scene, logger));
    });
    v3d::render::offline::rib::Reader reader(logger);
    if (!reader.read(infile, &client)) {
        std::cout << "error reading rib file - " << reader.error() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

/**
 * The outputs a frame writes: the scene's, with the first one's file replaced by outfile when it
 * is given. A scene with no output writes its beauty to outfile, or to paths.exr, in floats, with
 * the beauty denoised beside it when the options ask for it.
 **/
std::vector<v3d::moya::output::Output> outputsWritten(const v3d::moya::scene::Scene & scene, const std::string & outfile) {
    std::vector<v3d::moya::output::Output> outputs = v3d::moya::output::outputsOf(scene);
    if (outputs.empty()) {
        std::vector<std::string> channels;
        if (v3d::moya::output::denoisesBeauty(scene)) {
            channels.push_back(v3d::moya::output::denoisedOf(std::string(v3d::moya::output::kBeauty)));
        }
        outputs.push_back({ .file = outfile.empty() ? std::string("paths.exr") : outfile,
            .format = v3d::moya::output::Format::Exr,
            .channels = channels,
            .precision = v3d::moya::output::Precision::Float,
            .compression = v3d::moya::output::Compression::Zip });
    } else if (!outfile.empty()) {
        outputs.front().file = outfile;
    }
    if (!outfile.empty()) {
        outputs.front().format = v3d::moya::output::formatOfFile(outfile);
    }
    return outputs;
}

/**
 * Read a file into the new core's scene, and write the outputs it names from the picture its
 * integrator renders after each frame the file commits, in place of rendering it with the hiders.
 * When denoising, each frame's options ask for the beauty denoised.
 * A file of several frames leaves the last frame's. A checkpoint holds one frame, so a resume
 * continues the first frame, and each frame after it starts afresh and writes its checkpoint over
 * the one before.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the file read, then the file written
int renderPaths(const std::string & infile, const std::string & outfile, const v3d::moya::integrate::Checkpoints & checkpoints, bool denoising,
    const boost::shared_ptr<v3d::log::Logger> & logger) {
    v3d::moya::scene::Scene scene(v3d::moya::scene::kinds());
    v3d::moya::ri::SceneClient client(scene, logger);
    v3d::moya::device::CpuDevice device;
    const v3d::moya::colour::Config config;
    v3d::moya::integrate::Checkpoints kept = checkpoints;
    client.frame([&]() {
        scene.apply();
        if (denoising) {
            std::vector<v3d::moya::scene::Handle> options = scene.objects("options");
            if (options.empty()) {
                options.push_back(scene.create("options", "options"));
            }
            scene.set(options.front(), "denoise", v3d::moya::scene::Value::integer(1));
            scene.commit();
            scene.apply();
        }
        const v3d::moya::film::Snapshot picture = v3d::moya::integrate::render(device, scene, logger, config, kept);
        kept.resume = false;
        for (const v3d::moya::output::Output & output : outputsWritten(scene, outfile)) {
            v3d::moya::output::write(output, picture, config, logger);
        }
    });
    v3d::render::offline::rib::Reader reader(logger);
    if (!reader.read(infile, &client)) {
        std::cout << "error reading rib file - " << reader.error() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

/**
 * Make the texture file --maketexture names from its image, with the wrap modes, filter, widths and
 * colour space the options give. Without them a texture is black beyond its edges, made with a box
 * of width one, and read in the image's own colour space.
 **/
int makeTexture(const boost::program_options::variables_map& var_map) {
    const std::vector<std::string> files = var_map["maketexture"].as<std::vector<std::string>>();
    v3d::moya::texture::Making making;
    for (const auto& [option, wrap] : { std::pair { "swrap", &making.swrap }, std::pair { "twrap", &making.twrap } }) {
        if (var_map.count(option) > 0 && !v3d::moya::texture::wrapNamed(var_map[option].as<std::string>(), wrap)) {
            std::cout << "--" << option << " has to be black, clamp or periodic" << "\n";
            return EXIT_FAILURE;
        }
    }
    if (var_map.count("filter") > 0 && !v3d::render::offline::film::filterNamed(var_map["filter"].as<std::string>(), &making.filter)) {
        std::cout << "--filter has to be box, triangle, catmull-rom, gaussian or sinc" << "\n";
        return EXIT_FAILURE;
    }
    if (var_map.count("swidth") > 0) {
        making.width.x = var_map["swidth"].as<float>();
    }
    if (var_map.count("twidth") > 0) {
        making.width.y = var_map["twidth"].as<float>();
    }
    if (!v3d::moya::texture::widthAllowed(making.width)) {
        std::cout << "--swidth and --twidth have to be above zero and at most 64" << "\n";
        return EXIT_FAILURE;
    }
    if (var_map.count("colorspace") > 0) {
        making.colourSpace = var_map["colorspace"].as<std::string>();
    }
    const boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    try {
        v3d::moya::texture::make(files[0], files[1], making, v3d::moya::colour::Config(), logger);
    } catch (const std::exception& error) {
        std::cout << "error making the texture - " << error.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

/** The checkpoints --checkpoint, --checkpoint-interval and --resume name. **/
v3d::moya::integrate::Checkpoints checkpointsOf(const boost::program_options::variables_map& var_map) {
    v3d::moya::integrate::Checkpoints out;
    if (var_map.count("checkpoint") != 0) {
        out.path = var_map["checkpoint"].as<std::string>();
    }
    if (var_map.count("checkpoint-interval") != 0) {
        out.interval = var_map["checkpoint-interval"].as<double>();
    }
    out.resume = var_map.count("resume") != 0;
    return out;
}

/** The options moya takes, as --help prints them. **/
boost::program_options::options_description optionsOf() {
    boost::program_options::options_description opts_desc("Allowed options");
    opts_desc.add_options()
        ("help", "produce help message")
        ("version", "display version info")
        ("file", boost::program_options::value<std::string>(), "input filename to be rendered")
        ("output", boost::program_options::value<std::string>(), "filename to be written; not with --scene")
        ("width", boost::program_options::value<int>(), "image width, replacing the scene's Format; needs --height; not with --scene, --hits or --paths")
        ("height", boost::program_options::value<int>(), "image height, replacing the scene's Format; needs --width; not with --scene, --hits or --paths")
        ("silent", "print no progress line")
        ("scene", "print the scene the new core reads from the file, in place of rendering it; not with --hits or --paths")
        ("hits", "write what the new core's rays hit to --output, which has to end in .exr, or to hits.exr, as an EXR file, "
            "in place of rendering it; not with --scene or --paths")
        ("paths", "write the outputs the scene names from the picture the new core's path tracer renders, in place of rendering it; "
            "--output, which has to end in .exr or .png, replaces the first one's file, and a scene that names none writes paths.exr; "
            "not with --scene or --hits")
        ("checkpoint", boost::program_options::value<std::string>(),
            "a checkpoint the path tracer writes after each pass, as an EXR file, and reads with --resume; only with --paths")
        ("checkpoint-interval", boost::program_options::value<double>(),
            "the least seconds between two checkpoints, in place of one after every pass; needs --checkpoint")
        ("resume", "continue the render the --checkpoint file holds, at the pass after its last; needs --checkpoint")
        ("denoise", "write the path tracer's beauty denoised beside it, as beauty_denoised; only with --paths")
        ("grid", boost::program_options::value<int>(),
            "micropolygon grid size, used whatever size the scene's Option \"limits\" names; not with --scene, --hits or --paths")
        ("bucket", boost::program_options::value<int>(),
            "square bucket size in pixels, used whatever size the scene's Option \"limits\" names; not with --scene, --hits or --paths")
        ("maketexture", boost::program_options::value<std::vector<std::string>>()->multitoken(),
            "make a texture from an image: the image, then the texture, a tiled EXR file; not with --file")
        ("swrap", boost::program_options::value<std::string>(), "the texture's wrap mode along s: black, clamp or periodic; default black; needs --maketexture")
        ("twrap", boost::program_options::value<std::string>(), "the texture's wrap mode along t: black, clamp or periodic; default black; needs --maketexture")
        ("filter", boost::program_options::value<std::string>(),
            "the filter each level is made with: box, triangle, catmull-rom, gaussian or sinc; default box; needs --maketexture")
        ("swidth", boost::program_options::value<float>(), "the filter's width along s, in texels of the level made; default 1; needs --maketexture")
        ("twidth", boost::program_options::value<float>(), "the filter's width along t, in texels of the level made; default 1; needs --maketexture")
        ("colorspace", boost::program_options::value<std::string>(),
            "the colour space the image is read in, such as Raw for data; default the image's own; needs --maketexture");
    return opts_desc;
}

int run(int argc, char *argv[]) {
    const boost::program_options::options_description opts_desc = optionsOf();
    boost::program_options::variables_map var_map;
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, opts_desc), var_map);
    boost::program_options::notify(var_map);

    if (var_map.count("help") != 0) {
        std::cout << opts_desc << "\n";
        exit(EXIT_SUCCESS);
    }

    if (var_map.count("version") != 0) {
        std::cout << "moya " << v3d::version::text() << "\n";
        std::cout << "The RenderMan (R) Interface Procedures and Protocol are:" << "\n" <<
                     "Copyright 1988, 1989, Pixar" << "\n" <<
                     "All Rights Reserved" << "\n";

        exit(EXIT_SUCCESS);
    }

    std::string infile;
    std::string outfile;

    if (var_map.count("file") != 0) {
        infile = var_map["file"].as<std::string>();
    }
    if (var_map.count("output") != 0) {
        outfile = var_map["output"].as<std::string>();
    }

    const std::string textureRefused = textureConflict(var_map);
    if (!textureRefused.empty()) {
        std::cout << textureRefused << "\n";
        return EXIT_FAILURE;
    }
    if (var_map.count("maketexture") != 0) {
        return makeTexture(var_map);
    }

    if (infile.empty()) {
        std::cout << opts_desc << "\n";
        exit(EXIT_SUCCESS);
    }

    const std::string refused = conflict(var_map);
    if (!refused.empty()) {
        std::cout << refused << "\n";
        return EXIT_FAILURE;
    }

    // RiBegin and RiEnd have no RIB equivalent. The handler's life is the span between them
    const boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    if (var_map.count("hits") != 0) {
        return showHits(infile, outfile.empty() ? std::string("hits.exr") : outfile, logger);
    }
    if (var_map.count("scene") != 0) {
        return describe(infile, logger);
    }
    if (var_map.count("paths") != 0) {
        return renderPaths(infile, outfile, checkpointsOf(var_map), var_map.count("denoise") > 0, logger);
    }
    v3d::moya::ri::RIBHandler handler;

    if (!outfile.empty()) {
        handler.output(outfile);
    }
    sizes(var_map, &handler);

    // flushed rather than left to the buffer: the render that follows takes the rest of the
    // run, and the progress line has to appear before it starts
    if (var_map.count("silent") == 0) {
        std::cout << "Rendering scene file: " << infile << "\n" << std::flush;
    }

    v3d::render::offline::rib::Reader reader(logger);
    if (!reader.read(infile, &handler)) {
        std::cout << "error reading rib file - " << reader.error() << "\n";
        exit(EXIT_FAILURE);
    }

    // the picture was written by RiWorldEnd, as the RI standard specifies. The log names each
    // file of a scene of several frames that could not be written
    if (handler.context().unwritten() > 0) {
        std::cout << "error writing the picture - " << handler.context().displayName() << "\n";
        exit(EXIT_FAILURE);
    }
    return EXIT_SUCCESS;
}

};  // namespace

int main(int argc, char *argv[]) {
    // the option parser reports an error by throwing, and so does a failed allocation. An
    // exception leaving main aborts with no message. The catch blocks write through stdio,
    // not std::cout, because a last-resort handler must not throw
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
