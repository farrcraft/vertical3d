/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

// The main of a fuzz target built without libFuzzer. It passes each file it is given to the
// target's LLVMFuzzerTestOneInput once, and a directory stands for every file in it, so the
// target runs its corpus as a test. It takes libFuzzer's command line: an argument starting
// with '-' is one of libFuzzer's options, and is ignored.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <fstream>
#include <ios>
#include <iterator>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size);

namespace {

/**
 * The files an argument names: the file itself, or every regular file in a directory, in
 * order of name. Empty when the path does not exist.
 **/
std::vector<boost::filesystem::path> inputs(const boost::filesystem::path & path) {
    std::vector<boost::filesystem::path> found;
    boost::system::error_code error;
    if (boost::filesystem::is_directory(path, error)) {
        for (const auto & entry : boost::filesystem::directory_iterator(path, error)) {
            if (boost::filesystem::is_regular_file(entry.status(error))) {
                found.push_back(entry.path());
            }
        }
        std::sort(found.begin(), found.end());
    } else if (boost::filesystem::is_regular_file(path, error)) {
        found.push_back(path);
    }
    return found;
}

/**
 * Run one file through the target. False when it cannot be read.
 **/
bool run(const boost::filesystem::path & path) {
    std::ifstream stream(path.string(), std::ios::binary);
    if (!stream) {
        return false;
    }
    const std::vector<char> bytes((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    LLVMFuzzerTestOneInput(reinterpret_cast<const std::uint8_t*>(bytes.data()), bytes.size());
    return true;
}

/**
 * Run every file the command line names. Non-zero when an argument names no file or a file
 * cannot be read.
 **/
int runAll(int argc, char* argv[]) {
    std::size_t count = 0;
    for (int i = 1; i < argc; ++i) {
        const std::string argument(argv[i]);
        if (argument.starts_with("-")) {
            continue;
        }
        const std::vector<boost::filesystem::path> files = inputs(argument);
        if (files.empty()) {
            std::fprintf(stderr, "%s names no file\n", argument.c_str());
            return 1;
        }
        for (const boost::filesystem::path & file : files) {
            if (!run(file)) {
                std::fprintf(stderr, "could not read %s\n", file.string().c_str());
                return 1;
            }
            ++count;
        }
    }
    std::printf("ran %zu inputs\n", count);
    return 0;
}

};  // namespace

int main(int argc, char* argv[]) {
    // an exception that leaves the target is a failure, as it is under libFuzzer
    try {
        return runAll(argc, argv);
    } catch (const std::exception & error) {
        std::fprintf(stderr, "the target threw: %s\n", error.what());
    } catch (...) {
        std::fprintf(stderr, "the target threw\n");
    }
    return 1;
}
