/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/output/ExrImage.h>

#include <moya/libmoya/output/Chromaticities.h>
#include <Imath/ImathBox.h>
#include <Imath/ImathVec.h>
#include <OpenEXR/ImfChannelList.h>
#include <OpenEXR/ImfChromaticities.h>
#include <OpenEXR/ImfEnvmap.h>
#include <OpenEXR/ImfFrameBuffer.h>
#include <OpenEXR/ImfHeader.h>
#include <OpenEXR/ImfInputFile.h>
#include <OpenEXR/ImfPixelType.h>
#include <OpenEXR/ImfStandardAttributes.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <glm/vec3.hpp>

namespace v3d::moya::output {

namespace {

/** How far apart two chromaticities may lie and name one space. **/
constexpr float kNear = 1.0e-3F;

/** Whether a point of chromaticity lies near another, given as x and y. **/
bool near(const Imath::V2f & point, const float (&xy)[2]) {
    return std::fabs(point.x - xy[0]) <= kNear && std::fabs(point.y - xy[1]) <= kNear;
}

/** The ID of the linear space a header's chromaticities name, or empty for any other. **/
std::string interopOf(const Imf::Header & header) {
    if (!Imf::hasChromaticities(header)) {
        return kLinearIds[kRec709];
    }
    const Imf::Chromaticities & given = Imf::chromaticities(header);
    for (std::size_t i = 0; i < kLinearSpaces; ++i) {
        const auto & xy = kLinearPrimaries[i];
        if (near(given.red, xy[0]) && near(given.green, xy[1]) && near(given.blue, xy[2]) && near(given.white, xy[3])) {
            return kLinearIds[i];
        }
    }
    return {};
}

};  // namespace

ExrImage readExr(const std::string & path) {
    Imf::InputFile file(path.c_str());
    const Imf::Header & header = file.header();
    const Imath::Box2i & window = header.dataWindow();
    ExrImage out;
    out.width = static_cast<std::uint32_t>(window.max.x - window.min.x + 1);
    out.height = static_cast<std::uint32_t>(window.max.y - window.min.y + 1);
    out.interopId = interopOf(header);
    out.cube = Imf::hasEnvmap(header) && Imf::envmap(header) == Imf::ENVMAP_CUBE;
    out.texels.assign(static_cast<std::size_t>(out.width) * out.height, glm::vec3 { 0.0F });
    // a slice is addressed by the data window's coordinates, so its base lies before the texels
    const std::ptrdiff_t origin = -static_cast<std::ptrdiff_t>(window.min.x) - (static_cast<std::ptrdiff_t>(window.min.y) * out.width);
    char * base = reinterpret_cast<char *>(out.texels.data()) + (origin * static_cast<std::ptrdiff_t>(sizeof(glm::vec3)));
    const bool grey = header.channels().findChannel("R") == nullptr && header.channels().findChannel("Y") != nullptr;
    Imf::FrameBuffer buffer;
    const char * const names[3] = { "R", "G", "B" };
    // a channel the file lacks is filled with zero, and a grey file's Y is read as red alone
    for (std::size_t component = 0; component < (grey ? 1U : 3U); ++component) {
        buffer.insert(grey ? "Y" : names[component],
            Imf::Slice(Imf::FLOAT, base + (component * sizeof(float)), sizeof(glm::vec3), sizeof(glm::vec3) * out.width, 1, 1, 0.0));
    }
    file.setFrameBuffer(buffer);
    file.readPixels(window.min.y, window.max.y);
    if (grey) {
        for (glm::vec3 & texel : out.texels) {
            texel = glm::vec3 { texel.x };
        }
    }
    return out;
}

};  // namespace v3d::moya::output
