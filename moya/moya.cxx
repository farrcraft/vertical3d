/*
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 */

#include <api/log/Logger.h>
#include <api/rib/Sampling.h>
#include <api/rib/Reader.h>
#include <api/version/Version.h>
#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/integrate/Render.h>
#include <moya/libmoya/output/Exr.h>
#include <moya/libmoya/output/ExrPart.h>
#include <moya/libmoya/output/Output.h>
#include <moya/libmoya/output/Outputs.h>
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
#include <chrono>
#include <cstddef>
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
 * Pixar's notice, which a renderer of RIB that is not RenderMan compliant prints. It is printed
 * before every render, even a silent one, and with the version.
 **/
void notice() {
    std::cout << "The RenderMan (R) Interface Procedures and Protocol are:" << "\n" <<
                 "Copyright 1988, 1989, Pixar" << "\n" <<
                 "All Rights Reserved" << "\n";
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
    for (const char* const option : { "file", "output", "width", "height", "scene", "hits", "checkpoint", "checkpoint-interval", "resume",
             "denoise" }) {
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
 * Why the --output named cannot be written, or empty when it can. --hits writes an EXR file, and
 * a render an EXR or a PNG file, named in any case. An empty mode is a render.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the mode, then the file it writes
std::string outputConflict(const std::string & mode, std::string name) {
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char letter) { return static_cast<char>(std::tolower(letter)); });
    const auto ends = [&name](const std::string & extension) {
        return name.size() >= extension.size() && name.compare(name.size() - extension.size(), extension.size(), extension) == 0;
    };
    if (mode.empty() && !ends(".exr") && !ends(".png")) {
        return "--output has to name an .exr or a .png file, which a render writes";
    }
    if (mode == "--hits" && !ends(".exr")) {
        return "--output has to name an .exr file with --hits, which writes an EXR file";
    }
    return {};
}

/**
 * Why the options cannot be used together, or empty when they can.
 *
 * --scene and --hits each read the scene in place of rendering it, so only one of them can be
 * given. Neither renders a picture, so --checkpoint, --checkpoint-interval, --resume and
 * --denoise are refused with them. Each takes the picture's size from the scene's Format, so
 * --width and --height are refused with them too. --scene writes no file, so --output is refused
 * with it. --hits writes an EXR file, so its --output has to end in .exr, in any case.
 *
 * A render writes an EXR or a PNG file, so its --output has to end in .exr or .png. --width and
 * --height are given together. The interval and --resume each need --checkpoint, which names
 * the file, and the interval cannot be below zero.
 **/
std::string conflict(const boost::program_options::variables_map& var_map) {
    std::vector<std::string> modes;
    for (const char* const mode : { "scene", "hits" }) {
        if (var_map.count(mode) > 0) {
            modes.push_back("--" + std::string(mode));
        }
    }
    if (modes.size() > 1) {
        return modes[0] + " and " + modes[1] + " cannot be given together";
    }
    const std::string mode = modes.empty() ? std::string() : modes.front();
    if (!mode.empty()) {
        for (const char* const option : { "checkpoint", "checkpoint-interval", "resume", "denoise" }) {
            if (var_map.count(option) > 0) {
                return "--" + std::string(option) + " cannot be given with " + mode + ", which renders no picture";
            }
        }
        for (const char* const option : { "width", "height" }) {
            if (var_map.count(option) > 0) {
                return "--" + std::string(option) + " cannot be given with " + mode + ", which takes the scene's own Format";
            }
        }
    }
    if ((var_map.count("width") > 0) != (var_map.count("height") > 0)) {
        return "--width and --height have to be given together";
    }
    for (const char* const option : { "checkpoint-interval", "resume" }) {
        if (var_map.count(option) > 0 && var_map.count("checkpoint") == 0) {
            return "--" + std::string(option) + " needs --checkpoint, which names the checkpoint";
        }
    }
    if (var_map.count("checkpoint-interval") > 0 && var_map["checkpoint-interval"].as<double>() < 0.0) {
        return "--checkpoint-interval cannot be below zero";
    }
    if (var_map.count("output") == 0) {
        return {};
    }
    if (mode == "--scene") {
        return "--output cannot be given with --scene, which prints the scene and writes no file";
    }
    return outputConflict(mode, var_map["output"].as<std::string>());
}

/**
 * Read a file into the scene, and print the scene's description after each frame
 * the file commits, in place of rendering it.
 **/
