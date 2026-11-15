/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include "SceneClient.h"

#include <api/log/Logger.h>
#include <api/render/offline/film/Sampling.h>
#include <api/render/offline/rib/Bases.h>
#include <api/render/offline/rib/Handler.h>
#include <api/render/offline/rib/Parameters.h>
#include <api/render/offline/rib/requests/Basis.h>
#include <api/render/offline/rib/requests/Clipping.h>
#include <api/render/offline/rib/requests/Cone.h>
#include <api/render/offline/rib/requests/CropWindow.h>
#include <api/render/offline/rib/requests/Curves.h>
#include <api/render/offline/rib/requests/Cylinder.h>
#include <api/render/offline/rib/requests/DepthOfField.h>
#include <api/render/offline/rib/requests/Disk.h>
#include <api/render/offline/rib/requests/Display.h>
#include <api/render/offline/rib/requests/DisplayChannel.h>
#include <api/render/offline/rib/requests/Format.h>
#include <api/render/offline/rib/requests/Hyperboloid.h>
#include <api/render/offline/rib/requests/LightSource.h>
#include <api/render/offline/rib/requests/MakeTexture.h>
#include <api/render/offline/rib/requests/Paraboloid.h>
#include <api/render/offline/rib/requests/PointsPolygons.h>
#include <api/render/offline/rib/requests/ScreenWindow.h>
#include <api/render/offline/rib/requests/ShadingNode.h>
#include <api/render/offline/rib/requests/Shutter.h>
#include <api/render/offline/rib/requests/Sphere.h>
#include <api/render/offline/rib/requests/SubdivisionMesh.h>
#include <api/render/offline/rib/requests/Torus.h>
#include <moya/libmoya/ri/internal/Document.h>
#include <moya/libmoya/ri/internal/Primvars.h>
#include <moya/libmoya/ri/internal/Sampled.h>
#include <moya/libmoya/ri/internal/Shape.h>
#include <moya/libmoya/ri/internal/Shapes.h>
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

void SceneClient::unread(const std::string & request, const std::string & name, const v3d::render::offline::rib::ParameterList & parameters,
    const std::set<std::string> & read) {
    for (const std::string & parameter : parameters.names()) {
        if (read.contains(parameter)) {
            continue;
        }
        const std::string named = name.empty() ? request : request + " \"" + name + "\"";
        warnings_.once(named + " " + parameter, named + "'s \"" + parameter + "\" is not read, so it changes nothing");
    }
}

void SceneClient::hider(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) {
    if (name != "raytrace" && name != "hidden") {
        warnings_.once("hider " + name, "moya renders by path tracing alone, so Hider \"" + name + "\" changes nothing");
    }
    unread("Hider", name, parameters, {});
}

void SceneClient::frame(const std::function<void()> & committed) {
    committed_ = committed;
}

void SceneClient::format(const v3d::render::offline::rib::requests::Format & request) {
    const int width = request.width == 0 ? kWidth : static_cast<int>(request.width);
    const int height = request.height == 0 ? kHeight : static_cast<int>(request.height);
    scene_.set(options_, "resolution", scene::Value::integers({ width, height }));
    scene_.set(options_, "pixelAspect", scene::Value::number(request.pixelAspect));
}

void SceneClient::frameAspectRatio(float aspect) {
    scene_.set(options_, "frameAspect", scene::Value::number(aspect));
}

void SceneClient::screenWindow(const v3d::render::offline::rib::requests::ScreenWindow & request) {
    scene_.set(camera_, "screen", scene::Value::floats({ request.left, request.right, request.bottom, request.top }));
}

void SceneClient::cropWindow(const v3d::render::offline::rib::requests::CropWindow & request) {
    scene_.set(options_, "crop", scene::Value::floats({ request.xmin, request.xmax, request.ymin, request.ymax }));
}

void SceneClient::projection(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) {
    if (name != "perspective" && name != "orthographic") {
        logger_->get()->warn("moya's camera is perspective or orthographic, so Projection \"{}\" changes nothing", name);
        return;
    }
    scene_.set(camera_, "projection", scene::Value::string(name));
    if (parameters.has("fov")) {
        scene_.set(camera_, "fov", scene::Value::number(parameters.number("fov", 0.0F)));
    }
}

void SceneClient::clipping(const v3d::render::offline::rib::requests::Clipping & request) {
    scene_.set(camera_, "near", scene::Value::number(request.hither));
    scene_.set(camera_, "far", scene::Value::number(request.yon));
}

void SceneClient::depthOfField(const v3d::render::offline::rib::requests::DepthOfField & request) {
    scene_.set(camera_, "fstop", scene::Value::number(request.fstop));
    scene_.set(camera_, "focalLength", scene::Value::number(request.focalLength));
    scene_.set(camera_, "focalDistance", scene::Value::number(request.focalDistance));
}

