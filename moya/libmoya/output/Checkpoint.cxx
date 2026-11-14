/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/output/Checkpoint.h>

#include <moya/libmoya/film/Channel.h>
#include <moya/libmoya/film/State.h>
#include <moya/libmoya/kernel/film/Pixels.h>
#include <Imath/ImathBox.h>
#include <OpenEXR/ImfChannelList.h>
#include <OpenEXR/ImfCompression.h>
#include <OpenEXR/ImfFrameBuffer.h>
#include <OpenEXR/ImfHeader.h>
#include <OpenEXR/ImfInputFile.h>
#include <OpenEXR/ImfIntAttribute.h>
#include <OpenEXR/ImfOutputFile.h>
#include <OpenEXR/ImfPixelType.h>
#include <OpenEXR/ImfStringAttribute.h>

#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <ios>
#include <iterator>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

namespace v3d::moya::output {

namespace {

/** The header attributes a checkpoint holds its count of passes and its fingerprint in. **/
constexpr const char * const kPasses = "moya/passes";
constexpr const char * const kFingerprint = "moya/fingerprint";

/** The channel of each pixel's stop flag. **/
constexpr const char * const kStopped = "moya.stopped";

/** The names of the floats kernel::film::Pixels lays after a pixel's channels, in order. **/
constexpr const char * const kTail[] = {
    "moya.half.R",
    "moya.half.G",
    "moya.half.B",
    "moya.half.weight",
    "moya.sampleCount",
    "moya.moment.Y2",
    "moya.moment.Y",
    "moya.moment.weight2",
    "moya.weight",
};
static_assert(std::size(kTail) == kernel::film::kTailFloats, "a name for each float after a pixel's channels");

/** The suffixes of a channel of several floats, in order. **/
constexpr const char * const kComponents[4] = { "R", "G", "B", "A" };

/**
 * The name in the file of each float a pixel of these channels holds, in order. OpenEXR keeps one
 * channel of a name, so two floats of one name are refused.
 **/
std::vector<std::string> namesOf(const std::vector<film::Channel> & channels) {
    std::vector<std::string> out { "R", "G", "B", "A" };
    for (const film::Channel & channel : channels) {
        if (channel.components == 1) {
            out.push_back(channel.name);
            continue;
        }
        for (std::uint32_t i = 0; i < channel.components && i < 4; ++i) {
            out.push_back(channel.name + "." + kComponents[i]);
        }
    }
    out.insert(out.end(), std::begin(kTail), std::end(kTail));
    std::set<std::string> seen { kStopped };
    for (const std::string & name : out) {
        if (!seen.insert(name).second) {
            throw std::invalid_argument("a checkpoint would hold two channels named " + name);
        }
    }
    return out;
}

std::string hexOf(std::uint64_t value) {
    std::ostringstream out;
    out << std::hex << std::setw(16) << std::setfill('0') << value;
    return out.str();
}

};  // namespace

void writeCheckpoint(const std::string & path, const film::State & state, std::uint64_t fingerprint) {
    const std::vector<std::string> names = namesOf(state.channels);
    const std::size_t pixels = static_cast<std::size_t>(state.resolution.x) * state.resolution.y;
    if (pixels == 0 || names.size() != state.stored || state.sums.size() != pixels * state.stored || state.stopped.size() != pixels) {
        throw std::invalid_argument("a film state whose floats or flags do not fill its resolution");
    }

    Imf::Header header(static_cast<int>(state.resolution.x), static_cast<int>(state.resolution.y));
    header.compression() = Imf::ZIP_COMPRESSION;
    for (const std::string & name : names) {
        header.channels().insert(name, Imf::Channel(Imf::FLOAT));
    }
    header.channels().insert(kStopped, Imf::Channel(Imf::UINT));
    header.insert(kPasses, Imf::IntAttribute(static_cast<int>(state.passes)));
    header.insert(kFingerprint, Imf::StringAttribute(hexOf(fingerprint)));

    // OpenEXR reads through a pointer that is not const, and writing a file reads nothing back
    Imf::FrameBuffer frame;
    char * const base = const_cast<char *>(reinterpret_cast<const char *>(state.sums.data()));
    const std::size_t across = sizeof(float) * state.stored;
    const std::size_t down = across * state.resolution.x;
    for (std::size_t i = 0; i < names.size(); ++i) {
        frame.insert(names[i], Imf::Slice(Imf::FLOAT, base + (i * sizeof(float)), across, down));
    }
    char * const flags = const_cast<char *>(reinterpret_cast<const char *>(state.stopped.data()));
    frame.insert(kStopped, Imf::Slice(Imf::UINT, flags, sizeof(std::uint32_t), sizeof(std::uint32_t) * state.resolution.x));

    const std::string partial = path + ".partial";
    {
        Imf::OutputFile file(partial.c_str(), header);
        file.setFrameBuffer(frame);
        file.writePixels(static_cast<int>(state.resolution.y));
    }
    boost::filesystem::rename(partial, path);
}

film::State readCheckpoint(const std::string & path, const std::vector<film::Channel> & channels, std::uint64_t fingerprint) {
    Imf::InputFile file(path.c_str());
    const Imf::Header & header = file.header();
    const auto * const passes = header.findTypedAttribute<Imf::IntAttribute>(kPasses);
    const auto * const stamp = header.findTypedAttribute<Imf::StringAttribute>(kFingerprint);
    if (passes == nullptr || stamp == nullptr || passes->value() < 0) {
        throw Refused(path + " is not a checkpoint");
    }
    if (stamp->value() != hexOf(fingerprint)) {
        throw Refused(path + " is a checkpoint of another scene or film");
    }
    const std::vector<std::string> names = namesOf(channels);
    for (const std::string & name : names) {
        const Imf::Channel * const channel = header.channels().findChannel(name);
        if (channel == nullptr || channel->type != Imf::FLOAT) {
            std::string message = path;
            message += " holds no float channel ";
            message += name;
            throw Refused(message);
        }
    }
    const Imf::Channel * const flag = header.channels().findChannel(kStopped);
    if (flag == nullptr || flag->type != Imf::UINT) {
        throw Refused(path + " holds no channel " + std::string(kStopped));
    }

    const Imath::Box2i & window = header.dataWindow();
    film::State out;
    out.resolution = { static_cast<std::uint32_t>(window.max.x - window.min.x + 1), static_cast<std::uint32_t>(window.max.y - window.min.y + 1) };
    out.passes = static_cast<std::uint32_t>(passes->value());
    out.channels = channels;
    out.stored = static_cast<std::uint32_t>(names.size());
    const std::size_t pixels = static_cast<std::size_t>(out.resolution.x) * out.resolution.y;
    out.sums.assign(pixels * out.stored, 0.0F);
    out.stopped.assign(pixels, 0U);

    // a slice is addressed by the data window's coordinates, so its base lies before the pixels
    const std::ptrdiff_t origin = -static_cast<std::ptrdiff_t>(window.min.x) - (static_cast<std::ptrdiff_t>(window.min.y) * out.resolution.x);
    const std::size_t across = sizeof(float) * out.stored;
    char * const base = reinterpret_cast<char *>(out.sums.data()) + (origin * static_cast<std::ptrdiff_t>(across));
    Imf::FrameBuffer frame;
    for (std::size_t i = 0; i < names.size(); ++i) {
        frame.insert(names[i], Imf::Slice(Imf::FLOAT, base + (i * sizeof(float)), across, across * out.resolution.x));
    }
    char * const flags = reinterpret_cast<char *>(out.stopped.data()) + (origin * static_cast<std::ptrdiff_t>(sizeof(std::uint32_t)));
    frame.insert(kStopped, Imf::Slice(Imf::UINT, flags, sizeof(std::uint32_t), sizeof(std::uint32_t) * out.resolution.x));
    file.setFrameBuffer(frame);
    file.readPixels(window.min.y, window.max.y);
    return out;
}

};  // namespace v3d::moya::output