int describe(const std::string & infile, const boost::shared_ptr<v3d::log::Logger> & logger) {
    v3d::moya::scene::Scene scene(v3d::moya::scene::kinds());
    v3d::moya::ri::SceneClient client(scene, logger);
    client.frame([&scene]() {
        scene.apply();
        std::cout << v3d::moya::scene::describe(scene) << std::flush;
    });
    v3d::rib::Reader reader(logger);
    if (!reader.read(infile, &client)) {
        std::cout << "error reading rib file - " << reader.error() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

/**
 * Read a file into the scene, and write its hit view after each frame the file
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
    v3d::rib::Reader reader(logger);
    if (!reader.read(infile, &client)) {
        std::cout << "error reading rib file - " << reader.error() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

/** A file's name without its directory or last extension. `scenes/spheres.rib` is `spheres`. **/
std::string stemOf(const std::string & file) {
    const std::size_t slash = file.find_last_of("/\\");
    const std::string name = slash == std::string::npos ? file : file.substr(slash + 1);
    const std::size_t dot = name.find_last_of('.');
    return dot == std::string::npos || dot == 0 ? name : name.substr(0, dot);
}

/**
 * The outputs a frame writes: the scene's, with the first one's file replaced by outfile when it
 * is given. A scene with no output writes its beauty in floats to outfile, or to an EXR file named
 * after the scene in the working directory. The beauty is denoised beside it when the options ask
 * for it.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the file read, then the file written
std::vector<v3d::moya::output::Output> outputsWritten(const v3d::moya::scene::Scene & scene, const std::string & infile, const std::string & outfile) {
    std::vector<v3d::moya::output::Output> outputs = v3d::moya::output::outputsOf(scene);
    if (outputs.empty()) {
        std::vector<std::string> channels;
        if (v3d::moya::output::denoisesBeauty(scene)) {
            channels.push_back(v3d::moya::output::denoisedOf(std::string(v3d::moya::output::kBeauty)));
        }
        outputs.push_back({ .file = outfile.empty() ? stemOf(infile) + ".exr" : outfile,
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

/**
 * Read a file into the scene, and write the outputs it names from the picture the path tracer
 * renders after each frame the file commits. When denoising, each frame's options ask for the
 * beauty denoised. A file of several frames leaves the last frame's. A checkpoint holds one
 * frame, so a resume continues the first frame, and each frame after it starts afresh and writes
 * its checkpoint over the one before.
 *
 * A file that cannot be written is logged, and the frames after it still render. The last such
 * file is printed once the whole scene is read, and the render fails.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the file read, then the file written
int render(const boost::program_options::variables_map& var_map, const std::string & infile, const std::string & outfile,
    const boost::shared_ptr<v3d::log::Logger> & logger) {
    v3d::moya::scene::Scene scene(v3d::moya::scene::kinds());
    v3d::moya::ri::SceneClient client(scene, logger);
    if (var_map.count("width") != 0 && !client.resolution(var_map["width"].as<int>(), var_map["height"].as<int>())) {
        std::cout << "--width and --height have to be between 1 and " << v3d::rib::largestResolution << "\n";
        return EXIT_FAILURE;
    }
    notice();
    v3d::moya::device::CpuDevice device;
    const v3d::moya::colour::Config config;
    v3d::moya::integrate::Checkpoints kept = checkpointsOf(var_map);
    const bool denoising = var_map.count("denoise") != 0;
    std::string unwritten;
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
        const auto started = std::chrono::steady_clock::now();
        const v3d::moya::film::Snapshot picture = v3d::moya::integrate::render(device, scene, logger, config, kept);
        const std::chrono::duration<double> took = std::chrono::steady_clock::now() - started;
        logger->get()->info("rendered {} by {} pixels in {:.3f} s", picture.resolution.x, picture.resolution.y, took.count());
        kept.resume = false;
        for (const v3d::moya::output::Output & output : outputsWritten(scene, infile, outfile)) {
            try {
                v3d::moya::output::write(output, picture, config, logger);
            } catch (const std::exception & error) {
                logger->get()->error("the picture could not be written to '{}' - {}", output.file, error.what());
                unwritten = output.file;
            }
        }
    });
    // flushed rather than left to the buffer: the render that follows takes the rest of the
    // run, and the progress line has to appear before it starts
    if (var_map.count("silent") == 0) {
        std::cout << "Rendering scene file: " << infile << "\n" << std::flush;
    }
    v3d::rib::Reader reader(logger);
    if (!reader.read(infile, &client)) {
        std::cout << "error reading rib file - " << reader.error() << "\n";
        return EXIT_FAILURE;
    }
    if (!unwritten.empty()) {
        std::cout << "error writing the picture - " << unwritten << "\n";
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
    if (var_map.count("filter") > 0 && !v3d::rib::filterNamed(var_map["filter"].as<std::string>(), &making.filter)) {
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

/** The options moya takes, as --help prints them. **/
boost::program_options::options_description optionsOf() {
    boost::program_options::options_description opts_desc("Allowed options");
    opts_desc.add_options()
        ("help", "produce help message")
        ("version", "display version info")
        ("file", boost::program_options::value<std::string>(), "input filename to be rendered")
        ("output", boost::program_options::value<std::string>(),
            "the file to write, which has to end in .exr or .png, in place of the first file the scene names; not with --scene")
        ("width", boost::program_options::value<int>(), "image width, replacing the scene's Format; needs --height; not with --scene or --hits")
        ("height", boost::program_options::value<int>(), "image height, replacing the scene's Format; needs --width; not with --scene or --hits")
        ("silent", "print no progress line")
        ("scene", "print the scene read from the file, in place of rendering it; not with --hits")
        ("hits", "write what the rays hit to --output, which has to end in .exr, or to hits.exr, as an EXR file, "
            "in place of rendering it; not with --scene")
        ("checkpoint", boost::program_options::value<std::string>(),
            "a checkpoint the path tracer writes after each pass, as an EXR file, and reads with --resume; not with --scene or --hits")
        ("checkpoint-interval", boost::program_options::value<double>(),
            "the least seconds between two checkpoints, in place of one after every pass; needs --checkpoint")
        ("resume", "continue the render the --checkpoint file holds, at the pass after its last; needs --checkpoint")
        ("denoise", "write the path tracer's beauty denoised beside it, as beauty_denoised; not with --scene or --hits")
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
        notice();
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

    const boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    if (var_map.count("hits") != 0) {
        return showHits(infile, outfile.empty() ? std::string("hits.exr") : outfile, logger);
    }
    if (var_map.count("scene") != 0) {
        return describe(infile, logger);
    }
    return render(var_map, infile, outfile, logger);
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
