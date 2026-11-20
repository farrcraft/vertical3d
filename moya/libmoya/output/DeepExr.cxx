/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/output/DeepExr.h>

#include <moya/libmoya/film/DeepSample.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/output/ExrPart.h>
#include <moya/libmoya/output/internal/ExrWindows.h>
#include <moya/libmoya/output/internal/Field.h>
#include <Imath/half.h>
#include <OpenEXR/ImfChannelList.h>
#include <OpenEXR/ImfCompression.h>
#include <OpenEXR/ImfDeepFrameBuffer.h>
#include <OpenEXR/ImfDeepScanLineOutputFile.h>
#include <OpenEXR/ImfFrameBuffer.h>
#include <OpenEXR/ImfHeader.h>
#include <OpenEXR/ImfIDManifest.h>
#include <OpenEXR/ImfPartType.h>
#include <OpenEXR/ImfPixelType.h>
#include <OpenEXR/ImfStandardAttributes.h>

#include <cstddef>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace v3d::moya::output {

namespace {

/** The colour channels of a sample, in order. **/
constexpr const char * const kColours[] = { "R", "G", "B", "A" };
constexpr std::size_t kColourCount = std::size(kColours);

/** The Color Interop Forum ID of the space the deep samples' colour is in, ACEScg. **/
constexpr const char * const kWorkingSpace = "lin_ap1_scene";

/** The compression a deep file takes for one asked for. **/
Imf::Compression deepCompressionOf(Compression compression) {
    switch (compression) {
        case Compression::None:
            return Imf::NO_COMPRESSION;
        case Compression::Rle:
            return Imf::RLE_COMPRESSION;
        default:
            break;
    }
    return Imf::ZIPS_COMPRESSION;
}

/** One kind of ID's names in the manifest, by MurmurHash3, stored directly in a UINT channel. **/
void named(Imf::IDManifest & manifest, const std::string & channel, const std::map<std::uint32_t, std::string> & names) {
    Imf::IDManifest::ChannelGroupManifest & group = manifest.add(channel);
    group.setComponent("name");
    group.setHashScheme(Imf::IDManifest::MURMURHASH3_32);
    group.setEncodingScheme(Imf::IDManifest::ID_SCHEME);
    for (const auto & [id, name] : names) {
        group.insert(id, name);
    }
}

};  // namespace

void writeDeepExr(const std::string & path, const film::Snapshot & snapshot, Precision precision, Compression compression) {
    const std::size_t width = snapshot.resolution.x;
    const std::size_t pixels = width * snapshot.resolution.y;
    if (snapshot.deep.size() != pixels || pixels == 0) {
        throw std::invalid_argument("a deep file of a snapshot that holds no deep samples for every pixel");
    }
    const bool halves = precision == Precision::Half;
    const internal::Windows windows = internal::windowsOf(snapshot);
    Imf::Header header(windows.display, windows.data);
    header.setType(Imf::DEEPSCANLINE);
    header.compression() = deepCompressionOf(compression);
    for (const char * const name : kColours) {
        header.channels().insert(name, Imf::Channel(halves ? Imf::HALF : Imf::FLOAT));
    }
    header.channels().insert("Z", Imf::Channel(Imf::FLOAT));
    header.channels().insert("ZBack", Imf::Channel(Imf::FLOAT));
    header.channels().insert("objectid", Imf::Channel(Imf::UINT));
    header.channels().insert("materialid", Imf::Channel(Imf::UINT));
    Imf::addColorInteropID(header, kWorkingSpace);
    Imf::IDManifest manifest;
    named(manifest, "objectid", snapshot.deepObjects);
    named(manifest, "materialid", snapshot.deepMaterials);
    Imf::addIDManifest(header, Imf::CompressedIDManifest(manifest));

    std::vector<unsigned int> counts;
    counts.reserve(pixels);
    internal::Field<half> halfColours[kColourCount];
    internal::Field<float> floatColours[kColourCount];
    internal::Field<float> depths;
    internal::Field<float> backs;
    internal::Field<std::uint32_t> objects;
    internal::Field<std::uint32_t> materials;
    for (const std::vector<film::DeepSample> & pixel : snapshot.deep) {
        counts.push_back(static_cast<unsigned int>(pixel.size()));
        for (const film::DeepSample & sample : pixel) {
            const float colour[kColourCount] = { sample.red, sample.green, sample.blue, sample.alpha };
            for (std::size_t i = 0; i < kColourCount; ++i) {
                if (halves) {
                    halfColours[i].values.push_back(half(colour[i]));
                } else {
                    floatColours[i].values.push_back(colour[i]);
                }
            }
            depths.values.push_back(sample.depth);
            backs.values.push_back(sample.back);
            objects.values.push_back(sample.object);
            materials.values.push_back(sample.material);
        }
    }
    Imf::DeepFrameBuffer frame;
    frame.insertSampleCountSlice(Imf::Slice(Imf::UINT, internal::based(reinterpret_cast<char *>(counts.data()), windows.data, sizeof(unsigned int), sizeof(unsigned int) * width),
        sizeof(unsigned int), sizeof(unsigned int) * width));
    for (std::size_t i = 0; i < kColourCount; ++i) {
        if (halves) {
            halfColours[i].place(counts);
            frame.insert(kColours[i], halfColours[i].slice(Imf::HALF, width, windows.data));
        } else {
            floatColours[i].place(counts);
            frame.insert(kColours[i], floatColours[i].slice(Imf::FLOAT, width, windows.data));
        }
    }
    depths.place(counts);
    backs.place(counts);
    objects.place(counts);
    materials.place(counts);
    frame.insert("Z", depths.slice(Imf::FLOAT, width, windows.data));
    frame.insert("ZBack", backs.slice(Imf::FLOAT, width, windows.data));
    frame.insert("objectid", objects.slice(Imf::UINT, width, windows.data));
    frame.insert("materialid", materials.slice(Imf::UINT, width, windows.data));
    Imf::DeepScanLineOutputFile file(path.c_str(), header);
    file.setFrameBuffer(frame);
    file.writePixels(static_cast<int>(snapshot.resolution.y));
}

};  // namespace v3d::moya::output
