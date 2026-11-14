/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/output/Outputs.h>

#include <api/log/Logger.h>
#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/film/Channel.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/output/Channel.h>
#include <moya/libmoya/output/Exr.h>
#include <moya/libmoya/output/ExrPart.h>
#include <moya/libmoya/output/Output.h>
#include <moya/libmoya/output/Png.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::moya::output {

namespace {

const std::string & stringOf(const scene::Scene & scene, const scene::Handle & object, const std::string & parameter) {
    return scene.value(object, parameter).stringArray().front();
}

Source sourceNamed(const std::string & name) {
    if (name == "primvar") {
        return Source::Primvar;
    }
    if (name == "lpe") {
        return Source::Lpe;
    }
    return Source::Raw;
}

Format formatNamed(const std::string & name) {
    if (name == "png") {
        return Format::Png;
    }
    if (name == "framebuffer") {
        return Format::Framebuffer;
    }
    return Format::Exr;
}

Compression compressionNamed(const std::string & name) {
    const std::map<std::string, Compression> named { { "none", Compression::None }, { "rle", Compression::Rle }, { "zips", Compression::Zips },
        { "zip", Compression::Zip }, { "piz", Compression::Piz }, { "pxr24", Compression::Pxr24 }, { "b44", Compression::B44 },
        { "b44a", Compression::B44a }, { "dwaa", Compression::Dwaa }, { "dwab", Compression::Dwab }, { "htj2k256", Compression::Htj2k256 },
        { "htj2k32", Compression::Htj2k32 } };
    const auto found = named.find(name);
    return found == named.end() ? Compression::Zip : found->second;
}

};  // namespace

std::vector<Channel> channelsOf(const scene::Scene & scene) {
    std::vector<Channel> channels;
    std::vector<scene::Handle> seen;
    for (const scene::Handle & output : scene.objects("output")) {
        for (const scene::Handle & channel : scene.value(output, "channels").handleArray()) {
            if (std::find(seen.begin(), seen.end(), channel) != seen.end()) {
                continue;
            }
            seen.push_back(channel);
            channels.push_back(Channel { .film = { .name = scene.name(channel),
                                             .components = static_cast<std::uint32_t>(scene.value(channel, "components").integerArray().front()) },
                .source = sourceNamed(stringOf(scene, channel, "sourceType")),
                .sourceName = stringOf(scene, channel, "sourceName") });
        }
    }
    return channels;
}

std::vector<film::Channel> filmChannelsOf(const scene::Scene & scene) {
    std::vector<film::Channel> film;
    for (const Channel & channel : channelsOf(scene)) {
        film.push_back(channel.film);
    }
    return film;
}

std::vector<Output> outputsOf(const scene::Scene & scene) {
    std::vector<Output> outputs;
    for (const scene::Handle & made : scene.objects("output")) {
        Output output { .file = stringOf(scene, made, "file"),
            .format = formatNamed(stringOf(scene, made, "format")),
            .channels = {},
            .precision = stringOf(scene, made, "precision") == "float" ? Precision::Float : Precision::Half,
            .compression = compressionNamed(stringOf(scene, made, "compression")) };
        for (const scene::Handle & channel : scene.value(made, "channels").handleArray()) {
            output.channels.push_back(scene.name(channel));
        }
        outputs.push_back(output);
    }
    return outputs;
}

Format formatOfFile(const std::string & file) {
    const std::string png = ".png";
    if (file.size() < png.size()) {
        return Format::Exr;
    }
    std::string ending = file.substr(file.size() - png.size());
    std::transform(ending.begin(), ending.end(), ending.begin(), [](unsigned char letter) { return static_cast<char>(std::tolower(letter)); });
    return ending == png ? Format::Png : Format::Exr;
}

void write(const Output & output, const film::Snapshot & snapshot, const colour::Config & config, const boost::shared_ptr<log::Logger> & logger) {
    switch (output.format) {
    case Format::Exr: {
        ExrPart part { .name = "", .outputs = { std::string(kBeauty) }, .precision = output.precision, .compression = output.compression };
        part.outputs.insert(part.outputs.end(), output.channels.begin(), output.channels.end());
        writeExr(output.file, snapshot, { part }, {});
        return;
    }
    case Format::Png: {
        const std::string display = config.defaultDisplay();
        writePng(output.file, snapshot, config.display(display, config.defaultView(display)), logger);
        return;
    }
    case Format::Framebuffer:
        logger->get()->info("{} is a framebuffer, which moya has no window to show, so it is not written", output.file);
        return;
    }
}

};  // namespace v3d::moya::output
