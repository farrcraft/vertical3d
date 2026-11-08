/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include "SceneClient.h"

#include <api/log/Logger.h>
#include <api/render/offline/film/Sampling.h>
#include <api/render/offline/rib/Parameters.h>
#include <api/render/offline/rib/Requests.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::ri {

namespace {

/** RI's default picture, which a side of zero in a Format asks for. **/
constexpr int kWidth = 320;
constexpr int kHeight = 240;

/** Whether a file name ends with a suffix, compared exactly. **/
bool endsWith(const std::string & text, const std::string & suffix) {
    return text.size() >= suffix.size() && text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

};  // namespace

SceneClient::SceneClient(scene::Scene & scene, const boost::shared_ptr<v3d::log::Logger> & logger) :
    scene_(scene), logger_(logger) {
    options_ = scene_.create("options", "options");
    camera_ = scene_.create("camera", "camera");
    scene_.set(options_, "camera", scene::Value::handle(camera_));
}

void SceneClient::frame(const std::function<void()> & committed) {
    committed_ = committed;
}

void SceneClient::format(const v3d::render::offline::rib::Format & request) {
    const int width = request.width == 0 ? kWidth : static_cast<int>(request.width);
    const int height = request.height == 0 ? kHeight : static_cast<int>(request.height);
    scene_.set(options_, "resolution", scene::Value::integers({ width, height }));
    scene_.set(options_, "pixelAspect", scene::Value::number(request.pixelAspect));
}

void SceneClient::frameAspectRatio(float aspect) {
    scene_.set(options_, "frameAspect", scene::Value::number(aspect));
}

void SceneClient::screenWindow(const v3d::render::offline::rib::ScreenWindow & request) {
    scene_.set(camera_, "screen", scene::Value::floats({ request.left, request.right, request.bottom, request.top }));
}

void SceneClient::cropWindow(const v3d::render::offline::rib::CropWindow & request) {
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

void SceneClient::clipping(const v3d::render::offline::rib::Clipping & request) {
    scene_.set(camera_, "near", scene::Value::number(request.hither));
    scene_.set(camera_, "far", scene::Value::number(request.yon));
}

void SceneClient::depthOfField(const v3d::render::offline::rib::DepthOfField & request) {
    scene_.set(camera_, "fstop", scene::Value::number(request.fstop));
    scene_.set(camera_, "focalLength", scene::Value::number(request.focalLength));
    scene_.set(camera_, "focalDistance", scene::Value::number(request.focalDistance));
}

void SceneClient::shutter(const v3d::render::offline::rib::Shutter & request) {
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
    scene_.set(options_, "variance", scene::Value::number(variation));
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

void SceneClient::display(const v3d::render::offline::rib::Display & request, const v3d::render::offline::rib::ParameterList & parameters) {
    (void)parameters;
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
    }
    scene_.set(made, "format", scene::Value::string(format));

    std::vector<scene::Handle> channels;
    if (request.mode != "rgb" && request.mode != "rgba" && !request.mode.empty()) {
        const auto held = channels_.find(request.mode);
        if (held == channels_.end()) {
            const scene::Handle channel = scene_.create("channel", request.mode);
            scene_.set(channel, "source", scene::Value::string(request.mode));
            channels_[request.mode] = channel;
        }
        channels.push_back(channels_[request.mode]);
    }
    scene_.set(made, "channels", scene::Value::handles(channels));
}

void SceneClient::frameBegin(int frame) {
    scene_.set(options_, "frame", scene::Value::integer(frame));
}

void SceneClient::worldBegin() {
    // the transformation now takes world space to camera space, and the camera holds the other way
    Sampled camera = current_.transformation;
    for (glm::mat4 & matrix : camera.matrices) {
        matrix = glm::inverse(matrix);
    }
    setSampled(camera_, "transform", camera);
    current_.transformation = Sampled();

    // a new world replaces everything the world before it made
    for (const scene::Handle & made : made_) {
        scene_.remove(made);
    }
    made_.clear();
    taken_.clear();
    counts_.clear();
    instances_.clear();
    lights_.clear();
    unmade_.clear();
    objects_.clear();
    spaces_.clear();
    spaces_["camera"] = camera;
    current_.lights.clear();
    current_.material = scene::Handle {};
    world_ = true;
}

void SceneClient::worldEnd() {
    // a light that lit every instance names none, and any other names those it lit
    for (const auto & [handle, light] : lights_) {
        if (light.lit.size() != instances_.size()) {
            scene_.set(light.light, "instances", scene::Value::handles(light.lit));
        }
    }
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
    current_.transformation = Sampled { .times = {}, .matrices = { matrix } };
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
    deforming_ = scene::Handle {};
    samples_.clear();
}

void SceneClient::motionEnd() {
    if (!motion_) {
        return;
    }
    motion_ = false;

    // a deforming primitive's poses become samples of its prototype, one at each of the block's
    // times
    if (deforming_.valid()) {
        for (const auto & [name, values] : samples_) {
            const std::size_t count = std::min(values.size(), times_.size());
            if (count < 2) {
                continue;
            }
            try {
                scene_.set(deforming_, name, std::vector<float>(times_.begin(), times_.begin() + static_cast<std::ptrdiff_t>(count)),
                    std::vector<scene::Value>(values.begin(), values.begin() + static_cast<std::ptrdiff_t>(count)));
            } catch (const std::invalid_argument & error) {
                logger_->get()->warn("a deforming primitive's poses differ in their points or its times are not in order, so it keeps its first pose - {}",
                    error.what());
            }
        }
        deforming_ = scene::Handle {};
        samples_.clear();
    }

    // a block that held one request, or named one time, moves nothing
    const std::size_t count = std::min(poses_.size(), times_.size());
    if (count == 1 || (count == 0 && !poses_.empty())) {
        current_.transformation = Sampled { .times = {}, .matrices = { poses_.front() } };
    } else if (count > 1) {
        current_.transformation = Sampled { .times = std::vector<float>(times_.begin(), times_.begin() + static_cast<std::ptrdiff_t>(count)),
            .matrices = std::vector<glm::mat4>(poses_.begin(), poses_.begin() + static_cast<std::ptrdiff_t>(count)) };
    }
}

void SceneClient::attribute(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) {
    if (name == "identifier" && parameters.has("name")) {
        current_.name = parameters.string("name");
    }
}

void SceneClient::setSampled(const scene::Handle & object, const std::string & parameter, const Sampled & sampled) {
    if (sampled.times.empty()) {
        scene_.set(object, parameter, scene::Value::matrix(sampled.matrices.front()));
        return;
    }
    std::vector<scene::Value> values;
    values.reserve(sampled.matrices.size());
    for (const glm::mat4 & matrix : sampled.matrices) {
        values.push_back(scene::Value::matrix(matrix));
    }
    try {
        scene_.set(object, parameter, sampled.times, values);
    } catch (const std::invalid_argument & error) {
        // a motion block whose times do not rise keeps its first time alone
        logger_->get()->warn("a motion block's times are not in order, so it holds its first time alone - {}", error.what());
        scene_.set(object, parameter, values.front());
    }
}

};  // namespace v3d::moya::ri
