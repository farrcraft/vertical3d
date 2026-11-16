/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <ios>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>

namespace {

/** The scene every case reads, from the suite's data. **/
const char * const kScene = "--file data/raytrace-spheres.rib";

/** What a run of moya printed, and whether it exited with success. **/
struct Run {
    bool succeeded { false };
    std::string printed;
};

/**
 * Runs the moya executable with the arguments, from the suite's directory, and keeps what it
 * printed to stdout and stderr.
 **/
Run moya(const std::string & arguments) {
    const boost::filesystem::path printed = boost::filesystem::temp_directory_path() / "v3d_moya_command_line.txt";
    std::string command = "\"" + std::string(V3D_MOYA_EXECUTABLE) + "\" " + arguments + " > \"" + printed.string() + "\" 2>&1";
#ifdef _WIN32
    // cmd.exe drops the first and last quote of a command that starts with one
    command = "\"" + command + "\"";
#endif
    Run run;
    // NOLINTNEXTLINE(bugprone-command-processor) - the case runs the real executable
    run.succeeded = std::system(command.c_str()) == 0;
    std::ifstream stream(printed.string(), std::ios::binary);
    run.printed.assign(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
    stream.close();
    boost::system::error_code ignored;
    boost::filesystem::remove(printed, ignored);
    return run;
}

// --scene and --hits each replace the render, so they are refused together
BOOST_AUTO_TEST_CASE(moya_command_line_refuses_scene_with_hits_test) {
    const Run run = moya(std::string(kScene) + " --scene --hits");
    BOOST_TEST(!run.succeeded);
    BOOST_TEST(run.printed.find("--scene and --hits cannot be given together") != std::string::npos, run.printed);
}

// a size is refused with --scene or --hits, which take the scene's own Format, rather than ignored
BOOST_AUTO_TEST_CASE(moya_command_line_refuses_render_options_with_scene_or_hits_test) {
    const std::vector<std::string> options { "--width 64", "--height 64" };
    for (const std::string mode : { "--scene", "--hits" }) {
        for (const std::string & option : options) {
            std::string arguments = kScene;
            arguments.append(" ").append(mode).append(" ").append(option);
            const Run run = moya(arguments);
            BOOST_TEST(!run.succeeded, mode << " " << option);
            const std::string expected = option.substr(0, option.find(' ')) + " cannot be given with " + mode;
            BOOST_TEST(run.printed.find(expected) != std::string::npos, mode << " " << option << ": " << run.printed);
        }
    }
}

// --scene prints and writes no file, so an --output is refused. --hits writes an EXR file and a
// render an EXR or a PNG file, so an output of another kind is refused
BOOST_AUTO_TEST_CASE(moya_command_line_refuses_an_output_the_mode_cannot_write_test) {
    const Run scene = moya(std::string(kScene) + " --scene --output picture.exr");
    BOOST_TEST(!scene.succeeded);
    BOOST_TEST(scene.printed.find("--output cannot be given with --scene") != std::string::npos, scene.printed);

    const Run hits = moya(std::string(kScene) + " --hits --output hits.png");
    BOOST_TEST(!hits.succeeded);
    BOOST_TEST(hits.printed.find("--output has to name an .exr file with --hits") != std::string::npos, hits.printed);

    const Run rendered = moya(std::string(kScene) + " --output picture.tif");
    BOOST_TEST(!rendered.succeeded);
    BOOST_TEST(rendered.printed.find("--output has to name an .exr or a .png file") != std::string::npos, rendered.printed);
}

/** The command line's scene with displays before it, in the system's temporary directory. **/
boost::filesystem::path sceneDisplaying(const std::string & displays) {
    std::ifstream original("data/raytrace-spheres.rib", std::ios::binary);
    const std::string body((std::istreambuf_iterator<char>(original)), std::istreambuf_iterator<char>());
    const boost::filesystem::path made = boost::filesystem::temp_directory_path() / "v3d_moya_command_line_displays.rib";
    std::ofstream written(made.string(), std::ios::binary);
    written << displays << body;
    return made;
}

/** Whether a file starts with the eight bytes every PNG file starts with. **/
bool isPng(const boost::filesystem::path & file) {
    std::ifstream stream(file.string(), std::ios::binary);
    std::string start(8, '\0');
    stream.read(start.data(), static_cast<std::streamsize>(start.size()));
    return stream.good() && start == "\x89PNG\r\n\x1a\n";
}

// a render writes every output the scene names, each in its own format, and --output replaces the
// first one's file
BOOST_AUTO_TEST_CASE(moya_command_line_writes_every_output_the_scene_names_test) {
    const boost::filesystem::path directory = boost::filesystem::temp_directory_path();
    const boost::filesystem::path first = directory / "v3d_moya_command_line_first.exr";
    const boost::filesystem::path second = directory / "v3d_moya_command_line_second.png";
    const boost::filesystem::path replaced = directory / "v3d_moya_command_line_replaced.png";
    boost::system::error_code ignored;
    for (const boost::filesystem::path & file : { first, second, replaced }) {
        boost::filesystem::remove(file, ignored);
    }
    const boost::filesystem::path scene = sceneDisplaying("Display \"" + first.generic_string() + "\" \"file\" \"rgba\"\n" + "Display \"+" +
        second.generic_string() + "\" \"file\" \"rgb\"\n");

    const Run both = moya("--file \"" + scene.string() + "\"");
    BOOST_TEST(both.succeeded, both.printed);
    BOOST_TEST((boost::filesystem::exists(first) && boost::filesystem::file_size(first) > 0U));
    BOOST_TEST(isPng(second));

    boost::filesystem::remove(first, ignored);
    boost::filesystem::remove(second, ignored);
    const Run output = moya("--file \"" + scene.string() + "\" --output \"" + replaced.string() + "\"");
    BOOST_TEST(output.succeeded, output.printed);
    BOOST_TEST(!boost::filesystem::exists(first));
    BOOST_TEST(isPng(replaced));
    BOOST_TEST(isPng(second));

    for (const boost::filesystem::path & file : { first, second, replaced, scene }) {
        boost::filesystem::remove(file, ignored);
    }
}

/** The width and height a PNG file's header holds, or zero when it holds none. **/
std::pair<unsigned int, unsigned int> pngSize(const boost::filesystem::path & file) {
    std::ifstream stream(file.string(), std::ios::binary);
    std::string header(24, '\0');
    stream.read(header.data(), static_cast<std::streamsize>(header.size()));
    if (!stream.good()) {
        return { 0U, 0U };
    }
    // the IHDR chunk follows the signature, and holds the width and height, each in four bytes
    // with the most significant first
    const auto side = [&header](std::size_t at) {
        unsigned int value = 0U;
        for (std::size_t index = at; index < at + 4U; ++index) {
            value = (value << 8U) | static_cast<unsigned char>(header[index]);
        }
        return value;
    };
    return { side(16U), side(20U) };
}

/**
 * --width and --height replace the size the scene's Format names.
 *
 * Passes before the change: the hiders took the same two options, and this case holds that the
 * path tracer keeps them.
 **/
BOOST_AUTO_TEST_CASE(moya_command_line_width_and_height_replace_the_format_test) {
    const boost::filesystem::path written = boost::filesystem::temp_directory_path() / "v3d_moya_command_line_sized.png";
    boost::system::error_code ignored;
    boost::filesystem::remove(written, ignored);
    const Run run = moya(std::string(kScene) + " --width 16 --height 12 --output \"" + written.string() + "\"");
    BOOST_TEST(run.succeeded, run.printed);
    const std::pair<unsigned int, unsigned int> size = pngSize(written);
    BOOST_TEST(size.first == 16U);
    BOOST_TEST(size.second == 12U);
    boost::filesystem::remove(written, ignored);
}

// a side alone, or one the renderer cannot make, is refused rather than ignored
BOOST_AUTO_TEST_CASE(moya_command_line_refuses_a_size_it_cannot_use_test) {
    for (const std::string option : { "--width 16", "--height 12" }) {
        const Run run = moya(std::string(kScene) + " " + option);
        BOOST_TEST(!run.succeeded, option);
        BOOST_TEST(run.printed.find("--width and --height have to be given together") != std::string::npos, option << ": " << run.printed);
    }
    for (const std::string option : { "--width 0 --height 12", "--width 16 --height 65537" }) {
        const Run run = moya(std::string(kScene) + " " + option);
        BOOST_TEST(!run.succeeded, option);
        BOOST_TEST(run.printed.find("--width and --height have to be between 1 and 65536") != std::string::npos, option << ": " << run.printed);
    }
}

// the path tracer is moya's only renderer, so the options of the hiders and the option that
// chose the path tracer are refused, each by name
BOOST_AUTO_TEST_CASE(moya_command_line_refuses_the_options_of_the_hiders_test) {
    for (const std::string option : { "--paths", "--grid", "--bucket" }) {
        const std::string value = option == "--paths" ? "" : " 8";
        const Run run = moya(std::string(kScene) + " " + option + value);
        BOOST_TEST(!run.succeeded, option);
        BOOST_TEST(run.printed.find("'" + option + "'") != std::string::npos, option << ": " << run.printed);
    }
}

// a scene that names no output writes its beauty to an EXR file named after the scene, in the
// working directory, and --silent prints no progress line
BOOST_AUTO_TEST_CASE(moya_command_line_a_scene_with_no_output_writes_one_named_after_it_test) {
    const boost::filesystem::path written = boost::filesystem::current_path() / "raytrace-spheres.exr";
    boost::system::error_code ignored;
    boost::filesystem::remove(written, ignored);
    const Run run = moya(std::string(kScene) + " --silent");
    BOOST_TEST(run.succeeded, run.printed);
    BOOST_TEST((boost::filesystem::exists(written) && boost::filesystem::file_size(written) > 0U));
    BOOST_TEST(run.printed.find("Rendering scene file") == std::string::npos, run.printed);
    boost::filesystem::remove(written, ignored);
}

// a render writes the path tracer's picture as EXR, and says which scene it renders
BOOST_AUTO_TEST_CASE(moya_command_line_writes_the_picture_test) {
    const boost::filesystem::path written = boost::filesystem::temp_directory_path() / "v3d_moya_command_line_paths.exr";
    boost::system::error_code ignored;
    boost::filesystem::remove(written, ignored);
    const Run run = moya(std::string(kScene) + " --output \"" + written.string() + "\"");
    BOOST_TEST(run.succeeded, run.printed);
    BOOST_TEST((boost::filesystem::exists(written) && boost::filesystem::file_size(written) > 0U));
    BOOST_TEST(run.printed.find("Rendering scene file: data/raytrace-spheres.rib") != std::string::npos, run.printed);
    boost::filesystem::remove(written, ignored);
}

/**
 * A render prints Pixar's notice even when silent, since moya reads RIB and is not RenderMan
 * compliant.
 *
 * Passes before the change: the hiders' handler printed the notice, and this case holds that a
 * render through the path tracer still prints it.
 **/
BOOST_AUTO_TEST_CASE(moya_command_line_a_render_prints_the_renderman_notice_test) {
    const boost::filesystem::path written = boost::filesystem::temp_directory_path() / "v3d_moya_command_line_notice.exr";
    boost::system::error_code ignored;
    const Run run = moya(std::string(kScene) + " --silent --output \"" + written.string() + "\"");
    BOOST_TEST(run.succeeded, run.printed);
    BOOST_TEST(run.printed.find("Copyright 1988, 1989, Pixar") != std::string::npos, run.printed);
    boost::filesystem::remove(written, ignored);
}

/**
 * A picture that cannot be written is named once the scene is read, and the render fails.
 *
 * Passes before the change: the hiders' handler reported an unwritten picture the same way, and
 * this case holds that a render through the path tracer still does.
 **/
BOOST_AUTO_TEST_CASE(moya_command_line_a_picture_it_cannot_write_fails_the_render_test) {
    const boost::filesystem::path written = boost::filesystem::temp_directory_path() / "v3d_moya_command_line_absent" / "picture.png";
    const Run run = moya(std::string(kScene) + " --silent --output \"" + written.string() + "\"");
    BOOST_TEST(!run.succeeded, run.printed);
    BOOST_TEST(run.printed.find("error writing the picture - " + written.string()) != std::string::npos, run.printed);
}

// --scene and --hits render no picture, so they keep no checkpoint, and the interval and
// --resume need the file a checkpoint names
BOOST_AUTO_TEST_CASE(moya_command_line_refuses_checkpoints_it_cannot_keep_test) {
    for (const std::string mode : { "--scene", "--hits" }) {
        for (const std::string option : { "--checkpoint c.exr", "--checkpoint-interval 5", "--resume" }) {
            const Run run = moya(std::string(kScene) + " " + mode + " " + option);
            BOOST_TEST(!run.succeeded, mode << " " << option);
            const std::string expected = "cannot be given with " + mode + ", which renders no picture";
            BOOST_TEST(run.printed.find(expected) != std::string::npos, mode << " " << option << ": " << run.printed);
        }
    }
    for (const std::string option : { "--checkpoint-interval 5", "--resume" }) {
        const Run run = moya(std::string(kScene) + " " + option);
        BOOST_TEST(!run.succeeded, option);
        BOOST_TEST(run.printed.find("needs --checkpoint") != std::string::npos, option << ": " << run.printed);
    }
    const Run below = moya(std::string(kScene) + " --checkpoint c.exr --checkpoint-interval -1");
    BOOST_TEST(!below.succeeded);
    BOOST_TEST(below.printed.find("--checkpoint-interval cannot be below zero") != std::string::npos, below.printed);
}

// --denoise denoises the path tracer's picture. It is refused with --scene, which renders none,
// and with --maketexture, which reads no scene
BOOST_AUTO_TEST_CASE(moya_command_line_refuses_denoise_without_a_render_test) {
    const Run scene = moya(std::string(kScene) + " --scene --denoise");
    BOOST_TEST(!scene.succeeded);
    BOOST_TEST(scene.printed.find("--denoise cannot be given with --scene, which renders no picture") != std::string::npos, scene.printed);
    const Run texture = moya("--maketexture a.png a.tx --denoise");
    BOOST_TEST(!texture.succeeded);
    BOOST_TEST(texture.printed.find("--denoise cannot be given with --maketexture") != std::string::npos, texture.printed);
}

/** A file's bytes. **/
std::string bytesOf(const boost::filesystem::path & path) {
    std::ifstream stream(path.string(), std::ios::binary);
    return { std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>() };
}

// --checkpoint writes the checkpoint beside the picture. --resume continues from it, and
// a render it holds the end of writes the same picture again
BOOST_AUTO_TEST_CASE(moya_command_line_writes_and_resumes_a_checkpoint_test) {
    const boost::filesystem::path written = boost::filesystem::temp_directory_path() / "v3d_moya_command_line_resumed.exr";
    const boost::filesystem::path kept = boost::filesystem::temp_directory_path() / "v3d_moya_command_line_checkpoint.exr";
    boost::system::error_code ignored;
    boost::filesystem::remove(written, ignored);
    boost::filesystem::remove(kept, ignored);
    const std::string files = " --output \"" + written.string() + "\" --checkpoint \"" + kept.string() + "\"";
    const Run run = moya(std::string(kScene) + files);
    BOOST_TEST(run.succeeded, run.printed);
    BOOST_TEST(boost::filesystem::exists(kept));
    const std::string picture = bytesOf(written);
    boost::filesystem::remove(written, ignored);
    const Run resumed = moya(std::string(kScene) + files + " --resume");
    BOOST_TEST(resumed.succeeded, resumed.printed);
    BOOST_TEST((!picture.empty() && bytesOf(written) == picture));
    boost::filesystem::remove(written, ignored);
    boost::filesystem::remove(kept, ignored);
}

/**
 * --hits writes an output named .exr in any case.
 *
 * Passes before the change: an output refused for its extension is possible only under the
 * check of --hits' output that the same change adds, and this case keeps that check from
 * refusing an upper-case extension.
 **/
BOOST_AUTO_TEST_CASE(moya_command_line_hits_writes_an_exr_named_in_any_case_test) {
    const boost::filesystem::path written = boost::filesystem::temp_directory_path() / "v3d_moya_command_line_hits.EXR";
    boost::system::error_code ignored;
    boost::filesystem::remove(written, ignored);
    const Run run = moya(std::string(kScene) + " --hits --output \"" + written.string() + "\"");
    BOOST_TEST(run.succeeded, run.printed);
    BOOST_TEST(boost::filesystem::exists(written));
    boost::filesystem::remove(written, ignored);
}

// --maketexture makes a tiled texture from an image, with the wrap modes and filter it is given
BOOST_AUTO_TEST_CASE(moya_command_line_maketexture_makes_a_texture_test) {
    const boost::filesystem::path made = boost::filesystem::temp_directory_path() / "v3d_moya_command_line_texture.tx";
    boost::system::error_code ignored;
    boost::filesystem::remove(made, ignored);
    const Run run = moya("--maketexture data/blocks.png \"" + made.string() + "\" --swrap periodic --twrap clamp --filter gaussian --swidth 2 --twidth 2");
    BOOST_TEST(run.succeeded, run.printed);
    BOOST_TEST((boost::filesystem::exists(made) && boost::filesystem::file_size(made) > 0U));
    boost::filesystem::remove(made, ignored);

    const Run absent = moya("--maketexture data/absent.png \"" + made.string() + "\"");
    BOOST_TEST(!absent.succeeded);
    BOOST_TEST(absent.printed.find("error making the texture - ") != std::string::npos, absent.printed);
}

// --maketexture reads no scene, names two files, and is the only mode its options go with
BOOST_AUTO_TEST_CASE(moya_command_line_refuses_maketexture_options_it_cannot_use_test) {
    const Run file = moya(std::string(kScene) + " --maketexture a.png a.tx");
    BOOST_TEST(!file.succeeded);
    BOOST_TEST(file.printed.find("--file cannot be given with --maketexture") != std::string::npos, file.printed);

    const Run one = moya("--maketexture a.png");
    BOOST_TEST(!one.succeeded);
    BOOST_TEST(one.printed.find("--maketexture names two files") != std::string::npos, one.printed);

    for (const std::string option : { "--swrap clamp", "--twrap clamp", "--filter box", "--swidth 1", "--twidth 1", "--colorspace Raw" }) {
        const Run run = moya(std::string(kScene) + " " + option);
        BOOST_TEST(!run.succeeded, option);
        BOOST_TEST(run.printed.find("can only be given with --maketexture") != std::string::npos, option << ": " << run.printed);
    }

    const Run wrap = moya("--maketexture a.png a.tx --swrap mirror");
    BOOST_TEST(!wrap.succeeded);
    BOOST_TEST(wrap.printed.find("--swrap has to be black, clamp or periodic") != std::string::npos, wrap.printed);
    const Run filter = moya("--maketexture a.png a.tx --filter mitchell");
    BOOST_TEST(!filter.succeeded);
    BOOST_TEST(filter.printed.find("--filter has to be box") != std::string::npos, filter.printed);
    const Run width = moya("--maketexture a.png a.tx --swidth 0");
    BOOST_TEST(!width.succeeded);
    BOOST_TEST(width.printed.find("--swidth and --twidth have to be above zero and at most 64") != std::string::npos, width.printed);
}

};  // namespace