void SceneClient::shutter(const v3d::render::offline::rib::requests::Shutter & request) {
    scene_.set(camera_, "shutter", scene::Value::floats({ request.open, request.close }));
}

void SceneClient::pixelSamples(const glm::uvec2 & samples) {
    scene_.set(options_, "samples", scene::Value::integer(static_cast<std::int32_t>(samples.x * samples.y)));
}

void SceneClient::pixelFilter(v3d::render::offline::film::Filter filter, float xwidth, float ywidth) {
    scene_.set(options_, "filter", scene::Value::string(v3d::render::offline::film::filterName(filter)));
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

void SceneClient::display(const v3d::render::offline::rib::requests::Display & request, const v3d::render::offline::rib::ParameterList & parameters) {
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

void SceneClient::displayChannel(const v3d::render::offline::rib::requests::DisplayChannel & request,
    const v3d::render::offline::rib::ParameterList & parameters) {
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

void SceneClient::sampleFilter(const v3d::render::offline::rib::requests::ShadingNode & request, const v3d::render::offline::rib::ParameterList & parameters) {
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

void SceneClient::option(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) {
    static const std::map<std::string, std::set<std::string>> kRead { { "searchpath", { "texture", "shader" } },
        { "moya", { "denoise", "deepTolerance" } }, { "limits", { "texturememory", "bucketsize", "gridsize" } }, { "trace", { "maxdepth" } } };
    const auto read = kRead.find(name);
    unread("Option", name, parameters, read == kRead.end() ? std::set<std::string> {} : read->second);

    if (name == "trace" && parameters.has("maxdepth")) {
        const float depth = parameters.number("maxdepth", 0.0F);
        if (!(depth >= 0.0F) || !(depth <= static_cast<float>(std::numeric_limits<std::int32_t>::max()))) {
            warnings_.once("maxdepth", "Option \"trace\" \"maxdepth\" has to be from 0 to the most an integer holds, so it changes nothing");
        } else {
            // checked: within an integer's range, as the test above holds it
            scene_.set(options_, "maxDepth", scene::Value::integer(static_cast<std::int32_t>(depth)));
        }
    }
    if (name == "searchpath" && parameters.has("texture")) {
        const std::string given = parameters.string("texture");
        std::string path;
        for (const char letter : given) {
            path += letter == '&' ? textureSearchPath_ : std::string(1, letter);
        }
        textureSearchPath_ = path;
        scene_.set(options_, "textureSearchPath", scene::Value::string(path));
    }
    if (name == "moya" && parameters.has("denoise")) {
        scene_.set(options_, "denoise", scene::Value::integer(parameters.number("denoise", 0.0F) != 0.0F ? 1 : 0));
    }
    if (name == "moya" && parameters.has("deepTolerance")) {
        scene_.set(options_, "deepTolerance", scene::Value::number(parameters.number("deepTolerance", 0.0F)));
    }
    if (name == "limits" && parameters.has("texturememory")) {
        const float kilobytes = parameters.number("texturememory", 0.0F);
        if (!(kilobytes >= 1.0F) || !(kilobytes <= static_cast<float>(std::numeric_limits<std::int32_t>::max()))) {
            warnings_.once("texturememory", "Option \"limits\" \"texturememory\" has to be from 1 to the most an integer holds, so it changes nothing");
            return;
        }
        // checked: within an integer's range, as the test above holds it
        scene_.set(options_, "textureMemory", scene::Value::integer(static_cast<std::int32_t>(kilobytes)));
    }
}

void SceneClient::makeTexture(const v3d::render::offline::rib::requests::MakeTexture & request, const v3d::render::offline::rib::ParameterList & parameters) {
    texture::Making making;
    for (const auto & [given, wrap] : { std::pair { &request.swrap, &making.swrap }, std::pair { &request.twrap, &making.twrap } }) {
        if (!texture::wrapNamed(*given, wrap)) {
            logger_->get()->warn("MakeTexture's wrap mode {} is not one RI names, so {} is made with black", *given, request.texture);
        }
    }
    if (!v3d::render::offline::film::filterNamed(request.filter, &making.filter)) {
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

void SceneClient::attribute(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) {
    static const std::map<std::string, std::set<std::string>> kRead { { "identifier", { "name" } },
        { "displacementbound", { "sphere", "coordinatesystem" } }, { "trace", { "bias" } }, { "light", { "nsamples" } } };
    const auto read = kRead.find(name);
    unread("Attribute", name, parameters, read == kRead.end() ? std::set<std::string> {} : read->second);

    if (name == "identifier" && parameters.has("name")) {
        current_.name = parameters.string("name");
    }
    if (name == "displacementbound" && parameters.has("sphere")) {
        displacementBound(parameters);
    }
}

void SceneClient::displacementBound(const v3d::render::offline::rib::ParameterList & parameters) {
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

void SceneClient::place(const internal::Shape & shape) {
    if (!world_) {
        warnings_.once("outside", "a primitive outside WorldBegin and WorldEnd is not drawn");
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

std::optional<internal::Terminals> SceneClient::networkTerminals() {
    const std::vector<Node> & network = current_.network;
    std::size_t last = network.size();
    for (std::size_t place = network.size(); place > 0; --place) {
        if (network[place - 1].bxdf) {
            last = place - 1;
            break;
        }
    }
    std::map<std::size_t, scene::Handle> made;
    const Node & surface = network[last];
    if (surface.request.type != "materialx") {
        return internal::Terminals { .surface = networkNode(last, made), .displacement = std::nullopt };
    }
    const std::optional<scene::Value> file = internal::valueOf(surface.parameters, "file");
    const std::optional<scene::Value> named = internal::valueOf(surface.parameters, "material");
    const std::string path = file && !file->stringArray().empty() ? file->stringArray().front() : std::string();
    const std::string chosen = named && !named->stringArray().empty() ? named->stringArray().front() : std::string();
    std::string reason;
    const std::optional<internal::Terminals> read = internal::documentMaterial(scene_, made_, path, chosen, reason);
    if (!read) {
        warnings_.once("document " + path, "the MaterialX document \"" + path + "\" gives no material, because " + reason + ", so the surface is " + current_.surface.name);
        return std::nullopt;
    }
    inputsOf(read->surface, surface.request.handle, surface, last, made, { "file", "material" });
    return read;
}

scene::Handle SceneClient::networkNode(std::size_t place, std::map<std::size_t, scene::Handle> & made) {
    const auto found = made.find(place);
    if (found != made.end()) {
        return found->second;
    }
    const Node & node = current_.network[place];
    const std::string name = made_.unique("shader", node.request.handle.empty() ? node.request.type : node.request.handle);
    const scene::Handle out = made_.make("shader", name);
    made.emplace(place, out);
    scene_.set(out, "shader", scene::Value::string(node.request.type));
    scene_.set(out, "language", scene::Value::string("materialx"));
    inputsOf(out, name, node, place, made, {});
    return out;
}

void SceneClient::inputsOf(const scene::Handle & node, const std::string & name, const Node & request, std::size_t place,
    std::map<std::size_t, scene::Handle> & made, const std::set<std::string> & except) {
    std::set<std::string> constants = except;
    for (const std::string & input : request.parameters.names()) {
        if (!request.parameters.declaration(input)->reference()) {
            continue;
        }
        constants.insert(input);
        const std::vector<std::string> & values = request.parameters.strings(input);
        const std::string value = values.empty() ? std::string() : values.front();
        const std::size_t colon = value.find(':');
        const std::string handle = value.substr(0, colon);
        const std::string output = colon == std::string::npos ? std::string() : value.substr(colon + 1);
        // a reference reads a node that came before it
        std::size_t from = place;
        for (std::size_t earlier = place; earlier > 0; --earlier) {
            if (current_.network[earlier - 1].request.handle == handle) {
                from = earlier - 1;
                break;
            }
        }
        if (from == place) {
            std::string message = "a reference names ";
            message.append(handle).append(", which no Pattern or Bxdf before it has as its handle, so ").append(input).append(" keeps its value");
            warnings_.once("reference " + handle, message);
            continue;
        }
        const scene::Handle source = networkNode(from, made);
        std::string named = handle;
        named.append(" to ").append(input).append(" of ").append(name);
        const scene::Handle connection = made_.make("connection", made_.unique("connection", named));
        scene_.set(connection, "from", scene::Value::handle(source));
        scene_.set(connection, "to", scene::Value::handle(node));
        scene_.set(connection, "input", scene::Value::string(input));
        if (!output.empty()) {
            scene_.set(connection, "output", scene::Value::string(output));
        }
    }
    internal::declare(&scene_, node, request.parameters, constants, true);
}

scene::Handle SceneClient::material() {
    if (current_.material->valid()) {
        return *current_.material;
    }
    const bool networked = std::ranges::any_of(current_.network, [](const Node & node) {
        return node.bxdf;
    });
    const std::optional<internal::Terminals> networkMade = networked ? networkTerminals() : std::nullopt;
    if (networkMade) {
        const scene::Handle made = made_.make("material", made_.unique("material", "material"));
        scene_.set(made, "surface", scene::Value::handle(networkMade->surface));
        if (networkMade->displacement) {
            scene_.set(made, "displacement", scene::Value::handle(*networkMade->displacement));
        } else if (!current_.displacement.name.empty()) {
            scene_.set(made, "displacement", scene::Value::handle(shaderNode(current_.displacement, made_.unique("shader", current_.displacement.name))));
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

void SceneClient::basis(const v3d::render::offline::rib::requests::Basis & request) {
    std::string name = v3d::render::offline::rib::basisName(request.v);
    if (name.empty()) {
        warnings_.once("basis", "moya's curves take a basis RI names, so a basis of its own numbers is taken as bezier");
        name = "bezier";
    }
    current_.basis = name;
}

void SceneClient::surface(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) {
    if (name.empty()) {
        return;
    }
    current_.surface = Shader { .name = name, .parameters = parameters };
    current_.network.clear();
    current_.material = boost::make_shared<scene::Handle>();
}

void SceneClient::pattern(const v3d::render::offline::rib::requests::ShadingNode & request, const v3d::render::offline::rib::ParameterList & parameters) {
    if (request.type.empty()) {
        return;
    }
    current_.network.push_back(Node { .request = request, .parameters = parameters, .bxdf = false });
    current_.material = boost::make_shared<scene::Handle>();
}

void SceneClient::bxdf(const v3d::render::offline::rib::requests::ShadingNode & request, const v3d::render::offline::rib::ParameterList & parameters) {
    if (request.type.empty()) {
        return;
    }
    current_.network.push_back(Node { .request = request, .parameters = parameters, .bxdf = true });
    current_.material = boost::make_shared<scene::Handle>();
}

void SceneClient::displacement(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) {
    if (name.empty()) {
        return;
    }
    current_.displacement = Shader { .name = name, .parameters = parameters };
    current_.material = boost::make_shared<scene::Handle>();
}

void SceneClient::lightSource(const v3d::render::offline::rib::requests::LightSource & request, const v3d::render::offline::rib::ParameterList & parameters) {
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

void SceneClient::areaLightSource(const v3d::render::offline::rib::requests::LightSource & request, const v3d::render::offline::rib::ParameterList & parameters) {
    if (request.shader.empty()) {
        current_.area.reset();
        return;
    }
    current_.area = Shader { .name = current_.name.empty() ? "light " + request.handle : current_.name, .parameters = parameters, .light = request.shader };
}

void SceneClient::light(const v3d::render::offline::rib::requests::LightSource & request, const v3d::render::offline::rib::ParameterList & parameters) {
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

v3d::render::offline::rib::Handler* SceneClient::deformation() {
    return &deformations_;
}

void SceneClient::polygon(unsigned int vertices, const v3d::render::offline::rib::ParameterList & parameters) {
    place(internal::Shapes::polygon(vertices, parameters));
}

void SceneClient::pointsPolygons(const v3d::render::offline::rib::requests::PointsPolygons & request, const v3d::render::offline::rib::ParameterList & parameters) {
    place(internal::Shapes::mesh(request, parameters));
}

void SceneClient::sphere(const v3d::render::offline::rib::requests::Sphere & request, const v3d::render::offline::rib::ParameterList & parameters) {
    place(internal::Shapes::sphere(request, parameters));
}

void SceneClient::curves(const v3d::render::offline::rib::requests::Curves & request, const v3d::render::offline::rib::ParameterList & parameters) {
    place(internal::Shapes::curves(request, parameters, current_.basis));
}

void SceneClient::points(unsigned int count, const v3d::render::offline::rib::ParameterList & parameters) {
    (void)count;
    place(internal::Shapes::points(parameters));
}

void SceneClient::cone(const v3d::render::offline::rib::requests::Cone & request, const v3d::render::offline::rib::ParameterList & parameters) {
    place(internal::Shapes::cone(request, parameters));
}

void SceneClient::cylinder(const v3d::render::offline::rib::requests::Cylinder & request, const v3d::render::offline::rib::ParameterList & parameters) {
    place(internal::Shapes::cylinder(request, parameters));
}

void SceneClient::hyperboloid(const v3d::render::offline::rib::requests::Hyperboloid & request, const v3d::render::offline::rib::ParameterList & parameters) {
    place(internal::Shapes::hyperboloid(request, parameters));
}

void SceneClient::paraboloid(const v3d::render::offline::rib::requests::Paraboloid & request, const v3d::render::offline::rib::ParameterList & parameters) {
    place(internal::Shapes::paraboloid(request, parameters));
}

void SceneClient::disk(const v3d::render::offline::rib::requests::Disk & request, const v3d::render::offline::rib::ParameterList & parameters) {
    place(internal::Shapes::disk(request, parameters));
}

void SceneClient::torus(const v3d::render::offline::rib::requests::Torus & request, const v3d::render::offline::rib::ParameterList & parameters) {
    place(internal::Shapes::torus(request, parameters));
}

void SceneClient::subdivisionMesh(const v3d::render::offline::rib::requests::SubdivisionMesh & request, const v3d::render::offline::rib::ParameterList & parameters) {
    place(internal::Shapes::subdivision(request, parameters, &warnings_));
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
