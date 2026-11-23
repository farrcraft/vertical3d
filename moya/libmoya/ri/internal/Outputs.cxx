/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include "Outputs.h"

#include <api/rib/Declaration.h>
#include <api/rib/Parameters.h>
#include <api/rib/requests/Display.h>
#include <api/rib/requests/DisplayChannel.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace v3d::moya::ri::internal {

namespace {

/** Whether a file name ends with a suffix, compared exactly. **/
bool endsWith(const std::string & text, const std::string & suffix) {
    return text.size() >= suffix.size() && text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

/** The words of a declaration, apart at spaces. **/
std::vector<std::string> wordsOf(const std::string & text) {
    std::vector<std::string> out;
    std::size_t at = 0;
    while (at < text.size()) {
        const std::size_t next = std::min(text.find(' ', at), text.size());
        if (next > at) {
            out.push_back(text.substr(at, next - at));
        }
        at = next + 1U;
    }
    return out;
}

};  // namespace

Outputs::Outputs(scene::Scene * scene) : scene_(scene) {}

Outputs::Output & Outputs::output(const std::string & file) {
    const auto found = outputs_.find(file);
    if (found != outputs_.end()) {
        return found->second;
    }
    const scene::Handle made = scene_->create("output", file);
    scene_->set(made, "file", scene::Value::string(file));
    return outputs_[file] = Output { .handle = made, .channels = {} };
}

void Outputs::setChannels(Output * output, const std::vector<scene::Handle> & channels) {
    output->channels = channels;
    scene_->set(output->handle, "channels", scene::Value::handles(channels));
}

void Outputs::display(const v3d::rib::requests::Display & request) {
    const bool adds = !request.name.empty() && request.name.front() == '+';
    const std::string file = adds ? request.name.substr(1) : request.name;
    if (!adds) {
        for (const auto & [name, made] : outputs_) {
            if (name != file) {
                scene_->remove(made.handle);
            }
        }
        std::erase_if(outputs_, [&file](const auto & named) { return named.first != file; });
    }

    Output & made = output(file);
    std::string format = endsWith(file, ".png") ? "png" : "exr";
    if (request.type == "framebuffer") {
        format = "framebuffer";
    } else if (request.type == "deepexr") {
        format = "deepexr";
    }
    scene_->set(made.handle, "format", scene::Value::string(format));

    std::vector<scene::Handle> channels;
    std::size_t from = 0;
    while (from <= request.mode.size()) {
        const std::size_t comma = std::min(request.mode.find(',', from), request.mode.size());
        std::string name = request.mode.substr(from, comma - from);
        name.erase(0, name.find_first_not_of(' '));
        name.erase(name.find_last_not_of(' ') + 1U);
        from = comma + 1U;
        if (name.empty() || name == "rgb" || name == "rgba" || name == "Ci" || name == "a") {
            continue;
        }
        if (channels_.find(name) == channels_.end()) {
            channels_[name] = channelOf(name, name, 0);
        }
        channels.push_back(channels_[name]);
    }
    setChannels(&made, channels);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the channel's name, then its source
scene::Handle Outputs::channelOf(const std::string & name, const std::string & source, std::int32_t components) {
    const scene::Handle channel = scene_->create("channel", name);
    const std::string lpe = "lpe:";
    const bool expression = source.starts_with(lpe);
    static const std::map<std::string, std::string> kRenderMan { { "z", "depth" }, { "P", "position" }, { "Nn", "normal" } };
    const auto renamed = kRenderMan.find(source);
    scene_->set(channel, "sourceType", scene::Value::string(expression ? "lpe" : "raw"));
    std::string named = renamed != kRenderMan.end() ? renamed->second : source;
    if (expression) {
        named = source.substr(lpe.size());
    }
    scene_->set(channel, "sourceName", scene::Value::string(named));
    if (components > 0) {
        scene_->set(channel, "components", scene::Value::integer(components));
    }
    return channel;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the channel before, then its successor
void Outputs::replace(const scene::Handle & old, const scene::Handle & channel) {
    for (auto & [file, made] : outputs_) {
        std::vector<scene::Handle> named = made.channels;
        bool found = false;
        for (scene::Handle & held : named) {
            if (held == old) {
                held = channel;
                found = true;
            }
        }
        if (found) {
            setChannels(&made, named);
        }
    }
}

bool Outputs::declare(const v3d::rib::requests::DisplayChannel & request, const v3d::rib::ParameterList & parameters) {
    const std::vector<std::string> words = wordsOf(request.declaration);
    if (words.empty()) {
        return false;
    }
    const std::string & name = words.back();
    v3d::rib::Declaration declaration;
    const bool typed = words.size() > 1U && v3d::rib::Declaration::parse(request.declaration, &declaration);
    const std::int32_t components = typed && declaration.floats() == 3U ? 3 : 1;
    std::string source = parameters.has("source") ? parameters.string("source") : name;
    const std::size_t space = source.rfind(' ');
    source = space == std::string::npos ? source : source.substr(space + 1U);

    // a channel declared again is removed, and each output that named it names the new one
    const auto declared = channels_.find(name);
    std::optional<scene::Handle> old;
    if (declared != channels_.end()) {
        old = declared->second;
        scene_->remove(declared->second);
    }
    const scene::Handle channel = channelOf(name, source, components);
    static const std::map<std::string, std::string> kFilters { { "min", "min" }, { "max", "max" }, { "sum", "sum" }, { "zmin", "closest" } };
    const auto filter = kFilters.find(parameters.has("filter") ? parameters.string("filter") : std::string());
    if (filter != kFilters.end()) {
        scene_->set(channel, "filter", scene::Value::string(filter->second));
    }
    if (parameters.has("denoise")) {
        scene_->set(channel, "denoise", scene::Value::integer(parameters.number("denoise", 0.0F) != 0.0F ? 1 : 0));
    }
    channels_[name] = channel;
    if (old) {
        replace(*old, channel);
    }
    return true;
}

bool Outputs::cryptomatte(const v3d::rib::ParameterList & parameters) {
    static const std::map<std::string, std::pair<std::string, std::string>> kLayers { { "Name", { "CryptoObject", "crypto_object" } },
        { "object", { "CryptoObject", "crypto_object" } }, { "Material", { "CryptoMaterial", "crypto_material" } },
        { "material", { "CryptoMaterial", "crypto_material" } }, { "Path", { "CryptoAsset", "crypto_asset" } }, { "asset", { "CryptoAsset", "crypto_asset" } } };
    const auto layer = kLayers.find(parameters.has("layer") ? parameters.string("layer") : std::string("Name"));
    if (layer == kLayers.end()) {
        return false;
    }
    const std::string file = parameters.has("filename") ? parameters.string("filename") : std::string("cryptomatte.exr");
    Output & made = output(file);
    scene_->set(made.handle, "format", scene::Value::string("exr"));
    scene_->set(made.handle, "precision", scene::Value::string("float"));
    const auto & [name, source] = layer->second;
    if (channels_.find(name) == channels_.end()) {
        channels_[name] = channelOf(name, source, 0);
    }
    setChannels(&made, { channels_[name] });
    return true;
}

};  // namespace v3d::moya::ri::internal
