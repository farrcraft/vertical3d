/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/output/Checkpoint.h>

#include <moya/libmoya/film/Channel.h>
#include <moya/libmoya/film/Covering.h>
#include <moya/libmoya/film/DeepPoint.h>
#include <moya/libmoya/film/State.h>
#include <moya/libmoya/kernel/film/Combine.h>
#include <moya/libmoya/kernel/film/Pixels.h>
#include <moya/libmoya/output/internal/Entries.h>
#include <Imath/ImathBox.h>
#include <OpenEXR/ImfChannelList.h>
#include <OpenEXR/ImfCompression.h>
#include <OpenEXR/ImfDeepFrameBuffer.h>
#include <OpenEXR/ImfDeepScanLineInputPart.h>
#include <OpenEXR/ImfDeepScanLineOutputPart.h>
#include <OpenEXR/ImfFrameBuffer.h>
#include <OpenEXR/ImfHeader.h>
#include <OpenEXR/ImfInputFile.h>
#include <OpenEXR/ImfIntAttribute.h>
#include <OpenEXR/ImfMultiPartInputFile.h>
#include <OpenEXR/ImfMultiPartOutputFile.h>
#include <OpenEXR/ImfOutputFile.h>
#include <OpenEXR/ImfOutputPart.h>
#include <OpenEXR/ImfPartType.h>
#include <OpenEXR/ImfPixelType.h>
#include <OpenEXR/ImfStringAttribute.h>

#include <bit>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <ios>
#include <iterator>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include <glm/vec2.hpp>

