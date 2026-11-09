/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <cstdlib>
#include <fstream>
#include <ios>
#include <iterator>
#include <string>
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

// a size, a grid or a bucket is refused with --scene or --hits, which take the scene's own Format,
// rather than ignored
BOOST_AUTO_TEST_CASE(moya_command_line_refuses_render_options_with_scene_or_hits_test) {
    const std::vector<std::string> options { "--width 64", "--height 64", "--grid 8", "--bucket 16" };
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

// --scene prints and writes no file, so an --output is refused. --hits writes an EXR file, so an
// output of another kind is refused
BOOST_AUTO_TEST_CASE(moya_command_line_refuses_an_output_the_mode_cannot_write_test) {
    const Run scene = moya(std::string(kScene) + " --scene --output picture.exr");
    BOOST_TEST(!scene.succeeded);
    BOOST_TEST(scene.printed.find("--output cannot be given with --scene") != std::string::npos, scene.printed);

    const Run hits = moya(std::string(kScene) + " --hits --output hits.png");
    BOOST_TEST(!hits.succeeded);
    BOOST_TEST(hits.printed.find("--output has to name an .exr file with --hits") != std::string::npos, hits.printed);
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

};  // namespace
