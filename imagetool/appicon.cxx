/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

/**
 * appicon draws an app's SVG icon at each size an icon file holds, and writes the .ico the
 * build embeds in the app's executable.
 *
 *     appicon <icon.svg> <icon.ico>
 *
 * It is a step of the build rather than an app, so it has no icon of its own.
 **/

#include <api/asset/File.h>
#include <api/asset/Writer.h>
#include <api/image/Icon.h>
#include <api/image/Image.h>
#include <api/log/Logger.h>

#include <cstdio>
#include <cstdlib>
#include <optional>
#include <string>
#include <vector>

#include <boost/filesystem/path.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::fputs("usage: appicon <icon.svg> <icon.ico>\n", stderr);
        return EXIT_FAILURE;
    }
    const boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    const std::optional<std::string> svg = v3d::asset::readFile(argv[1]);
    if (!svg) {
        std::fprintf(stderr, "error: %s could not be read\n", argv[1]);
        return EXIT_FAILURE;
    }
    const std::vector<boost::shared_ptr<v3d::image::Image>> drawn = v3d::image::drawIcon(*svg, logger);
    if (drawn.empty()) {
        std::fprintf(stderr, "error: %s is not an SVG with a size\n", argv[1]);
        return EXIT_FAILURE;
    }
    const std::optional<std::string> icon = v3d::image::encodeIcon(drawn, logger);
    if (!icon || !v3d::asset::writeFile(boost::filesystem::path(argv[2]), *icon)) {
        std::fprintf(stderr, "error: %s could not be written\n", argv[2]);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
