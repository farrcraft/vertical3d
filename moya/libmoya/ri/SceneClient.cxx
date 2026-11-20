/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include "SceneClient.h"

#include <api/log/Logger.h>
#include <api/rib/Sampling.h>
#include <api/rib/Bases.h>
#include <api/rib/Handler.h>
#include <api/rib/Parameters.h>
#include <api/rib/requests/Basis.h>
#include <api/rib/requests/Clipping.h>
#include <api/rib/requests/Cone.h>
#include <api/rib/requests/CropWindow.h>
#include <api/rib/requests/Curves.h>
#include <api/rib/requests/Cylinder.h>
#include <api/rib/requests/DepthOfField.h>
#include <api/rib/requests/Disk.h>
#include <api/rib/requests/Display.h>
#include <api/rib/requests/DisplayChannel.h>
#include <api/rib/requests/Format.h>
#include <api/rib/requests/Hyperboloid.h>
#include <api/rib/requests/LightSource.h>
#include <api/rib/requests/MakeTexture.h>
#include <api/rib/requests/Paraboloid.h>
#include <api/rib/requests/PointsPolygons.h>
#include <api/rib/requests/ScreenWindow.h>
#include <api/rib/requests/ShadingNode.h>
#include <api/rib/requests/Shutter.h>
#include <api/rib/requests/Sphere.h>
#include <api/rib/requests/SubdivisionMesh.h>
#include <api/rib/requests/Torus.h>
#include <api/rib/requests/Volume.h>
#include <api/type/Checked.h>
#include <moya/libmoya/ri/internal/Document.h>
#include <moya/libmoya/ri/internal/Network.h>
#include <moya/libmoya/ri/internal/Primvars.h>
#include <moya/libmoya/ri/internal/Sampled.h>
#include <moya/libmoya/ri/internal/Shape.h>
#include <moya/libmoya/ri/internal/Shapes.h>
#include <moya/libmoya/ri/internal/Volumes.h>
#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Kind.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Schema.h>
#include <moya/libmoya/scene/Value.h>
#include <moya/libmoya/texture/Maker.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <exception>
#include <functional>
#include <limits>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::ri {

SceneClient::SceneClient(scene::Scene & scene, const boost::shared_ptr<v3d::log::Logger> & logger) :
    scene_(scene), logger_(logger), warnings_(logger), made_(&scene), lights_(&scene, &made_, &warnings_, logger),
    deformations_(&scene, &warnings_, logger, current_.basis) {
    options_ = scene_.create("options", "options");
    camera_ = scene_.create("camera", "camera");
    scene_.set(options_, "camera", scene::Value::handle(camera_));
}