namespace v3d::moya::output {

namespace {

/** The header attributes a checkpoint holds its count of passes and its fingerprint in. **/
constexpr const char * const kPasses = "moya/passes";
constexpr const char * const kFingerprint = "moya/fingerprint";

/** The channel of each pixel's stop flag. **/
constexpr const char * const kStopped = "moya.stopped";

/** The names of a checkpoint's two parts when it holds Cryptomatte coverage. **/
constexpr const char * const kFilmPart = "moya.film";
constexpr const char * const kCoveragePart = "moya.coverage";
constexpr const char * const kDeepPart = "moya.deep";

/** Where the halves of a deep entry's weight, red, green and blue begin among its fields. **/
constexpr std::size_t kDeepWeight = 3;
constexpr std::size_t kDeepRed = 5;
constexpr std::size_t kDeepGreen = 7;
constexpr std::size_t kDeepBlue = 9;

/** The bits of a half of a double's 64. **/
constexpr std::uint32_t kHalfBits = 32;

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
        }
        for (std::uint32_t i = 0; channel.components > 1 && i < channel.components && i < 4; ++i) {
            out.push_back(channel.name + "." + kComponents[i]);
        }
        if (channel.combine == kernel::film::Combine::Closest) {
            out.push_back("moya.key." + channel.name);
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

/** The coverage part's channels: an entry's layer, its ID, and its weight's bits in halves. **/
std::vector<std::string> coverageChannels() {
    return { "layer", "id", "weight.high", "weight.low" };
}

/**
 * The channels of the deep part: an entry's depth's bits, its object and material IDs, and the
 * halves of the bits of its weight and of its red, green and blue.
 **/
std::vector<std::string> deepChannels() {
    return { "depth", "object", "material", "weight.high", "weight.low", "red.high", "red.low", "green.high", "green.low", "blue.high", "blue.low" };
}

/** A double's bits, as its high half and then its low. **/
void push(std::vector<std::vector<std::uint32_t>> & fields, std::size_t first, double value) {
    const auto bits = std::bit_cast<std::uint64_t>(value);
    fields[first].push_back(static_cast<std::uint32_t>(bits >> kHalfBits));
    fields[first + 1U].push_back(static_cast<std::uint32_t>(bits));
}

/** A double from its high half's bits and its low's. **/
double doubleOf(const std::vector<std::vector<std::uint32_t>> & fields, std::size_t first, std::size_t at) {
    return std::bit_cast<double>((static_cast<std::uint64_t>(fields[first][at]) << kHalfBits) | fields[first + 1U][at]);
}

/** The header of a deep part of a name and its entries' channels, of a resolution. **/
Imf::Header deepHeaderOf(const std::string & name, const std::vector<std::string> & channels, const glm::uvec2 & resolution) {
    Imf::Header out(static_cast<int>(resolution.x), static_cast<int>(resolution.y));
    out.setName(name);
    out.setType(Imf::DEEPSCANLINE);
    out.compression() = Imf::ZIPS_COMPRESSION;
    for (const std::string & channel : channels) {
        out.channels().insert(channel, Imf::Channel(Imf::UINT));
    }
    return out;
}

/** Every pixel's coverage as the coverage part's entries. **/
internal::Entries entriesOf(const std::vector<std::vector<film::Covering>> & coverage) {
    internal::Entries out(coverageChannels());
    for (const std::vector<film::Covering> & pixel : coverage) {
        out.counts.push_back(static_cast<unsigned int>(pixel.size()));
        for (const film::Covering & entry : pixel) {
            out.fields[0].push_back(entry.layer);
            out.fields[1].push_back(entry.id);
            push(out.fields, 2, entry.weight);
        }
    }
    out.place();
    return out;
}

/** Every pixel's deep list as the deep part's entries. **/
internal::Entries entriesOf(const std::vector<std::vector<film::DeepPoint>> & deep) {
    internal::Entries out(deepChannels());
    for (const std::vector<film::DeepPoint> & pixel : deep) {
        out.counts.push_back(static_cast<unsigned int>(pixel.size()));
        for (const film::DeepPoint & entry : pixel) {
            out.fields[0].push_back(std::bit_cast<std::uint32_t>(entry.depth));
            out.fields[1].push_back(entry.object);
            out.fields[2].push_back(entry.material);
            push(out.fields, kDeepWeight, entry.weight);
            push(out.fields, kDeepRed, entry.red);
            push(out.fields, kDeepGreen, entry.green);
            push(out.fields, kDeepBlue, entry.blue);
        }
    }
    out.place();
    return out;
}

/**
 * The entries of a checkpoint's deep part of a name, of the channels given, or none for a
 * checkpoint without that part.
 *
 * @throws Refused when the part holds no unsigned channel a field needs
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the file, then the part's name
std::optional<internal::Entries> partOf(const std::string & path, const std::string & name, const std::vector<std::string> & channels) {
    Imf::MultiPartInputFile file(path.c_str());
    for (int index = 0; index < file.parts(); ++index) {
        const Imf::Header & header = file.header(index);
        if (!header.hasName() || header.name() != name || header.type() != Imf::DEEPSCANLINE) {
            continue;
        }
        for (const std::string & channel : channels) {
            const Imf::Channel * const found = header.channels().findChannel(channel);
            if (found == nullptr || found->type != Imf::UINT) {
                std::string message = path;
                message += "'s part ";
                message += name;
                message += " holds no unsigned channel ";
                message += channel;
                throw Refused(message);
            }
        }
        const Imath::Box2i & window = header.dataWindow();
        const int across = window.max.x - window.min.x + 1;
        const int down = window.max.y - window.min.y + 1;
        const auto width = static_cast<std::size_t>(across);
        const std::size_t pixels = width * static_cast<std::size_t>(down);
        const std::ptrdiff_t origin = -static_cast<std::ptrdiff_t>(window.min.x) - (static_cast<std::ptrdiff_t>(window.min.y) * static_cast<std::ptrdiff_t>(width));
        Imf::DeepScanLineInputPart part(file, index);
        internal::Entries entries(channels);
        entries.counts.assign(pixels, 0U);
        for (std::vector<char *> & pointers : entries.pointers) {
            pointers.assign(pixels, nullptr);
        }
        part.setFrameBuffer(entries.frame(origin, width));
        part.readPixelSampleCounts(window.min.y, window.max.y);
        std::size_t total = 0;
        for (const unsigned int count : entries.counts) {
            total += count;
        }
        for (std::vector<std::uint32_t> & field : entries.fields) {
            field.assign(total, 0U);
        }
        entries.place();
        part.setFrameBuffer(entries.frame(origin, width));
        part.readPixels(window.min.y, window.max.y);
        return entries;
    }
    return std::nullopt;
}

/** Every pixel's coverage from a checkpoint's coverage part, or none without one. **/
std::vector<std::vector<film::Covering>> coverageOf(const std::string & path) {
    const std::optional<internal::Entries> entries = partOf(path, kCoveragePart, coverageChannels());
    if (!entries) {
        return {};
    }
    std::vector<std::vector<film::Covering>> out(entries->counts.size());
    std::size_t at = 0;
    for (std::size_t pixel = 0; pixel < out.size(); ++pixel) {
        for (unsigned int i = 0; i < entries->counts[pixel]; ++i, ++at) {
            out[pixel].push_back(film::Covering { .layer = entries->fields[0][at], .id = entries->fields[1][at], .weight = doubleOf(entries->fields, 2, at) });
        }
    }
    return out;
}

/** Every pixel's deep list from a checkpoint's deep part, or none without one. **/
std::vector<std::vector<film::DeepPoint>> deepOf(const std::string & path) {
    const std::optional<internal::Entries> entries = partOf(path, kDeepPart, deepChannels());
    if (!entries) {
        return {};
    }
    std::vector<std::vector<film::DeepPoint>> out(entries->counts.size());
    std::size_t at = 0;
    for (std::size_t pixel = 0; pixel < out.size(); ++pixel) {
        for (unsigned int i = 0; i < entries->counts[pixel]; ++i, ++at) {
            out[pixel].push_back(film::DeepPoint { .depth = std::bit_cast<float>(entries->fields[0][at]),
                .object = entries->fields[1][at],
                .material = entries->fields[2][at],
                .weight = doubleOf(entries->fields, kDeepWeight, at),
                .red = doubleOf(entries->fields, kDeepRed, at),
                .green = doubleOf(entries->fields, kDeepGreen, at),
                .blue = doubleOf(entries->fields, kDeepBlue, at) });
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

    if ((!state.coverage.empty() && state.coverage.size() != pixels) || (!state.deep.empty() && state.deep.size() != pixels)) {
        throw std::invalid_argument("a film state whose coverage or deep lists do not fill its resolution");
    }
    const std::string partial = path + ".partial";
    if (state.coverage.empty() && state.deep.empty()) {
        Imf::OutputFile file(partial.c_str(), header);
        file.setFrameBuffer(frame);
        file.writePixels(static_cast<int>(state.resolution.y));
    } else {
        header.setName(kFilmPart);
        header.setType(Imf::SCANLINEIMAGE);
        std::vector<Imf::Header> headers { header };
        std::vector<internal::Entries> parts;
        if (!state.coverage.empty()) {
            headers.push_back(deepHeaderOf(kCoveragePart, coverageChannels(), state.resolution));
            parts.push_back(entriesOf(state.coverage));
        }
        if (!state.deep.empty()) {
            headers.push_back(deepHeaderOf(kDeepPart, deepChannels(), state.resolution));
            parts.push_back(entriesOf(state.deep));
        }
        Imf::MultiPartOutputFile file(partial.c_str(), headers.data(), static_cast<int>(headers.size()));
        Imf::OutputPart film(file, 0);
        film.setFrameBuffer(frame);
        film.writePixels(static_cast<int>(state.resolution.y));
        for (std::size_t i = 0; i < parts.size(); ++i) {
            Imf::DeepScanLineOutputPart deep(file, static_cast<int>(i + 1U));
            deep.setFrameBuffer(parts[i].frame(0, state.resolution.x));
            deep.writePixels(static_cast<int>(state.resolution.y));
        }
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
    out.coverage = coverageOf(path);
    out.deep = deepOf(path);
    return out;
}

};  // namespace v3d::moya::output
