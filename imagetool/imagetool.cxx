/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/Writer.h>
#include <api/asset/media/SpriteSheet.h>
#include <api/asset/media/SpriteSheets.h>
#include <api/image/Factory.h>
#include <api/image/Crop.h>
#include <api/image/Image.h>
#include <api/image/Pack.h>
#include <api/log/Logger.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/make_shared.hpp>

namespace {

/** The longest side of a sheet --pack makes. **/
constexpr unsigned int LARGEST_SHEET = 4096;

/** The document a sheet's sprites are written to, beside the sheet's image. **/
constexpr const char* SPRITES_DOCUMENT = "sprites.json";

/** Whether the file's extension is one the image factory reads. **/
bool readable(const boost::filesystem::path & file) {
    std::string extension = file.extension().string();
    std::ranges::transform(extension, extension.begin(), [](unsigned char letter) { return static_cast<char>(std::tolower(letter)); });
    return extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".bmp" || extension == ".tga";
}

/**
 * The image files named: a file as it is, and every image in a directory, in name order so a
 * run packs the same sheet every time. A directory's other files are skipped.
 **/
std::vector<boost::filesystem::path> imageFiles(const std::vector<std::string> & named) {
    std::vector<boost::filesystem::path> files;
    for (const std::string & name : named) {
        const boost::filesystem::path path(name);
        if (!boost::filesystem::is_directory(path)) {
            files.push_back(path);
            continue;
        }
        std::vector<boost::filesystem::path> found;
        for (const boost::filesystem::directory_entry & entry : boost::filesystem::directory_iterator(path)) {
            if (boost::filesystem::is_regular_file(entry.path()) && readable(entry.path())) {
                found.push_back(entry.path());
            }
        }
        std::ranges::sort(found);
        files.insert(files.end(), found.begin(), found.end());
    }
    return files;
}

/** Whether a sprite's name can be a file's name, with no directory in it. **/
bool plainName(const std::string & name) {
    return !name.empty() && name != "." && name != ".." && name.find_first_of("/\\:") == std::string::npos;
}

/** What a --pack or --unpack run was asked to do. **/
class SheetRequest final {
 public:
    /** The images, and directories of images, to pack. **/
    std::vector<std::string> pack;
    /** The sprites document to unpack. **/
    std::string unpack;
    /** The sheet to pack, or the one sheet to unpack. Empty unpacks every sheet. **/
    std::string sheet;
    /** Where a pack writes the sheet's image. Empty writes <sheet>.png. **/
    std::string outfile;
    bool silent = false;
};

/**
 * Pack the named images into one sheet, write it, and write where each sprite went into the
 * sprites document beside it. Each sprite is named for its file, without the extension.
 **/
int packSheet(const SheetRequest & request, const boost::shared_ptr<v3d::log::Logger> & logger) {
    v3d::image::Factory factory(logger);
    std::vector<v3d::image::Sprite> sprites;
    for (const boost::filesystem::path & file : imageFiles(request.pack)) {
        if (!request.silent) {
            std::cout << "Reading: " << file.string() << "\n";
        }
        const boost::shared_ptr<v3d::image::Image> image = factory.read(file.string());
        if (!image) {
            std::cout << "error reading " << file.string() << "!\n";
            return EXIT_FAILURE;
        }
        sprites.push_back({.name = file.stem().string(), .image = image});
    }

    const v3d::image::Packing packing = v3d::image::pack(sprites, LARGEST_SHEET, logger);
    if (!packing.sheet) {
        std::cout << "error: " << packing.refusal << "\n";
        return EXIT_FAILURE;
    }

    const boost::filesystem::path image(request.outfile.empty() ? request.sheet + ".png" : request.outfile);
    const boost::filesystem::path document = image.parent_path() / SPRITES_DOCUMENT;
    // the document's other sheets are kept, and only the one of this name is replaced
    v3d::asset::media::SpriteSheets sheets(logger);
    if (!sheets.read(document)) {
        std::cout << "error: " << document.string() << " holds something that is not a sprite sheet, so it is not rewritten\n";
        return EXIT_FAILURE;
    }
    if (!sheets.add(v3d::asset::media::SpriteSheet::packed(request.sheet, image.filename().string(), packing))) {
        std::cout << "error: a sheet needs a name\n";
        return EXIT_FAILURE;
    }

    if (!request.silent) {
        std::cout << "Writing: " << image.string() << ", " << packing.sheet->width() << "x" << packing.sheet->height() << "\n";
    }
    const std::optional<std::string> encoded = factory.encode(packing.sheet, v3d::image::Factory::kind(image.string()));
    if (!encoded || !v3d::asset::writeFile(image, *encoded)) {
        std::cout << "error writing file!" << "\n";
        return EXIT_FAILURE;
    }
    if (!request.silent) {
        std::cout << "Writing: " << document.string() << "\n";
    }
    if (!v3d::asset::writeDocument(document, sheets.document())) {
        std::cout << "error writing " << document.string() << "!\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

/**
 * Cut one sheet's sprites out of its image, and write each as a png in a directory named for
 * the sheet, beside the document.
 **/
int unpackSheet(const boost::filesystem::path & document, const v3d::asset::media::SpriteSheet & sheet, bool silent,
    v3d::image::Factory* factory) {
    if (!plainName(sheet.name())) {
        std::cout << "error: the sheet name \"" << sheet.name() << "\" cannot name a directory\n";
        return EXIT_FAILURE;
    }
    const boost::filesystem::path imageFile = document.parent_path() / sheet.image();
    if (!silent) {
        std::cout << "Reading: " << imageFile.string() << "\n";
    }
    const boost::shared_ptr<v3d::image::Image> image = factory->read(imageFile.string());
    if (!image) {
        std::cout << "error reading " << imageFile.string() << "!\n";
        return EXIT_FAILURE;
    }
    // a sheet's regions are in the pixels of the size it states, so an image of another size
    // cannot be cut by them
    if (static_cast<int>(image->width()) != sheet.width() || static_cast<int>(image->height()) != sheet.height()) {
        std::cout << "error: " << imageFile.string() << " is " << image->width() << "x" << image->height() <<
            " and the sheet says " << sheet.width() << "x" << sheet.height() << "\n";
        return EXIT_FAILURE;
    }

    for (const std::string & sprite : sheet.sprites()) {
        if (!plainName(sprite)) {
            std::cout << "error: the sprite name \"" << sprite << "\" cannot name a file\n";
            return EXIT_FAILURE;
        }
    }
    const v3d::image::Unpacking unpacking = v3d::image::unpack(*image, sheet.placements());
    if (!unpacking.refusal.empty()) {
        std::cout << "error: " << unpacking.refusal << "\n";
        return EXIT_FAILURE;
    }

    const boost::filesystem::path directory = document.parent_path() / sheet.name();
    boost::filesystem::create_directories(directory);
    for (const v3d::image::Sprite & sprite : unpacking.sprites) {
        const boost::filesystem::path file = directory / (sprite.name + ".png");
        if (!silent) {
            std::cout << "Writing: " << file.string() << "\n";
        }
        const std::optional<std::string> encoded = factory->encode(sprite.image, "png");
        if (!encoded || !v3d::asset::writeFile(file, *encoded)) {
            std::cout << "error writing " << file.string() << "!\n";
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

/**
 * Cut every sprite out of the sheets a sprites document names, or out of the one sheet the
 * request names.
 **/
int unpackSheets(const SheetRequest & request, const boost::shared_ptr<v3d::log::Logger> & logger) {
    const boost::filesystem::path document(request.unpack);
    if (!boost::filesystem::exists(document)) {
        std::cout << "error: there is no " << document.string() << "\n";
        return EXIT_FAILURE;
    }
    v3d::asset::media::SpriteSheets sheets(logger);
    if (!sheets.read(document)) {
        std::cout << "error: " << document.string() << " is not a sprites document\n";
        return EXIT_FAILURE;
    }
    if (!request.sheet.empty() && !sheets.has(request.sheet)) {
        std::cout << "error: " << document.string() << " has no sheet named " << request.sheet << "\n";
        return EXIT_FAILURE;
    }
    v3d::image::Factory factory(logger);
    for (const std::string & name : sheets.names()) {
        if (!request.sheet.empty() && name != request.sheet) {
            continue;
        }
        const int unpacked = unpackSheet(document, sheets.get(name), request.silent, &factory);
        if (unpacked != EXIT_SUCCESS) {
            return unpacked;
        }
    }
    return EXIT_SUCCESS;
}

/**
 * Run a --pack or --unpack, or refuse a command line that mixes them with each other or with
 * the options that work on one --file.
 *
 * @return the exit status, or nothing when the command line asks for neither
 **/
std::optional<int> sheets(const boost::program_options::variables_map & options) {
    SheetRequest request;
    if (options.count("pack") > 0) {
        request.pack = options["pack"].as<std::vector<std::string> >();
    }
    if (options.count("unpack") > 0) {
        request.unpack = options["unpack"].as<std::string>();
    }
    if (options.count("sheet") > 0) {
        request.sheet = options["sheet"].as<std::string>();
    }
    if (options.count("outfile") > 0) {
        request.outfile = options["outfile"].as<std::string>();
    }
    request.silent = options.count("silent") > 0;
    const bool packing = options.count("pack") > 0;
    const bool unpacking = options.count("unpack") > 0;
    if (!packing && !unpacking) {
        if (!request.sheet.empty()) {
            std::cout << "error: --sheet names a sheet to --pack or --unpack\n";
            return EXIT_FAILURE;
        }
        return std::nullopt;
    }
    // packing and unpacking each read files of their own, so neither is run on an image read by
    // --file or a crop of one
    if (options.count("file") > 0 || options.count("crop") > 0 || options.count("info") > 0) {
        std::cout << "error: --pack and --unpack take no --file, --crop or --info\n";
        return EXIT_FAILURE;
    }
    if (packing && unpacking) {
        std::cout << "error: a run either packs or unpacks\n";
        return EXIT_FAILURE;
    }
    if (packing && request.sheet.empty()) {
        std::cout << "error: a pack needs a --sheet to name the sheet\n";
        return EXIT_FAILURE;
    }
    if (unpacking && !request.outfile.empty()) {
        std::cout << "error: an unpack writes beside the sprites document, and takes no --outfile\n";
        return EXIT_FAILURE;
    }
    const boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    return packing ? packSheet(request, logger) : unpackSheets(request, logger);
}

/**
 * Read the four whole numbers of a "x,y,width,height" rectangle.
 *
 * Each number is read as an int64_t, which is wider than the unsigned int it is stored in, so
 * a negative value is refused here. Read as unsigned, it would wrap to a huge value, and crop()
 * would then reject it as outside the image.
 *
 * @param values filled with x, y, width and height when true is returned
 * @return whether the text was exactly four comma separated non negative numbers
 **/
bool rectangle(const std::string& text, unsigned int* values) {
    std::istringstream stream(text);
    for (unsigned int field = 0; field < 4; ++field) {
        if (field > 0) {
            char comma = 0;
            if (!(stream >> comma) || comma != ',') {
                return false;
            }
        }
        int64_t value = 0;
        if (!(stream >> value) || value < 0 || value > UINT32_MAX) {
            return false;
        }
        values[field] = static_cast<unsigned int>(value);
    }
    // a trailing field is a typo rather than something to ignore
    char extra = 0;
    return !(stream >> extra);
}

int run(int argc, char *argv[]) {
    // setup option parser
    boost::program_options::options_description opts_desc("Allowed options");
    opts_desc.add_options()
        ("help", "produce help message")
        ("info", "display image info")
        ("silent", "don't display any output")
        ("file", boost::program_options::value<std::string>(), "input image filename to be read")
        ("outfile", boost::program_options::value<std::string>(), "image filename to be written")
        ("crop", boost::program_options::value<std::string>(), "cut a rectangle out before writing, as x,y,width,height")
        ("pack", boost::program_options::value<std::vector<std::string> >()->multitoken(),
            "images or directories of images to pack into one sheet, written to outfile or <sheet>.png with sprites.json beside it")
        ("sheet", boost::program_options::value<std::string>(), "the name of the sheet to pack, or the one sheet to unpack")
        ("unpack", boost::program_options::value<std::string>(),
            "a sprites.json whose sprites are each cut out and written to <sheet>/<sprite>.png beside it");

    // parse options
    boost::program_options::variables_map var_map;
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, opts_desc), var_map);
    boost::program_options::notify(var_map);

    // process options
    if (var_map.count("help") != 0) {
        std::cout << opts_desc << "\n";
        exit(EXIT_SUCCESS);
    }

    bool info = false;
    bool silent = false;
    std::string infile;
    std::string outfile;

    if (var_map.count("info") != 0) {
        info = true;
    }
    if (var_map.count("silent") != 0) {
        silent = true;
    }
    if (var_map.count("file") != 0) {
        infile = var_map["file"].as<std::string>();
    }
    if (var_map.count("outfile") != 0) {
        outfile = var_map["outfile"].as<std::string>();
    }

    if (const std::optional<int> done = sheets(var_map)) {
        return *done;
    }

    unsigned int cut[4] = { 0, 0, 0, 0 };
    const bool cropping = var_map.count("crop") > 0;
    if (cropping && !rectangle(var_map["crop"].as<std::string>(), cut)) {
        std::cout << "error: a crop is four whole numbers, x,y,width,height\n";
        return EXIT_FAILURE;
    }
    // a crop with no outfile is a mistyped command, so it is refused rather than run as a read
    if (cropping && outfile.empty()) {
        std::cout << "error: a crop needs an outfile to write the cut image to\n";
        return EXIT_FAILURE;
    }

    if (infile.empty()) {
        std::cout << opts_desc << "\n";
        exit(EXIT_SUCCESS);
    }

    auto logger = boost::make_shared<v3d::log::Logger>();

    v3d::image::Factory factory(logger);
    boost::shared_ptr<v3d::image::Image> image;

    // silent covers the progress lines. Info was asked for by name and prints regardless
    if (!silent) {
        std::cout << "Reading: " << infile << "\n";
    }
    image = factory.read(infile);
    if (!image) {
        std::cout << "error reading file!" << "\n";
        exit(EXIT_FAILURE);
    }

    if (info) {
        std::cout << "Source image width: " << image->width() << "\n";
        std::cout << "Source image height: " << image->height() << "\n";
        std::cout << "Source image bpp: " << static_cast<unsigned int>(image->bpp()) << "\n";
    }

    // the crop happens after info, so that info describes the file that was read whatever
    // else the run does to it
    if (cropping) {
        if (!silent) {
            std::cout << "Cropping: " << cut[2] << "x" << cut[3] << " at " << cut[0] << "," << cut[1] << "\n";
        }
        image = v3d::image::crop(*image, cut[0], cut[1], cut[2], cut[3]);
        if (!image) {
            std::cout << "error: that rectangle is not inside the image!" << "\n";
            return EXIT_FAILURE;
        }
    }

    if (!outfile.empty()) {
        if (!silent) {
            std::cout << "Writing: " << outfile << "\n";
        }
        const std::optional<std::string> encoded = factory.encode(image, v3d::image::Factory::kind(outfile));
        if (!encoded || !v3d::asset::writeFile(outfile, *encoded)) {
            std::cout << "error writing file!" << "\n";
            exit(EXIT_FAILURE);
        }
    }

    return EXIT_SUCCESS;
}

};  // namespace

int main(int argc, char *argv[]) {
    // the option parser throws on a malformed command line, and the image factory's path
    // handling can throw a filesystem error. An exception leaving main aborts with no message.
    // The handler writes with std::fputs rather than std::cout, because the handler itself
    // must not throw
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