bool SceneClient::endsWith(const std::string & text, const std::string & suffix) {
    return text.size() >= suffix.size() && text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the path given, then the path it replaces
std::string SceneClient::searchPath(const std::string & given, const std::string & previous) {
    std::string path;
    for (const char letter : given) {
        if (letter == '&') {
            path += previous;
        } else {
            path += letter;
        }
    }
    return path;
}

bool SceneClient::inIntegers(float value, float least) {
    return value >= least && value <= static_cast<float>(std::numeric_limits<std::int32_t>::max());
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the request, then its name
void SceneClient::unread(const std::string & request, const std::string & name, const v3d::rib::ParameterList & parameters,
    const std::set<std::string> & read) {
    for (const std::string & parameter : parameters.names()) {
        if (read.contains(parameter)) {
            continue;
        }
        std::string named = request;
        if (!name.empty()) {
            named.append(" \"").append(name).append("\"");
        }
        std::string key = named;
        key.append(" ").append(parameter);
        std::string message = named;
        message.append("'s \"").append(parameter).append("\" is not read, so it changes nothing");
        warnings_.once(key, message);
    }
}

void SceneClient::hider(const std::string & name, const v3d::rib::ParameterList & parameters) {
    if (name != "raytrace" && name != "hidden") {
        warnings_.once("hider " + name, "moya renders by path tracing alone, so Hider \"" + name + "\" changes nothing");
    }
    unread("Hider", name, parameters, {});
}

void SceneClient::frame(const std::function<void()> & committed) {
    committed_ = committed;
}

bool SceneClient::resolution(int width, int height) {
    if (!v3d::rib::resolution(static_cast<float>(width)) || !v3d::rib::resolution(static_cast<float>(height))) {
        return false;
    }
    resolution_ = glm::ivec2(width, height);
    scene_.set(options_, "resolution", scene::Value::integers({ width, height }));
    return true;
}

void SceneClient::format(const v3d::rib::requests::Format & request) {
    if (!resolution_) {
        const int width = request.width == 0 ? kWidth : static_cast<int>(request.width);
        const int height = request.height == 0 ? kHeight : static_cast<int>(request.height);
        scene_.set(options_, "resolution", scene::Value::integers({ width, height }));
    }
    scene_.set(options_, "pixelAspect", scene::Value::number(request.pixelAspect));
}

void SceneClient::frameAspectRatio(float aspect) {
    scene_.set(options_, "frameAspect", scene::Value::number(aspect));
}

void SceneClient::screenWindow(const v3d::rib::requests::ScreenWindow & request) {
    scene_.set(camera_, "screen", scene::Value::floats({ request.left, request.right, request.bottom, request.top }));
}

void SceneClient::cropWindow(const v3d::rib::requests::CropWindow & request) {
    scene_.set(options_, "crop", scene::Value::floats({ request.xmin, request.xmax, request.ymin, request.ymax }));
}

void SceneClient::projection(const std::string & name, const v3d::rib::ParameterList & parameters) {
    if (name != "perspective" && name != "orthographic") {
        logger_->get()->warn("moya's camera is perspective or orthographic, so Projection \"{}\" changes nothing", name);
        return;
    }
    scene_.set(camera_, "projection", scene::Value::string(name));
    if (parameters.has("fov")) {
        scene_.set(camera_, "fov", scene::Value::number(parameters.number("fov", 0.0F)));
    }
}

void SceneClient::clipping(const v3d::rib::requests::Clipping & request) {
    scene_.set(camera_, "near", scene::Value::number(request.hither));
    scene_.set(camera_, "far", scene::Value::number(request.yon));
}

void SceneClient::depthOfField(const v3d::rib::requests::DepthOfField & request) {
    scene_.set(camera_, "fstop", scene::Value::number(request.fstop));
    scene_.set(camera_, "focalLength", scene::Value::number(request.focalLength));
    scene_.set(camera_, "focalDistance", scene::Value::number(request.focalDistance));
}

void SceneClient::shutter(const v3d::rib::requests::Shutter & request) {
    scene_.set(camera_, "shutter", scene::Value::floats({ request.open, request.close }));
}

void SceneClient::pixelSamples(const glm::uvec2 & samples) {
    scene_.set(options_, "samples", scene::Value::integer(static_cast<std::int32_t>(samples.x * samples.y)));
}

void SceneClient::pixelFilter(v3d::rib::Filter filter, float xwidth, float ywidth) {
    scene_.set(options_, "filter", scene::Value::string(v3d::rib::filterName(filter)));
    scene_.set(options_, "filterWidth", scene::Value::floats({ xwidth, ywidth }));
}

void SceneClient::pixelVariance(float variation) {
    scene_.set(options_, "noiseThreshold", scene::Value::number(variation));
}

scene::Handle SceneClient::output(const std::string & file) {
    const auto found = outputs_.find(file);
    if (found != outputs_.end()) {
        return found->second;
    }
    const scene::Handle made = scene_.create("output", file);
    scene_.set(made, "file", scene::Value::string(file));
    outputs_[file] = made;
    return made;
}

void SceneClient::display(const v3d::rib::requests::Display & request, const v3d::rib::ParameterList & parameters) {
    unread("Display", "", parameters, {});
    const bool adds = !request.name.empty() && request.name.front() == '+';
    const std::string file = adds ? request.name.substr(1) : request.name;
    if (!adds) {
        for (const auto & [name, made] : outputs_) {
            if (name != file) {
                scene_.remove(made);
            }
        }
        std::erase_if(outputs_, [&file](const auto & named) { return named.first != file; });
    }

    const scene::Handle made = output(file);
    std::string format = endsWith(file, ".png") ? "png" : "exr";
    if (request.type == "framebuffer") {
        format = "framebuffer";
    } else if (request.type == "deepexr") {
        format = "deepexr";
    }
    scene_.set(made, "format", scene::Value::string(format));

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
    scene_.set(made, "channels", scene::Value::handles(channels));
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the channel's name, then its source
scene::Handle SceneClient::channelOf(const std::string & name, const std::string & source, std::int32_t components) {
    const scene::Handle channel = scene_.create("channel", name);
    const std::string lpe = "lpe:";
    const bool expression = source.starts_with(lpe);
    static const std::map<std::string, std::string> kRenderMan { { "z", "depth" }, { "P", "position" }, { "Nn", "normal" } };
    const auto renamed = kRenderMan.find(source);
    scene_.set(channel, "sourceType", scene::Value::string(expression ? "lpe" : "raw"));
    std::string named = renamed != kRenderMan.end() ? renamed->second : source;
    if (expression) {
        named = source.substr(lpe.size());
    }
    scene_.set(channel, "sourceName", scene::Value::string(named));
    if (components > 0) {
        scene_.set(channel, "components", scene::Value::integer(components));
    }
    return channel;
}

void SceneClient::displayChannel(const v3d::rib::requests::DisplayChannel & request,
    const v3d::rib::ParameterList & parameters) {
    std::vector<std::string> words;
    std::size_t at = 0;
    while (at < request.declaration.size()) {
        const std::size_t next = std::min(request.declaration.find(' ', at), request.declaration.size());
        if (next > at) {
            words.push_back(request.declaration.substr(at, next - at));
        }
        at = next + 1U;
    }
    if (words.empty()) {
        warnings_.once("display channel", "a DisplayChannel that declares no name makes no channel");
        return;
    }
    const std::string name = words.back();
    static const std::set<std::string> kTriples { "color", "point", "vector", "normal" };
    const bool triple = std::any_of(words.begin(), words.end() - 1, [](const std::string & word) { return kTriples.contains(word); });
    std::string source = parameters.has("source") ? parameters.string("source") : name;
    const std::size_t space = source.rfind(' ');
    source = space == std::string::npos ? source : source.substr(space + 1U);
    if (channels_.find(name) != channels_.end()) {
        scene_.remove(channels_[name]);
    }
    const scene::Handle channel = channelOf(name, source, triple ? 3 : 1);
    static const std::map<std::string, std::string> kFilters { { "min", "min" }, { "max", "max" }, { "sum", "sum" }, { "zmin", "closest" } };
    const auto filter = kFilters.find(parameters.has("filter") ? parameters.string("filter") : std::string());
    if (filter != kFilters.end()) {
        scene_.set(channel, "filter", scene::Value::string(filter->second));
    }
    if (parameters.has("denoise")) {
        scene_.set(channel, "denoise", scene::Value::integer(parameters.number("denoise", 0.0F) != 0.0F ? 1 : 0));
    }
    channels_[name] = channel;
}

void SceneClient::displayFilter(const v3d::rib::requests::ShadingNode & request, const v3d::rib::ParameterList & parameters) {
    if (request.type != "PxrBackgroundDisplayFilter") {
        warnings_.once("display filter " + request.type, "moya runs no display filter " + request.type + ", so it changes nothing");
        return;
    }
    background(parameters, "backgroundColor");
}

void SceneClient::imager(const std::string & name, const v3d::rib::ParameterList & parameters) {
    if (name.empty()) {
        return;
    }
    if (name != "background") {
        warnings_.once("imager " + name, "moya runs no imager " + name + ", so it changes nothing");
        return;
    }
    background(parameters, "background");
}

void SceneClient::background(const v3d::rib::ParameterList & parameters, const std::string & colour) {
    glm::vec3 given(0.0F);
    if (parameters.has(colour) && parameters.floats(colour).size() >= 3U) {
        const std::vector<float> & floats = parameters.floats(colour);
        given = glm::vec3 { floats[0], floats[1], floats[2] };
    }
    scene_.set(options_, "background", scene::Value::colours({ given }));
}

void SceneClient::sampleFilter(const v3d::rib::requests::ShadingNode & request, const v3d::rib::ParameterList & parameters) {
    if (request.type != "PxrCryptomatte") {
        warnings_.once("sample filter " + request.type, "the sample filter " + request.type + " is not one moya reads, so it changes nothing");
        return;
    }
    static const std::map<std::string, std::pair<std::string, std::string>> kLayers { { "Name", { "CryptoObject", "crypto_object" } },
        { "object", { "CryptoObject", "crypto_object" } }, { "Material", { "CryptoMaterial", "crypto_material" } },
        { "material", { "CryptoMaterial", "crypto_material" } }, { "Path", { "CryptoAsset", "crypto_asset" } }, { "asset", { "CryptoAsset", "crypto_asset" } } };
    const auto layer = kLayers.find(parameters.has("layer") ? parameters.string("layer") : std::string("Name"));
    if (layer == kLayers.end()) {
        warnings_.once("cryptomatte layer", "a PxrCryptomatte layer other than Name, Material or Path makes no output");
        return;
    }
    const std::string file = parameters.has("filename") ? parameters.string("filename") : std::string("cryptomatte.exr");
    const scene::Handle made = output(file);
    scene_.set(made, "format", scene::Value::string("exr"));
    scene_.set(made, "precision", scene::Value::string("float"));
    const scene::Handle channel = channelOf(layer->second.first, layer->second.second, 0);
    channels_[layer->second.first] = channel;
    scene_.set(made, "channels", scene::Value::handles({ channel }));
}

void SceneClient::option(const std::string & name, const v3d::rib::ParameterList & parameters) {
    static const std::map<std::string, std::set<std::string>> kRead { { "searchpath", { "texture", "shader" } },
        { "moya", { "denoise", "deepTolerance" } }, { "limits", { "texturememory", "bucketsize", "gridsize" } }, { "trace", { "maxdepth" } } };
    const auto read = kRead.find(name);
    unread("Option", name, parameters, read == kRead.end() ? std::set<std::string> {} : read->second);

    if (name == "trace" && parameters.has("maxdepth")) {
        const float depth = parameters.number("maxdepth", 0.0F);
        if (!inIntegers(depth, 0.0F)) {
            warnings_.once("maxdepth", "Option \"trace\" \"maxdepth\" has to be from 0 to the most an integer holds, so it changes nothing");
        } else {
            // checked: within an integer's range, as the test above holds it
            scene_.set(options_, "maxDepth", scene::Value::integer(static_cast<std::int32_t>(depth)));
        }
    }
    if (name == "searchpath" && parameters.has("texture")) {
        textureSearchPath_ = searchPath(parameters.string("texture"), textureSearchPath_);
        scene_.set(options_, "textureSearchPath", scene::Value::string(textureSearchPath_));
    }
    if (name == "searchpath" && parameters.has("shader")) {
        shaderSearchPath_ = searchPath(parameters.string("shader"), shaderSearchPath_);
        scene_.set(options_, "shaderSearchPath", scene::Value::string(shaderSearchPath_));
    }
    if (name == "moya" && parameters.has("denoise")) {
        scene_.set(options_, "denoise", scene::Value::integer(parameters.number("denoise", 0.0F) != 0.0F ? 1 : 0));
    }
    if (name == "moya" && parameters.has("deepTolerance")) {
        scene_.set(options_, "deepTolerance", scene::Value::number(parameters.number("deepTolerance", 0.0F)));
    }
    if (name == "limits" && parameters.has("texturememory")) {
        const float kilobytes = parameters.number("texturememory", 0.0F);
        if (!inIntegers(kilobytes, 1.0F)) {
            warnings_.once("texturememory", "Option \"limits\" \"texturememory\" has to be from 1 to the most an integer holds, so it changes nothing");
            return;
        }
        // checked: within an integer's range, as the test above holds it
        scene_.set(options_, "textureMemory", scene::Value::integer(static_cast<std::int32_t>(kilobytes)));
    }
}

void SceneClient::makeTexture(const v3d::rib::requests::MakeTexture & request, const v3d::rib::ParameterList & parameters) {
    texture::Making making;
    for (const auto & [given, wrap] : { std::pair { &request.swrap, &making.swrap }, std::pair { &request.twrap, &making.twrap } }) {
        if (!texture::wrapNamed(*given, wrap)) {
            logger_->get()->warn("MakeTexture's wrap mode {} is not one RI names, so {} is made with black", *given, request.texture);
        }
    }
    if (!v3d::rib::filterNamed(request.filter, &making.filter)) {
        logger_->get()->warn("MakeTexture's filter {} is not one RI names, so {} is made with the box", request.filter, request.texture);
    }
    making.width = glm::vec2(request.swidth, request.twidth);
    if (parameters.has("colorspace")) {
        making.colourSpace = parameters.string("colorspace");
    }
    try {
        if (!colours_) {
            colours_.emplace();
        }
        texture::make(request.picture, request.texture, making, *colours_, logger_);
    } catch (const std::exception & error) {
        logger_->get()->error("the texture {} cannot be made from {}: {}", request.texture, request.picture, error.what());
    }
}

void SceneClient::frameBegin(int frame) {
    scene_.set(options_, "frame", scene::Value::integer(frame));
}

void SceneClient::worldBegin() {
    // the transformation now takes world space to camera space, and the camera holds the other way
    internal::Sampled camera = current_.transformation;
    for (glm::mat4 & matrix : camera.matrices) {
        matrix = glm::inverse(matrix);
    }
    camera.set(&scene_, camera_, "transform", logger_);
    current_.transformation = internal::Sampled();

    // a new world replaces everything the world before it made
    made_.clear();
    lights_.clear();
    objects_.clear();
    spaces_.clear();
    spaces_["camera"] = camera;
    current_.lights.clear();
    current_.material = boost::make_shared<scene::Handle>();
    worldAttributes_ = attributes_.size();
    world_ = true;
}

void SceneClient::worldEnd() {
    // a block the world opened ends with it, so no object of this world reaches the next
    if (attributes_.size() > worldAttributes_) {
        warnings_.once("open at WorldEnd", "an AttributeBegin or ObjectBegin left open at WorldEnd ends there");
        current_ = attributes_[worldAttributes_];
        attributes_.erase(attributes_.begin() + static_cast<std::ptrdiff_t>(worldAttributes_), attributes_.end());
    }
    lights_.finish();
    world_ = false;
    scene_.commit();
    if (committed_) {
        committed_();
    }
}

void SceneClient::attributeBegin() {
    attributes_.push_back(current_);
}

void SceneClient::attributeEnd() {
    if (attributes_.empty()) {
        return;
    }
    current_ = attributes_.back();
    attributes_.pop_back();
}

void SceneClient::transformBegin() {
    transformations_.push_back(current_.transformation);
}

void SceneClient::transformEnd() {
    if (transformations_.empty()) {
        return;
    }
    current_.transformation = transformations_.back();
    transformations_.pop_back();
}

void SceneClient::concat(const glm::mat4 & matrix) {
    if (motion_) {
        // each request builds on the transformation the block opened with, at its own time
        const std::size_t at = poses_.size();
        const bool same = base_.times == times_;
        poses_.push_back((same && at < base_.matrices.size() ? base_.matrices[at] : base_.matrices.front()) * matrix);
        return;
    }
    for (glm::mat4 & sample : current_.transformation.matrices) {
        sample = sample * matrix;
    }
}

void SceneClient::replace(const glm::mat4 & matrix) {
    if (motion_) {
        poses_.push_back(matrix);
        return;
    }
    current_.transformation = internal::Sampled { .times = {}, .matrices = { matrix } };
}

void SceneClient::identity() {
    replace(glm::mat4(1.0F));
}

void SceneClient::transform(const glm::mat4x4 & matrix) {  // NOLINT(build/include_what_you_use)
    replace(matrix);
}

void SceneClient::concatTransform(const glm::mat4x4 & matrix) {
    concat(matrix);
}

void SceneClient::translate(float dx, float dy, float dz) {
    concat(glm::translate(glm::mat4(1.0F), glm::vec3(dx, dy, dz)));
}

// RiRotate takes its angle in degrees and glm takes radians
void SceneClient::rotate(float angle, const glm::vec3 & axis) {
    concat(glm::rotate(glm::mat4(1.0F), glm::radians(angle), axis));
}

void SceneClient::scale(float sx, float sy, float sz) {
    concat(glm::scale(glm::mat4(1.0F), glm::vec3(sx, sy, sz)));
}

void SceneClient::motionBegin(const std::vector<float> & times) {
    motion_ = true;
    times_ = times;
    base_ = current_.transformation;
    poses_.clear();
    deformations_.clear();
}

void SceneClient::motionEnd() {
    if (!motion_) {
        return;
    }
    motion_ = false;

    // a deforming primitive's poses become samples of its prototype, one at each of the block's
    // times
    deformations_.end(times_);

    // a block that held one request, or named one time, moves nothing
    const std::size_t count = std::min(poses_.size(), times_.size());
    if (count == 1 || (count == 0 && !poses_.empty())) {
        current_.transformation = internal::Sampled { .times = {}, .matrices = { poses_.front() } };
    } else if (count > 1) {
        current_.transformation = internal::Sampled { .times = std::vector<float>(times_.begin(), times_.begin() + static_cast<std::ptrdiff_t>(count)),
            .matrices = std::vector<glm::mat4>(poses_.begin(), poses_.begin() + static_cast<std::ptrdiff_t>(count)) };
    }
}

void SceneClient::attribute(const std::string & name, const v3d::rib::ParameterList & parameters) {
    static const std::map<std::string, std::set<std::string>> kRead { { "identifier", { "name" } },
        { "displacementbound", { "sphere", "coordinatesystem" } }, { "trace", { "bias" } }, { "light", { "nsamples" } },
        { "shade", { "intersectpriority" } } };
    const auto read = kRead.find(name);
    unread("Attribute", name, parameters, read == kRead.end() ? std::set<std::string> {} : read->second);

    if (name == "identifier" && parameters.has("name")) {
        current_.name = parameters.string("name");
    }
    if (name == "displacementbound" && parameters.has("sphere")) {
        displacementBound(parameters);
    }
    if (name == "shade" && parameters.has("intersectpriority")) {
        const float given = parameters.number("intersectpriority", 0.0F);
        const std::optional<std::int32_t> priority = v3d::type::toInteger(given, {.minimum = -2000000000, .maximum = 2000000000});
        if (!priority) {
            warnings_.once("priority " + std::to_string(given), "an intersection priority of " + std::to_string(given) + " is beyond what an integer holds, and is not used");
        } else {
            current_.priority = *priority;
        }
    }
}

void SceneClient::displacementBound(const v3d::rib::ParameterList & parameters) {
    const float radius = parameters.number("sphere", 0.0F);
    if (!std::isfinite(radius) || radius < 0.0F) {
        warnings_.once("bound " + std::to_string(radius), "a displacement bound of " + std::to_string(radius) + " is not a distance, and is not used");
        return;
    }
    // the bound is stated in object space or the space it names, and a subdivision surface holds
    // it in object space. A scale that differs between axes is taken at its largest, so the
    // bound still holds the surface
    const std::vector<std::string> & named = parameters.strings("coordinatesystem");
    const std::string space = named.empty() ? std::string("object") : named.front();
    glm::mat4 toWorld = current_.transformation.matrices.front();
    if (space == "world") {
        toWorld = glm::mat4(1.0F);
    } else if (space != "object") {
        const auto found = spaces_.find(space);
        if (found == spaces_.end()) {
            warnings_.once("bound space " + space, "the displacement bound's space \"" + space + "\" was not named, so object space is used");
        } else {
            toWorld = found->second.matrices.front();
        }
    }
    const glm::mat3 linear(glm::inverse(current_.transformation.matrices.front()) * toWorld);
    current_.displacementBound = radius * std::max({ glm::length(linear[0]), glm::length(linear[1]), glm::length(linear[2]) });
}

std::string SceneClient::nameFor(const std::string & kind, const std::string & request) {
    return made_.name(kind, request, current_.name);
}

bool SceneClient::inWorld() {
    if (!world_) {
        warnings_.once("outside", "a primitive outside WorldBegin and WorldEnd is not drawn");
    }
    return world_;
}

void SceneClient::place(const internal::Shape & shape) {
    if (!inWorld()) {
        return;
    }
    const scene::Handle prototype = made_.make(shape.kind, nameFor(shape.kind, shape.request));
    for (const auto & [name, value] : shape.parameters) {
        scene_.set(prototype, name, value);
    }
    if (shape.kind == "subdivision") {
        scene_.set(prototype, "displacementBound", scene::Value::number(current_.displacementBound));
    }

    // the request's other parameters are primvars, unless the kind holds one of the name
    std::set<std::string> except = shape.taken;
    const scene::Kind & kind = scene_.schema().kind(prototype.kind);
    for (const scene::Declaration & declared : kind.parameters) {
        except.insert(declared.name);
    }
    internal::declare(&scene_, prototype, shape.primvars, except, false);

    // the first pose of a deforming primitive, whose later poses add samples to it
    if (motion_) {
        deformations_.begin(prototype, shape);
    }

    const bool inObject = current_.group.valid();
    instance(prototype, nameFor("instance", shape.request), !inObject);

    if (current_.area && !inObject) {
        lights_.glow(prototype, current_.area->light, current_.area->name, current_.area->parameters, current_.transformation);
    }
}

scene::Handle SceneClient::instance(const scene::Handle & prototype, const std::string & name, bool lit) {
    const scene::Handle placed = made_.make("instance", name);
    scene_.set(placed, "prototype", scene::Value::handle(prototype));
    current_.transformation.set(&scene_, placed, "transform", logger_);
    scene_.set(placed, "doubleSided", scene::Value::integer(current_.twoSided ? 1 : 0));
    if (current_.priority != 0) {
        scene_.set(placed, "priority", scene::Value::integer(current_.priority));
    }
    if (current_.group.valid()) {
        // an object's geometry takes the material in force where the object is placed
        scene_.set(placed, "group", scene::Value::handle(current_.group));
    } else {
        scene_.set(placed, "material", scene::Value::handle(material()));
    }
    if (lit) {
        lights_.lit(placed, current_.lights);
    }
    return placed;
}

scene::Handle SceneClient::shaderNode(const Shader & shader, const std::string & name) {
    const scene::Handle node = made_.make("shader", name);
    scene_.set(node, "shader", scene::Value::string(shader.name));
    scene_.set(node, "language", scene::Value::string("ri"));
    internal::declare(&scene_, node, shader.parameters, { "shader", "language" }, true);
    return node;
}

scene::Handle SceneClient::material() {
    if (current_.material->valid()) {
        return *current_.material;
    }
    const bool networked = std::ranges::any_of(current_.network, [](const internal::Network::Node & node) {
        return node.bxdf;
    });
    std::optional<internal::Terminals> networkMade;
    if (networked) {
        internal::Network network(&scene_, &made_, &warnings_, current_.network);
        networkMade = network.terminals(current_.surface.name);
    }
    if (networkMade) {
        const scene::Handle made = made_.make("material", made_.unique("material", "material"));
        if (networkMade->surface) {
            scene_.set(made, "surface", scene::Value::handle(*networkMade->surface));
        }
        if (networkMade->displacement) {
            scene_.set(made, "displacement", scene::Value::handle(*networkMade->displacement));
        } else if (!current_.displacement.name.empty()) {
            scene_.set(made, "displacement", scene::Value::handle(shaderNode(current_.displacement, made_.unique("shader", current_.displacement.name))));
        }
        if (networkMade->volume) {
            scene_.set(made, "volume", scene::Value::handle(*networkMade->volume));
        }
        *current_.material = made;
        return made;
    }
    // a material and its nodes are named after the surface shader, whatever geometry made them
    const scene::Handle surface = shaderNode(current_.surface, made_.unique("shader", current_.surface.name));
    for (const auto & [input, value] : { std::pair { "Cs", current_.colour }, std::pair { "Os", current_.opacity } }) {
        if (!current_.surface.parameters.has(input)) {
            scene_.declare(surface, scene::Declaration { .name = input, .type = scene::Type::Colour, .fallback = scene::Value::colour(value) });
        }
    }
    const scene::Handle made = made_.make("material", made_.unique("material", current_.surface.name));
    scene_.set(made, "surface", scene::Value::handle(surface));
    if (!current_.displacement.name.empty()) {
        const scene::Handle displacement = shaderNode(current_.displacement, made_.unique("shader", current_.displacement.name));
        scene_.set(made, "displacement", scene::Value::handle(displacement));
    }
    *current_.material = made;
    return made;
}

void SceneClient::coordinateSystem(const std::string & space) {
    if (!space.empty()) {
        spaces_[space] = current_.transformation;
    }
}

void SceneClient::coordSysTransform(const std::string & space) {
    if (space == "world") {
        current_.transformation = internal::Sampled();
        return;
    }
    const auto found = spaces_.find(space);
    if (found == spaces_.end()) {
        warnings_.once("space " + space, "the space \"" + space + "\" was not named, so CoordSysTransform changes nothing");
        return;
    }
    current_.transformation = found->second;
}

void SceneClient::color(const glm::vec3 & value) {
    current_.colour = value;
    current_.material = boost::make_shared<scene::Handle>();
}

void SceneClient::opacity(const glm::vec3 & value) {
    current_.opacity = value;
    current_.material = boost::make_shared<scene::Handle>();
}

void SceneClient::sides(int count) {
    current_.twoSided = count == 2;
}

void SceneClient::basis(const v3d::rib::requests::Basis & request) {
    std::string name = v3d::rib::basisName(request.v);
    if (name.empty()) {
        warnings_.once("basis", "moya's curves take a basis RI names, so a basis of its own numbers is taken as bezier");
        name = "bezier";
    }
    current_.basis = name;
}

void SceneClient::surface(const std::string & name, const v3d::rib::ParameterList & parameters) {
    if (name.empty()) {
        return;
    }
    current_.surface = Shader { .name = name, .parameters = parameters };
    current_.network.clear();
    current_.material = boost::make_shared<scene::Handle>();
}

void SceneClient::pattern(const v3d::rib::requests::ShadingNode & request, const v3d::rib::ParameterList & parameters) {
    if (request.type.empty()) {
        return;
    }
    current_.network.push_back(internal::Network::Node { .request = request, .parameters = parameters, .bxdf = false });
    current_.material = boost::make_shared<scene::Handle>();
}

void SceneClient::bxdf(const v3d::rib::requests::ShadingNode & request, const v3d::rib::ParameterList & parameters) {
    if (request.type.empty()) {
        return;
    }
    current_.network.push_back(internal::Network::Node { .request = request, .parameters = parameters, .bxdf = true });
    current_.material = boost::make_shared<scene::Handle>();
}

void SceneClient::displacement(const std::string & name, const v3d::rib::ParameterList & parameters) {
    if (name.empty()) {
        return;
    }
    current_.displacement = Shader { .name = name, .parameters = parameters };
    current_.material = boost::make_shared<scene::Handle>();
}

void SceneClient::lightSource(const v3d::rib::requests::LightSource & request, const v3d::rib::ParameterList & parameters) {
    if (request.shader.empty()) {
        return;
    }
    if (!world_) {
        warnings_.once("light outside", "a light outside WorldBegin and WorldEnd makes no light");
        return;
    }
    const std::string name = current_.name.empty() ? "light " + request.handle : current_.name;
    if (lights_.source(request, parameters, name, current_.transformation)) {
        current_.lights.insert(request.handle);
    }
}

void SceneClient::areaLightSource(const v3d::rib::requests::LightSource & request, const v3d::rib::ParameterList & parameters) {
    if (request.shader.empty()) {
        current_.area.reset();
        return;
    }
    if (request.shader != "arealight") {
        // another area light is a shader written for RI, which moya does not run
        warnings_.once("area light " + request.shader, "moya runs no area light shader \"" + request.shader + "\", so it changes nothing");
        current_.area.reset();
        return;
    }
    current_.area = Shader { .name = current_.name.empty() ? "light " + request.handle : current_.name, .parameters = parameters, .light = request.shader };
}

void SceneClient::light(const v3d::rib::requests::LightSource & request, const v3d::rib::ParameterList & parameters) {
    if (request.shader.empty()) {
        return;
    }
    if (!world_) {
        warnings_.once("light outside", "a light outside WorldBegin and WorldEnd makes no light");
        return;
    }
    const std::string name = current_.name.empty() ? "light " + request.handle : current_.name;
    if (request.shader == "PxrMeshLight") {
        current_.area = Shader { .name = name, .parameters = parameters, .light = request.shader };
        return;
    }
    if (lights_.renderman(request, parameters, name, current_.transformation)) {
        current_.lights.insert(request.handle);
    }
}

void SceneClient::illuminate(const std::string & handle, bool on) {
    if (!lights_.named(handle)) {
        return;
    }
    if (on) {
        current_.lights.insert(handle);
    } else {
        current_.lights.erase(handle);
    }
}

v3d::rib::Handler* SceneClient::deformation() {
    return &deformations_;
}

void SceneClient::polygon(unsigned int vertices, const v3d::rib::ParameterList & parameters) {
    place(internal::Shapes::polygon(vertices, parameters));
}

void SceneClient::pointsPolygons(const v3d::rib::requests::PointsPolygons & request, const v3d::rib::ParameterList & parameters) {
    place(internal::Shapes::mesh(request, parameters));
}

void SceneClient::sphere(const v3d::rib::requests::Sphere & request, const v3d::rib::ParameterList & parameters) {
    place(internal::Shapes::sphere(request, parameters));
}

void SceneClient::curves(const v3d::rib::requests::Curves & request, const v3d::rib::ParameterList & parameters) {
    place(internal::Shapes::curves(request, parameters, current_.basis));
}

void SceneClient::points(unsigned int count, const v3d::rib::ParameterList & parameters) {
    (void)count;
    place(internal::Shapes::points(parameters));
}

void SceneClient::cone(const v3d::rib::requests::Cone & request, const v3d::rib::ParameterList & parameters) {
    place(internal::Shapes::cone(request, parameters));
}

void SceneClient::cylinder(const v3d::rib::requests::Cylinder & request, const v3d::rib::ParameterList & parameters) {
    place(internal::Shapes::cylinder(request, parameters));
}

void SceneClient::hyperboloid(const v3d::rib::requests::Hyperboloid & request, const v3d::rib::ParameterList & parameters) {
    place(internal::Shapes::hyperboloid(request, parameters));
}

void SceneClient::paraboloid(const v3d::rib::requests::Paraboloid & request, const v3d::rib::ParameterList & parameters) {
    place(internal::Shapes::paraboloid(request, parameters));
}

void SceneClient::disk(const v3d::rib::requests::Disk & request, const v3d::rib::ParameterList & parameters) {
    place(internal::Shapes::disk(request, parameters));
}

void SceneClient::torus(const v3d::rib::requests::Torus & request, const v3d::rib::ParameterList & parameters) {
    place(internal::Shapes::torus(request, parameters));
}

void SceneClient::subdivisionMesh(const v3d::rib::requests::SubdivisionMesh & request, const v3d::rib::ParameterList & parameters) {
    place(internal::Shapes::subdivision(request, parameters, &warnings_));
}

void SceneClient::volume(const v3d::rib::requests::Volume & request, const v3d::rib::ParameterList & parameters) {
    const std::optional<internal::Grids> grids = internal::openVdbGrids(request, parameters, &warnings_);
    if (!grids || !inWorld()) {
        return;
    }
    std::vector<scene::Handle> fields;
    for (const std::string & name : grids->names) {
        const scene::Handle field = made_.make("openVDBAsset", made_.unique("openVDBAsset", name));
        scene_.set(field, "filePath", scene::Value::string(grids->file));
        scene_.set(field, "fieldName", scene::Value::string(name));
        fields.push_back(field);
    }
    place(internal::Shape { .kind = "volume", .request = "Volume",
        .parameters = { { "fields", scene::Value::handles(fields) }, { "fieldNames", scene::Value::strings(grids->names) } } });
}

void SceneClient::objectBegin(const std::string & handle) {
    if (!world_) {
        warnings_.once("object outside", "an object outside WorldBegin and WorldEnd is not kept");
    }
    attributes_.push_back(current_);
    const scene::Handle group = made_.make("group", made_.unique("group", current_.name.empty() ? "object " + handle : current_.name));
    objects_[handle] = group;
    current_.transformation = internal::Sampled();
    current_.group = group;
}

void SceneClient::objectEnd() {
    if (attributes_.empty() || !current_.group.valid()) {
        return;
    }
    current_ = attributes_.back();
    attributes_.pop_back();
}

void SceneClient::objectInstance(const std::string & handle) {
    const auto found = objects_.find(handle);
    if (found == objects_.end()) {
        warnings_.once("object " + handle, "ObjectInstance names the object " + handle + ", which no ObjectBegin made");
        return;
    }
    instance(found->second, nameFor("instance", "object " + handle), !current_.group.valid());
}

};  // namespace v3d::moya::ri
