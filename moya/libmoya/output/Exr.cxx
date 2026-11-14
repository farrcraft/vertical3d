/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/output/Exr.h>

#include <moya/libmoya/film/Channel.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/kernel/film/Pixels.h>
#include <moya/libmoya/output/Chromaticities.h>
#include <moya/libmoya/output/ExrMetadata.h>
#include <moya/libmoya/output/ExrPart.h>
#include <Imath/ImathMatrix.h>
#include <Imath/half.h>
#include <Imath/ImathVec.h>
#include <OpenEXR/ImfChannelList.h>
#include <OpenEXR/ImfChromaticities.h>
#include <OpenEXR/ImfCompression.h>
#include <OpenEXR/ImfFloatAttribute.h>
#include <OpenEXR/ImfFrameBuffer.h>
#include <OpenEXR/ImfHeader.h>
#include <OpenEXR/ImfMultiPartOutputFile.h>
#include <OpenEXR/ImfOutputFile.h>
#include <OpenEXR/ImfOutputPart.h>
#include <OpenEXR/ImfPartType.h>
#include <OpenEXR/ImfPixelType.h>
#include <OpenEXR/ImfStandardAttributes.h>

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <glm/mat4x4.hpp>

namespace v3d::moya::output {

namespace {

/** A part's channels: each one's name in the file, and where its float sits in a pixel. **/
typedef std::vector<std::pair<std::string, std::uint32_t>> FileChannels;

/** Where a part's channel of the count of samples sits, in place of a float of a pixel. **/
constexpr std::uint32_t kCounted = std::numeric_limits<std::uint32_t>::max();

/** The suffixes of a channel of several floats, in order. **/
constexpr const char * const kComponents[4] = { "R", "G", "B", "A" };

/**
 * The channels a part holds, in the order its outputs name them. OpenEXR keeps one channel of a
 * name, so two outputs that give the same name in the file are refused.
 **/
FileChannels channelsOf(const film::Snapshot & snapshot, const ExrPart & part) {
    if (part.outputs.empty()) {
        throw std::invalid_argument("an EXR part with no outputs");
    }
    FileChannels out;
    std::set<std::string> named;
    std::set<std::string> inFile;
    const auto add = [&out, &inFile](const std::string & name, std::uint32_t offset) {
        if (!inFile.insert(name).second) {
            throw std::invalid_argument("an EXR part would hold two channels named " + name);
        }
        out.emplace_back(name, offset);
    };
    for (const std::string & output : part.outputs) {
        if (!named.insert(output).second) {
            throw std::invalid_argument("an EXR part holds " + output + " twice");
        }
        if (output == kBeauty) {
            if (snapshot.stride < kernel::film::kBeautyFloats) {
                throw std::invalid_argument("an EXR part names the beauty, which a snapshot of fewer floats than its four does not hold");
            }
            for (std::uint32_t i = 0; i < kernel::film::kBeautyFloats; ++i) {
                add(kComponents[i], i);
            }
            continue;
        }
        if (output == kSampleCount) {
            if (snapshot.samples.size() != static_cast<std::size_t>(snapshot.resolution.x) * snapshot.resolution.y) {
                throw std::invalid_argument("an EXR part names the count of samples, which the snapshot does not hold");
            }
            add(std::string(kSampleCount), kCounted);
            continue;
        }
        std::size_t index = 0;
        while (index < snapshot.channels.size() && snapshot.channels[index].name != output) {
            ++index;
        }
        if (index == snapshot.channels.size()) {
            throw std::invalid_argument("an EXR part names " + output + ", which is not one of the film's outputs");
        }
        const film::Channel & channel = snapshot.channels[index];
        const std::uint32_t first = snapshot.offsets.at(index);
        if (channel.components == 1) {
            add(channel.name, first);
            continue;
        }
        for (std::uint32_t i = 0; i < channel.components; ++i) {
            add(channel.name + "." + kComponents[i], first + i);
        }
    }
    return out;
}

Imf::Compression compressionOf(Compression compression) {
    switch (compression) {
    case Compression::None:
        return Imf::NO_COMPRESSION;
    case Compression::Rle:
        return Imf::RLE_COMPRESSION;
    case Compression::Zips:
        return Imf::ZIPS_COMPRESSION;
    case Compression::Zip:
        return Imf::ZIP_COMPRESSION;
    case Compression::Piz:
        return Imf::PIZ_COMPRESSION;
    case Compression::Pxr24:
        return Imf::PXR24_COMPRESSION;
    case Compression::B44:
        return Imf::B44_COMPRESSION;
    case Compression::B44a:
        return Imf::B44A_COMPRESSION;
    case Compression::Dwaa:
        return Imf::DWAA_COMPRESSION;
    case Compression::Dwab:
        return Imf::DWAB_COMPRESSION;
    case Compression::Htj2k256:
        return Imf::HTJ2K256_COMPRESSION;
    case Compression::Htj2k32:
        return Imf::HTJ2K32_COMPRESSION;
    }
    return Imf::ZIP_COMPRESSION;
}

/**
 * A glm matrix as Imath's. glm multiplies column vectors and Imath row vectors, so Imath's matrix
 * is the transpose, which holds the same floats at the same indices.
 **/
Imath::M44f matrixOf(const glm::mat4 & matrix) {
    Imath::M44f out;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            out[i][j] = matrix[i][j];
        }
    }
    return out;
}

