/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <cstdlib>
#include <fstream>
#include <ios>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>
#include <boost/test/unit_test.hpp>

namespace v3d::moya::bench {
namespace {

/** What a run of moya_bench printed, and whether it exited with success. **/
struct Run {
    bool succeeded { false };
    std::string printed;
};

/** Runs moya_bench with the arguments, and keeps what it printed to stdout and stderr. **/
Run bench(const std::string & arguments) {
    const boost::filesystem::path printed = boost::filesystem::temp_directory_path() / "v3d_moya_bench.txt";
    std::string command = "\"" + std::string(V3D_MOYA_BENCH_EXECUTABLE) + "\" " + arguments + " > \"" + printed.string() + "\" 2>&1";
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

/** The cells of each row of a table that starts with a scene's name, split at its bars. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - what was printed, then the scene
std::vector<std::vector<std::string>> rowsOf(const std::string & printed, const std::string & scene) {
    std::vector<std::vector<std::string>> out;
    std::istringstream lines(printed);
    std::string line;
    while (std::getline(lines, line)) {
        if (line.rfind("| " + scene + " |", 0) != 0) {
            continue;
        }
        std::vector<std::string> cells;
        std::istringstream split(line);
        std::string cell;
        while (std::getline(split, cell, '|')) {
            cell.erase(0, cell.find_first_not_of(' '));
            cell.erase(cell.find_last_not_of(" \r") + 1);
            cells.push_back(cell);
        }
        out.push_back(cells);
    }
    return out;
}

// a short pass times every scene at the instruction set it is asked for. Each row holds its
// primitives, both kinds of ray are traced at a rate, and the incoherent rays meet something
BOOST_AUTO_TEST_CASE(moya_bench_a_short_pass_times_every_scene_test) {
    const Run run = bench("--quick --isa sse42");
    BOOST_TEST_REQUIRE(run.succeeded, run.printed);
    for (const char * scene : { "small triangles", "large triangles", "curves", "instances", "motion" }) {
        const std::vector<std::vector<std::string>> rows = rowsOf(run.printed, scene);
        BOOST_TEST_REQUIRE(rows.size() == 1U, scene);
        // the cells after the leading bar: scene, isa, primitives, gather, build, bytes, coherent
        // rate and hits, incoherent rate and hits
        const std::vector<std::string> & cells = rows.front();
        BOOST_TEST_REQUIRE(cells.size() >= 11U, scene);
        BOOST_TEST(cells[2] == "sse42", scene);
        BOOST_TEST(std::stod(cells[3]) > 0.0, scene);
        BOOST_TEST(std::stod(cells[7]) > 0.0, scene);
        BOOST_TEST(std::stod(cells[9]) > 0.0, scene);
        BOOST_TEST(std::stod(cells[10]) > 0.0, scene);
    }
}

// a short pass times the lit room with its outputs, at a rate of samples, and the shares of its
// time in adding Cryptomatte coverage and deep lists lie between none and all
BOOST_AUTO_TEST_CASE(moya_bench_a_short_pass_times_the_room_with_outputs_test) {
    const Run run = bench("--quick --isa sse42");
    BOOST_TEST_REQUIRE(run.succeeded, run.printed);
    const std::vector<std::vector<std::string>> rows = rowsOf(run.printed, "lit room with outputs");
    BOOST_TEST_REQUIRE(rows.size() == 1U);
    // the cells after the leading bar: scene, isa, samples a second, the plain room's rate as a
    // share, and the shares in coverage and in deep lists
    const std::vector<std::string> & cells = rows.front();
    BOOST_TEST_REQUIRE(cells.size() >= 7U);
    BOOST_TEST(cells[2] == "sse42");
    BOOST_TEST(std::stod(cells[3]) > 0.0);
    BOOST_TEST(std::stod(cells[4]) > 0.0);
    for (std::size_t i = 5; i < 7U; ++i) {
        BOOST_TEST(std::stod(cells[i]) > 0.0);
        BOOST_TEST(std::stod(cells[i]) < 100.0);
    }
}

// a short pass times the integrator over the lit room, at a rate of samples and of rays. There
// are more rays than samples, since a sample traces a camera ray and its lights' shadow rays. The
// share of the time in traversal lies between none and all
BOOST_AUTO_TEST_CASE(moya_bench_a_short_pass_times_the_lit_room_test) {
    const Run run = bench("--quick --isa sse42");
    BOOST_TEST_REQUIRE(run.succeeded, run.printed);
    const std::vector<std::vector<std::string>> rows = rowsOf(run.printed, "lit room");
    BOOST_TEST_REQUIRE(rows.size() == 1U);
    // the cells after the leading bar: scene, isa, primitives, pixels, samples per pixel, samples
    // and rays a second, rays per sample, and the share in traversal
    const std::vector<std::string> & cells = rows.front();
    BOOST_TEST_REQUIRE(cells.size() >= 10U);
    BOOST_TEST(cells[2] == "sse42");
    BOOST_TEST(std::stod(cells[3]) > 0.0);
    BOOST_TEST(std::stod(cells[4]) == 256.0);
    BOOST_TEST(std::stod(cells[5]) == 4.0);
    BOOST_TEST(std::stod(cells[6]) > 0.0);
    BOOST_TEST(std::stod(cells[7]) > std::stod(cells[6]));
    BOOST_TEST(std::stod(cells[8]) > 1.0);
    BOOST_TEST(std::stod(cells[9]) > 0.0);
    BOOST_TEST(std::stod(cells[9]) < 100.0);
}

// a short pass times the integrator over the textured room too. Its materials' programs and its
// texture lookups each take a share of the time, and the lookups' share lies within the
// programs', since a program's time holds the lookups it makes
BOOST_AUTO_TEST_CASE(moya_bench_a_short_pass_times_the_textured_room_test) {
    const Run run = bench("--quick --isa sse42");
    BOOST_TEST_REQUIRE(run.succeeded, run.printed);
    const std::vector<std::vector<std::string>> rows = rowsOf(run.printed, "textured room");
    BOOST_TEST_REQUIRE(rows.size() == 1U);
    // the cells after the leading bar: as the lit room's, then the shares in programs and in
    // texture lookups
    const std::vector<std::string> & cells = rows.front();
    BOOST_TEST_REQUIRE(cells.size() >= 12U);
    BOOST_TEST(std::stod(cells[6]) > 0.0);
    BOOST_TEST(std::stod(cells[10]) > 0.0);
    BOOST_TEST(std::stod(cells[10]) < 100.0);
    BOOST_TEST(std::stod(cells[11]) > 0.0);
    BOOST_TEST(std::stod(cells[11]) < std::stod(cells[10]));
}

// a short pass times the integrator over the scenes of media, subsurface and hair, and each one's
// time falls in its own shares. The fog room's tracking holds its grid lookups, the statue's walk
// is tracking through a medium of no grid, and the lock's time is in the hair model and not in
// tracking
BOOST_AUTO_TEST_CASE(moya_bench_a_short_pass_times_media_subsurface_and_hair_test) {
    const Run run = bench("--quick --isa sse42");
    BOOST_TEST_REQUIRE(run.succeeded, run.printed);
    // the cells after the leading bar: as the lit room's to the share in traversal, then the
    // shares in tracking, grid lookups, programs and the hair model
    const auto cellsOf = [&](const std::string & scene) {
        const std::vector<std::vector<std::string>> rows = rowsOf(run.printed, scene);
        BOOST_TEST_REQUIRE(rows.size() == 1U, scene);
        BOOST_TEST_REQUIRE(rows.front().size() >= 14U, scene);
        BOOST_TEST(std::stod(rows.front()[6]) > 0.0, scene);
        return rows.front();
    };
    const std::vector<std::string> fog = cellsOf("fog room");
    BOOST_TEST(std::stod(fog[11]) > 0.0);
    BOOST_TEST(std::stod(fog[11]) < std::stod(fog[10]));
    BOOST_TEST(std::stod(fog[10]) < 100.0);
    BOOST_TEST(std::stod(fog[13]) == 0.0);
    const std::vector<std::string> statue = cellsOf("subsurface room");
    BOOST_TEST(std::stod(statue[10]) > 0.0);
    BOOST_TEST(std::stod(statue[11]) == 0.0);
    BOOST_TEST(std::stod(statue[13]) == 0.0);
    const std::vector<std::string> lock = cellsOf("hair lock");
    BOOST_TEST(std::stod(lock[13]) > 0.0);
    BOOST_TEST(std::stod(lock[13]) < 100.0);
    BOOST_TEST(std::stod(lock[10]) == 0.0);
}

// an instruction set the processor does not run, or one that has no such name, is refused
BOOST_AUTO_TEST_CASE(moya_bench_refuses_an_instruction_set_it_cannot_run_test) {
    const Run run = bench("--quick --isa mmx");
    BOOST_TEST(!run.succeeded);
    BOOST_TEST(run.printed.find("runs no instruction set named mmx") != std::string::npos, run.printed);
}

};  // namespace
};  // namespace v3d::moya::bench
