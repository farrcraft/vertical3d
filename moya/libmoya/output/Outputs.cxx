/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/output/Outputs.h>

#include <api/log/Logger.h>
#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/film/Channel.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/kernel/film/Combine.h>
#include <moya/libmoya/kernel/integrate/FirstHit.h>
#include <moya/libmoya/output/Channel.h>
#include <moya/libmoya/output/Exr.h>
#include <moya/libmoya/output/ExrPart.h>
#include <moya/libmoya/output/Output.h>
#include <moya/libmoya/output/Png.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/output/Cryptomatte.h>
#include <moya/libmoya/output/DeepExr.h>
#include <moya/libmoya/output/ExrMetadata.h>
#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Schema.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string_view>
#include <utility>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::moya::output {

namespace {

/** An integer of the scene's options, or the fallback while the scene has none. **/
std::int32_t option(const scene::Scene & scene, const std::string & name) {
    const std::vector<scene::Handle> options = scene.objects("options");
    if (!options.empty()) {
        return scene.value(options.front(), name).integerArray().front();
    }
    for (const scene::Declaration & declaration : scene.schema().kind(*scene.schema().find("options")).parameters) {
        if (declaration.name == name) {
            return declaration.fallback.integerArray().front();
        }
    }
    return 0;
}

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

/** A channel's combine, by its filter, or by its source when the filter is the default. **/
kernel::film::Combine combineOf(const std::string & filter, Source source, const std::string & sourceName) {
    const std::map<std::string, kernel::film::Combine> named { { "mean", kernel::film::Combine::Mean }, { "sum", kernel::film::Combine::Sum },
        { "closest", kernel::film::Combine::Closest }, { "min", kernel::film::Combine::Least }, { "max", kernel::film::Combine::Most } };
    const auto found = named.find(filter);
    if (found != named.end()) {
        return found->second;
    }
    const std::optional<kernel::integrate::Reading> reading = source == Source::Raw ? readingNamed(sourceName) : std::nullopt;
    const bool nearest = reading == kernel::integrate::Reading::Depth || reading == kernel::integrate::Reading::Position ||
        reading == kernel::integrate::Reading::Instance || reading == kernel::integrate::Reading::Face;
    return nearest ? kernel::film::Combine::Closest : kernel::film::Combine::Mean;
}

Format formatNamed(const std::string & name) {
    if (name == "deepexr") {
        return Format::DeepExr;
    }
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

std::optional<kernel::integrate::Reading> readingNamed(const std::string & name) {
    const std::map<std::string, kernel::integrate::Reading> named { { "depth", kernel::integrate::Reading::Depth },
        { "position", kernel::integrate::Reading::Position }, { "normal", kernel::integrate::Reading::Normal },
        { "st", kernel::integrate::Reading::Texture }, { "instance", kernel::integrate::Reading::Instance },
        { "face", kernel::integrate::Reading::Face }, { "motion", kernel::integrate::Reading::Motion }, { "albedo", kernel::integrate::Reading::Albedo },
        { "denoisingNormal", kernel::integrate::Reading::DenoisingNormal } };
    const auto found = named.find(name);
    return found == named.end() ? std::nullopt : std::optional<kernel::integrate::Reading>(found->second);
}

std::vector<Channel> channelsOf(const scene::Scene & scene) {
    std::vector<Channel> channels;
    std::vector<scene::Handle> seen;
    for (const scene::Handle & output : scene.objects("output")) {
        for (const scene::Handle & channel : scene.value(output, "channels").handleArray()) {
            const auto same = [&channel](const scene::Handle & held) {
                return held == channel;
            };
            if (std::any_of(seen.begin(), seen.end(), same)) {
                continue;
            }
            seen.push_back(channel);
            const Source source = sourceNamed(stringOf(scene, channel, "sourceType"));
            const std::string & sourceName = stringOf(scene, channel, "sourceName");
            channels.push_back(Channel { .film = { .name = scene.name(channel),
                                             .components = static_cast<std::uint32_t>(scene.value(channel, "components").integerArray().front()),
                                             .combine = combineOf(stringOf(scene, channel, "filter"), source, sourceName) },
                .source = source,
                .sourceName = sourceName,
                .ranks = static_cast<std::uint32_t>(std::max(scene.value(channel, "ranks").integerArray().front(), 1)),
                .denoised = scene.value(channel, "denoise").integerArray().front() != 0 });
        }
    }
    const bool denoising = denoisesBeauty(scene) || std::any_of(channels.begin(), channels.end(), [](const Channel & each) { return each.denoised; });
    for (const auto & [name, reading] : { std::pair { kDenoiseAlbedo, "albedo" }, std::pair { kDenoiseNormal, "denoisingNormal" } }) {
        const bool held = std::any_of(channels.begin(), channels.end(), [&](const Channel & each) {
            return each.source == Source::Raw && each.sourceName == reading && each.film.components == 3U;
        });
        if (denoising && !held) {
            channels.push_back(Channel {
                .film = { .name = std::string(name), .components = 3U, .combine = kernel::film::Combine::Mean }, .source = Source::Raw, .sourceName = reading });
        }
    }
    return channels;
}

bool denoisesBeauty(const scene::Scene & scene) {
    return option(scene, "denoise") != 0;
}

std::string denoisedOf(const std::string & channel) {
    return channel + std::string(kDenoised);
}

std::vector<film::Channel> filmChannelsOf(const scene::Scene & scene) {
    std::vector<film::Channel> film;
    for (const Channel & channel : channelsOf(scene)) {
        const std::optional<Cryptomatte> layer = cryptomatteOf(channel);
        if (!layer) {
            film.push_back(channel.film);
            if (channel.denoised) {
                film.push_back(film::Channel { .name = denoisedOf(channel.film.name), .components = 3U, .combine = kernel::film::Combine::Sum });
            }
            continue;
        }
        for (const std::string & ranked : rankChannelsOf(*layer)) {
            film.push_back(film::Channel { .name = ranked, .components = 2U * kRanksPerChannel, .combine = kernel::film::Combine::Sum });
        }
    }
    if (denoisesBeauty(scene)) {
        film.push_back(film::Channel { .name = denoisedOf(std::string(kBeauty)), .components = 3U, .combine = kernel::film::Combine::Sum });
    }
    return film;
}

film::Snapshot composited(const film::Snapshot & snapshot, const glm::vec3 & background) {
    film::Snapshot out = snapshot;
    const std::string denoised = denoisedOf(std::string(kBeauty));
    const bool cleaned = std::any_of(out.channels.begin(), out.channels.end(), [&denoised](const film::Channel & channel) { return channel.name == denoised; });
    const std::uint32_t cleanAt = cleaned ? out.offset(denoised) : 0U;
    const std::size_t pixels = static_cast<std::size_t>(out.resolution.x) * out.resolution.y;
    for (std::size_t pixel = 0; pixel < pixels && out.stride >= 4U; ++pixel) {
        const std::size_t first = pixel * out.stride;
        const float uncovered = 1.0F - out.values[first + 3U];
        for (std::size_t i = 0; i < 3U; ++i) {
            const float added = uncovered * background[static_cast<glm::length_t>(i)];
            out.values[first + i] += added;
            if (cleaned) {
                out.values[first + cleanAt + i] += added;
            }
        }
        out.values[first + 3U] = 1.0F;
    }
    return out;
}

std::vector<Output> outputsOf(const scene::Scene & scene) {
    std::vector<Output> outputs;
    for (const scene::Handle & made : scene.objects("output")) {
        Output output { .file = stringOf(scene, made, "file"),
            .format = formatNamed(stringOf(scene, made, "format")),
            .channels = {},
            .precision = stringOf(scene, made, "precision") == "float" ? Precision::Float : Precision::Half,
            .compression = compressionNamed(stringOf(scene, made, "compression")) };
        const std::vector<Channel> every = channelsOf(scene);
        for (const scene::Handle & channel : scene.value(made, "channels").handleArray()) {
            const auto named = std::find_if(every.begin(), every.end(), [&](const Channel & held) { return held.film.name == scene.name(channel); });
            const std::optional<Cryptomatte> layer = named == every.end() ? std::nullopt : cryptomatteOf(*named);
            if (!layer) {
                output.channels.push_back(scene.name(channel));
                if (named != every.end() && named->denoised) {
                    output.channels.push_back(denoisedOf(named->film.name));
                }
                continue;
            }
            const std::vector<std::string> ranked = rankChannelsOf(*layer);
            output.channels.insert(output.channels.end(), ranked.begin(), ranked.end());
            output.floats.insert(output.floats.end(), ranked.begin(), ranked.end());
            output.cryptomattes.push_back(layer->name);
        }
        if (denoisesBeauty(scene) && output.format == Format::Exr) {
            output.channels.insert(output.channels.begin(), denoisedOf(std::string(kBeauty)));
        }
        const std::vector<scene::Handle> options = scene.objects("options");
        if (!options.empty()) {
            const scene::Value & background = scene.value(options.front(), "background");
            const std::vector<float> & floats = background.floatArray();
            if (floats.size() >= 3U) {
                output.background = glm::vec3 { floats[0], floats[1], floats[2] };
                output.backgroundSpace = background.space();
            }
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
    const film::Snapshot written = output.background ? composited(snapshot, config.input(output.backgroundSpace).apply(*output.background)) : snapshot;
    switch (output.format) {
    case Format::Exr: {
        ExrPart part { .name = "", .outputs = { std::string(kBeauty) }, .precision = output.precision, .compression = output.compression, .floats = output.floats };
        part.outputs.insert(part.outputs.end(), output.channels.begin(), output.channels.end());
        ExrMetadata metadata;
        for (const std::string & layer : output.cryptomattes) {
            const std::string prefix = "cryptomatte/" + keyOf(layer) + "/";
            const auto manifest = written.manifests.find(layer);
            metadata.strings.emplace_back(prefix + "name", layer);
            metadata.strings.emplace_back(prefix + "hash", "MurmurHash3_32");
            metadata.strings.emplace_back(prefix + "conversion", "uint32_to_float32");
            metadata.strings.emplace_back(prefix + "manifest", manifest == written.manifests.end() ? std::string("{}") : manifest->second);
        }
        writeExr(output.file, written, { part }, metadata);
        return;
    }
    case Format::DeepExr:
        writeDeepExr(output.file, written, output.precision, output.compression);
        return;
    case Format::Png: {
        const std::string display = config.defaultDisplay();
        writePng(output.file, written, config.display(display, config.defaultView(display)), logger);
        return;
    }
    case Format::Framebuffer:
        logger->get()->info("{} is a framebuffer, which moya has no window to show, so it is not written", output.file);
        return;
    }
}

};  // namespace v3d::moya::output