/** The header of one part. **/
Imf::Header headerOf(const film::Snapshot & snapshot, const ExrPart & part, const FileChannels & channels, const ExrMetadata & metadata) {
    Imf::Header header(static_cast<int>(snapshot.resolution.x), static_cast<int>(snapshot.resolution.y));
    header.compression() = compressionOf(part.compression);
    const Imf::PixelType type = part.precision == Precision::Half ? Imf::HALF : Imf::FLOAT;
    for (const auto & [name, offset] : channels) {
        header.channels().insert(name, Imf::Channel(type));
    }
    if (!part.name.empty()) {
        header.setName(part.name);
    }
    header.setType(Imf::SCANLINEIMAGE);

    if (!metadata.colourSpace.empty()) {
        Imf::addColorInteropID(header, metadata.colourSpace);
    }
    for (std::size_t i = 0; i < kLinearSpaces; ++i) {
        if (metadata.colourSpace == kLinearIds[i]) {
            const auto & xy = kLinearPrimaries[i];
            Imf::addChromaticities(header, Imf::Chromaticities(Imath::V2f(xy[0][0], xy[0][1]), Imath::V2f(xy[1][0], xy[1][1]),
                Imath::V2f(xy[2][0], xy[2][1]), Imath::V2f(xy[3][0], xy[3][1])));
        }
    }
    if (metadata.worldToCamera) {
        Imf::addWorldToCamera(header, matrixOf(*metadata.worldToCamera));
    }
    if (metadata.worldToScreen) {
        Imf::addWorldToNDC(header, matrixOf(*metadata.worldToScreen));
    }
    if (metadata.renderSeconds) {
        header.insert("moya/renderTime", Imf::FloatAttribute(*metadata.renderSeconds));
    }
    return header;
}

/**
 * Where OpenEXR reads each of a part's channels from. A float part reads the snapshot's floats in
 * place. A half part reads halves, converted into the storage given first, because OpenEXR
 * converts between pixel types only when it reads a file.
 **/
Imf::FrameBuffer frameOf(const film::Snapshot & snapshot, const FileChannels & channels, Precision precision,
    const std::vector<float> & counts, std::vector<half> & halves) {
    Imf::FrameBuffer out;
    if (precision == Precision::Float) {
        // OpenEXR reads through a pointer that is not const, and writing a file reads nothing back
        char * const base = const_cast<char *>(reinterpret_cast<const char *>(snapshot.values.data()));
        const std::size_t across = sizeof(float) * snapshot.stride;
        const std::size_t down = across * snapshot.resolution.x;
        for (const auto & [name, offset] : channels) {
            if (offset == kCounted) {
                char * const counted = const_cast<char *>(reinterpret_cast<const char *>(counts.data()));
                out.insert(name, Imf::Slice(Imf::FLOAT, counted, sizeof(float), sizeof(float) * snapshot.resolution.x));
                continue;
            }
            out.insert(name, Imf::Slice(Imf::FLOAT, base + (offset * sizeof(float)), across, down));
        }
        return out;
    }

    const std::size_t pixels = static_cast<std::size_t>(snapshot.resolution.x) * snapshot.resolution.y;
    const std::size_t count = channels.size();
    halves.resize(pixels * count);
    for (std::size_t pixel = 0; pixel < pixels; ++pixel) {
        for (std::size_t i = 0; i < count; ++i) {
            const std::uint32_t offset = channels[i].second;
            halves[(pixel * count) + i] = half(offset == kCounted ? counts[pixel] : snapshot.values[(pixel * snapshot.stride) + offset]);
        }
    }
    char * const base = reinterpret_cast<char *>(halves.data());
    const std::size_t across = sizeof(half) * count;
    const std::size_t down = across * snapshot.resolution.x;
    for (std::size_t i = 0; i < count; ++i) {
        out.insert(channels[i].first, Imf::Slice(Imf::HALF, base + (i * sizeof(half)), across, down));
    }
    return out;
}

};  // namespace

void writeExr(const std::string & path, const film::Snapshot & snapshot, const std::vector<ExrPart> & parts,
    const ExrMetadata & metadata) {
    if (parts.empty()) {
        throw std::invalid_argument("an EXR file with no parts");
    }
    if (snapshot.values.size() != static_cast<std::size_t>(snapshot.resolution.x) * snapshot.resolution.y * snapshot.stride) {
        throw std::invalid_argument("a snapshot whose values do not fill its resolution");
    }
    if (parts.size() > 1) {
        std::set<std::string> names;
        for (const ExrPart & part : parts) {
            if (part.name.empty() || !names.insert(part.name).second) {
                throw std::invalid_argument("each part of a multi-part EXR file needs a name of its own");
            }
        }
    }

    std::vector<FileChannels> channels;
    std::vector<Imf::Header> headers;
    for (const ExrPart & part : parts) {
        channels.push_back(channelsOf(snapshot, part));
        headers.push_back(headerOf(snapshot, part, channels.back(), metadata));
    }

    // OpenEXR writes floats or halves, so each count of samples is written as a float
    std::vector<float> counts;
    counts.reserve(snapshot.samples.size());
    for (const std::uint32_t samples : snapshot.samples) {
        counts.push_back(static_cast<float>(samples));
    }
    const int rows = static_cast<int>(snapshot.resolution.y);
    std::vector<std::vector<half>> halves(parts.size());
    if (parts.size() == 1) {
        Imf::OutputFile file(path.c_str(), headers.front());
        file.setFrameBuffer(frameOf(snapshot, channels.front(), parts.front().precision, counts, halves.front()));
        file.writePixels(rows);
        return;
    }
    Imf::MultiPartOutputFile file(path.c_str(), headers.data(), static_cast<int>(headers.size()));
    for (std::size_t i = 0; i < parts.size(); ++i) {
        Imf::OutputPart part(file, static_cast<int>(i));
        part.setFrameBuffer(frameOf(snapshot, channels[i], parts[i].precision, counts, halves[i]));
        part.writePixels(rows);
    }
}

};  // namespace v3d::moya::output
