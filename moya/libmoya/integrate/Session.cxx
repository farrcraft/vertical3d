/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/integrate/Session.h>

#include <api/log/Logger.h>
#include <api/rib/Sampling.h>
#include <moya/libmoya/accel/Assembler.h>
#include <moya/libmoya/accel/Assembly.h>
#include <moya/libmoya/accel/Box.h>
#include <moya/libmoya/accel/Sweep.h>
#include <moya/libmoya/camera/Camera.h>
#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/colour/RgbTables.h>
#include <moya/libmoya/denoise/Denoise.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/device/cpu/Module.h>
#include <moya/libmoya/device/cpu/ProfileParams.h>
#include <moya/libmoya/film/Channel.h>
#include <moya/libmoya/film/Film.h>
#include <moya/libmoya/film/Filter.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/film/State.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/geometry/Surfaces.h>
#include <moya/libmoya/integrate/Profile.h>
#include <moya/libmoya/integrate/Render.h>
#include <moya/libmoya/integrate/Trails.h>
#include <moya/libmoya/integrate/internal/Coverage.h>
#include <moya/libmoya/integrate/internal/Crop.h>
#include <moya/libmoya/integrate/internal/Deep.h>
#include <moya/libmoya/integrate/internal/FirstHits.h>
#include <moya/libmoya/integrate/internal/Kept.h>
#include <moya/libmoya/integrate/internal/LightPaths.h>
#include <moya/libmoya/integrate/internal/Options.h>
#include <moya/libmoya/integrate/internal/Records.h>
#include <moya/libmoya/integrate/internal/Tallies.h>
#include <moya/libmoya/integrate/internal/TrailSteps.h>
#include <moya/libmoya/kernel/film/Pixels.h>
#include <moya/libmoya/kernel/Launch.h>
#include <moya/libmoya/kernel/sample/ZSobol.h>
#include <moya/libmoya/light/Lights.h>
#include <moya/libmoya/light/Shading.h>
#include <moya/libmoya/material/Albedos.h>
#include <moya/libmoya/output/Channel.h>
#include <moya/libmoya/output/Checkpoint.h>
#include <moya/libmoya/output/Output.h>
#include <moya/libmoya/output/Outputs.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/texture/Options.h>
#include <moya/libmoya/texture/Textures.h>
#include <moya/libmoya/view/Tracing.h>
#include <moya/libmoya/volume/Volumes.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <exception>
#include <functional>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>

namespace v3d::moya::integrate {

namespace {

/** Makes the pixel filter the scene's options name, of the width they give. **/
void filterInto(std::optional<film::Filter> & out, device::CpuDevice & device, const scene::Scene & scene) {
    rib::Filter kind = rib::Filter::Gaussian;
    rib::filterNamed(internal::option(scene, "filter").stringArray().front(), &kind);
    const std::vector<float> & width = internal::option(scene, "filterWidth").floatArray();
    out.reset();
    out.emplace(device, kind, glm::vec2 { width[0], width[1] });
}

/** Whether any of a scene's outputs is a deep file. **/
bool deepened(const scene::Scene & scene) {
    const std::vector<output::Output> outputs = output::outputsOf(scene);
    return std::any_of(outputs.begin(), outputs.end(), [](const output::Output & each) { return each.format == output::Format::DeepExr; });
}

/** Restores a film and what it keeps from a checkpoint, and gives the passes it held. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the path, then the fingerprint
std::uint32_t resumed(const std::string & path, const std::vector<film::Channel> & channels, std::uint64_t print, film::Film & picture, internal::Kept & kept) {
    const film::State state = output::readCheckpoint(path, channels, print);
    picture.restore(state);
    kept.coverage.restore(state);
    kept.deep.restore(state);
    return state.passes;
}

/** A film's state for a checkpoint, with every pixel's coverage and deep list. **/
film::State stateOf(const film::Film & picture, const internal::Kept & kept) {
    film::State out = picture.state();
    kept.coverage.into(out);
    kept.deep.into(out);
    return out;
}

/** The radius of a sphere about the box that bounds the world over the shutter, or zero. **/
float reach(const accel::LinearBox & bounds) {
    accel::Box all = bounds.start;
    all.grow(bounds.end);
    return all.empty() ? 0.0F : 0.5F * glm::length(all.upper - all.lower);
}

};  // namespace

Session::Session(device::CpuDevice & device, scene::Scene & scene, const boost::shared_ptr<log::Logger> & logger, const colour::Config & config,
    const Checkpoints & checkpoints) :
    Session(device, scene, nullptr, logger, config, checkpoints, nullptr, nullptr) {
    edits_ = &scene;
}

Session::Session(device::CpuDevice & device, const scene::Scene & scene, const geometry::Prototypes * prototypes,
    const boost::shared_ptr<log::Logger> & logger, const colour::Config & config, const Checkpoints & checkpoints, Profile * profile, Trails * trails) :
    device_(device), scene_(scene), logger_(logger), config_(config), checkpoints_(checkpoints), profile_(profile), trails_(trails) {
    if (prototypes != nullptr) {
        given_.emplace(*prototypes);
    } else {
        assembler_.emplace(device, logger);
    }
}

Session::~Session() {
    try {
        stop();
    } catch (const std::exception & error) {
        // a destructor cannot throw, so what the thread threw is only reported
        if (logger_) {
            logger_->get()->error("a render session ended on an error: {}", error.what());
        }
    }
}

void Session::edited() {
    edited_.store(true);
    const std::scoped_lock lock(mutex_);
    told_ = true;
    woken_.notify_all();
}

const geometry::Prototypes & Session::prototypes() const {
    return assembler_ ? assembler_->prototypes() : *given_;
}

void Session::refresh() {
    if (edits_ != nullptr) {
        // an edit said after this is applied by the next refresh, and drops the next pass
        edited_.store(false);
        edits_->apply();
    }
    if (!built_ || scene_.version() != version_) {
        build();
    }
}

void Session::build() {
    if (assembler_) {
        assembler_->update(scene_);
    } else if (!assembly_) {
        assembly_.emplace(accel::assemble(device_, *given_));
    }
    const geometry::Prototypes & made = prototypes();
    const accel::Assembly & assembly = assembler_ ? assembler_->assembly() : *assembly_;
    tracing_.reset();
    tracing_.emplace(device_, made, assembly);
    surfaces_.reset();
    surfaces_.emplace(device_, made, config_);
    if (volumes_) {
        volumes_->update(device_, made);
    } else {
        volumes_.emplace(device_, made);
    }
    const texture::Options options = texture::optionsOf(scene_);
    if (textures_ && textures_->options() == options) {
        textures_->update(device_, made.images);
    } else {
        textures_.reset();
        textures_.emplace(device_, made.images, options, config_, logger_);
    }
    const light::Shading shading { .surfaces = surfaces_->view(), .textures = textures_->view(), .volumes = volumes_->view() };
    if (!lights_) {
        lights_.emplace(device_, config_, logger_);
    }
    lights_->update(scene_, made, shading, reach(assembly.bounds));
    filterInto(filter_, device_, scene_);

    camera_ = camera::fromScene(scene_);
    image_ = glm::uvec2 { static_cast<std::uint32_t>(camera_.resolution.x), static_cast<std::uint32_t>(camera_.resolution.y) };
    window_ = internal::windowOf(internal::option(scene_, "crop").floatArray(), image_, logger_);
    // the film holds the window's pixels alone
    const glm::uvec2 resolution = window_.size;
    sampler_.emplace(std::max(internal::count(scene_, "samples"), 1U), image_, 0, internal::count(scene_, "frame"), window_.origin);
    settings_ = settingsOf(scene_);
    threshold_ = internal::option(scene_, "noiseThreshold").floatArray().front();
    least_ = internal::count(scene_, "minSamples");
    const std::vector<film::Channel> channels = output::filmChannelsOf(scene_);

    // what reads the film goes before it does
    firstHits_.reset();
    lightPaths_.reset();
    madeFilm_ = !film_ || film_->resolution() != resolution || film_->channels() != channels;
    if (madeFilm_) {
        film_.reset();
        film_.emplace(device_, resolution, channels);
    } else {
        film_->clear();
    }
    outputs_ = output::channelsOf(scene_);
    firstHits_.emplace(device_, outputs_, *film_, made.primvarNames, logger_);
    lightPaths_.emplace(device_, outputs_, *film_, lights_->labels(), logger_);
    kept_.reset();
    kept_.emplace(internal::Kept { .coverage = internal::Coverage(resolution, outputs_, scene_, made),
        .deep = internal::Deep(resolution, deepened(scene_), internal::option(scene_, "deepTolerance").floatArray().front(), scene_, made) });
    records_.reset();
    records_.emplace(device_, resolution, kSamplesPerPass, !kept_->coverage.none() || !kept_->deep.none());
    tallies_.reset();
    if (profile_ != nullptr && profile_->counted) {
        tallies_.emplace(device_, resolution);
    }
    trail_.reset();
    trail_.emplace(device_, resolution, trails_ != nullptr);

    print_ = checkpoints_.path.empty() ? 0U : fingerprint(scene_, channels);
    done_ = !built_ && checkpoints_.resume ? resumed(checkpoints_.path, channels, print_, *film_, *kept_) : 0U;
    written_ = done_;
    last_ = std::chrono::steady_clock::now();
    first_ = static_cast<std::uint32_t>(std::min<std::uint64_t>(std::uint64_t { done_ } * kSamplesPerPass, sampler_->samplesPerPixel()));
    ended_ = false;
    version_ = scene_.version();
    built_ = true;
    if (edits_ != nullptr) {
        edits_->forget(version_);
    }
}

bool Session::advance() {
    refresh();
    if (ended_) {
        return false;
    }
    const std::uint32_t total = sampler_->samplesPerPixel();
    // a pass only adds samples that the noise check after the pass before it left sampling.
    // A resumed render repeats the check the render it continues made before this pass
    if (first_ >= total || (checkpoints_.stop && checkpoints_.stop(done_)) ||
        (first_ != 0 && threshold_ > 0.0F && first_ >= least_ && film_->converge(threshold_) == 0U)) {
        ended_ = true;
        written();
        return false;
    }
    const std::uint32_t samples = std::min(kSamplesPerPass, total - first_);
    const std::chrono::steady_clock::time_point began = std::chrono::steady_clock::now();

    // an edit, a stop of the thread and the checkpoints' stop are asked before each tile, the
    // last one thread at a time
    const bool stoppable = checkpoints_.stop || edits_ != nullptr;
    std::mutex asking;
    bool asked = false;
    std::function<bool()> during;
    if (stoppable) {
        during = [&] {
            if (edited_.load() || quitting_.load()) {
                return true;
            }
            if (!checkpoints_.stop) {
                return false;
            }
            const std::scoped_lock lock(asking);
            asked = asked || checkpoints_.stop(done_);
            return asked;
        };
    }
    const auto launch = [&](const kernel::film::Pixels & pixels) {
        const device::cpu::PathParams params { .pixels = pixels,
            .filter = filter_->view(),
            .camera = camera_,
            .sampler = *sampler_,
            .traversal = tracing_->hierarchy(),
            .textures = textures_->view(),
            .surfaces = surfaces_->view(),
            .volumes = volumes_->view(),
            .lights = lights_->view(),
            .table = colour::acescgTable(),
            .albedos = material::ggxAlbedos(),
            .settings = settings_,
            .first = first_,
            .count = samples,
            .firstHits = firstHits_->view(),
            .features = firstHits_->features(),
            .trail = trail_->view(),
            .expressions = lightPaths_->view(),
            .covered = records_->view() };
        if (tallies_) {
            return device_.launch(window_.size.x, window_.size.y, device::cpu::ProfileParams { .paths = params, .tallies = tallies_->view() },
                device_.module().profiled, during);
        }
        return device_.launch(window_.size.x, window_.size.y, params, device_.module().paths, during);
    };
    // a session that cannot stop keeps no copy of the film to go back to
    if (!stoppable) {
        film_->pass([&](const kernel::film::Pixels & pixels) { launch(pixels); });
    } else if (!film_->attempt(launch)) {
        // an edit or a stop of the thread drops the pass, and the next advance goes on
        if (asked) {
            ended_ = true;
            written();
        }
        return !asked;
    }
    kept_->add(records_->taken(samples), samples);
    ++done_;
    first_ += kSamplesPerPass;
    const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    passing_ += std::chrono::duration<double>(now - began).count();
    if (!checkpoints_.path.empty() && std::chrono::duration<double>(now - last_).count() >= checkpoints_.interval) {
        output::writeCheckpoint(checkpoints_.path, stateOf(*film_, *kept_), print_);
        written_ = done_;
        last_ = now;
    }
    return true;
}

void Session::written() {
    if (!checkpoints_.path.empty() && written_ != done_) {
        output::writeCheckpoint(checkpoints_.path, stateOf(*film_, *kept_), print_);
        written_ = done_;
    }
}

void Session::finish() {
    if (profile_ != nullptr) {
        profile_->seconds = passing_;
        profile_->coverage = kept_->coverageSeconds;
        profile_->deep = kept_->deepSeconds;
        if (tallies_) {
            tallies_->into(*profile_);
        }
    }
    if (trails_ != nullptr) {
        trail_->into(*trails_, lights_->labels());
    }
    textures_->report();
}

film::Snapshot Session::picture() const {
    if (!built_) {
        throw std::logic_error("a render session's picture before its first advance");
    }
    film::Snapshot out = film_->snapshot();
    kept_->coverage.into(out);
    kept_->deep.into(out);
    denoise::denoise(out, outputs_, output::denoisesBeauty(scene_), logger_);
    out.origin = window_.origin;
    out.image = image_;
    return out;
}

void Session::start() {
    const std::scoped_lock lock(mutex_);
    if (running_) {
        return;
    }
    running_ = true;
    quit_ = false;
    idle_ = false;
    failed_ = nullptr;
    thread_ = std::thread([this] {
        for (;;) {
            {
                const std::scoped_lock held(mutex_);
                if (quit_) {
                    return;
                }
                told_ = false;
            }
            bool more = false;
            try {
                more = advance();
            } catch (...) {
                const std::scoped_lock held(mutex_);
                failed_ = std::current_exception();
                idle_ = true;
                woken_.notify_all();
                return;
            }
            if (!more) {
                std::unique_lock held(mutex_);
                idle_ = true;
                woken_.notify_all();
                woken_.wait(held, [this] { return quit_ || told_; });
                idle_ = false;
            }
        }
    });
}

void Session::wait() {
    std::unique_lock lock(mutex_);
    woken_.wait(lock, [this] { return !running_ || failed_ || (idle_ && !told_); });
    if (failed_) {
        std::rethrow_exception(failed_);
    }
}

void Session::stop() {
    {
        const std::scoped_lock lock(mutex_);
        if (!running_) {
            return;
        }
        quit_ = true;
        woken_.notify_all();
    }
    quitting_.store(true);
    thread_.join();
    quitting_.store(false);
    std::exception_ptr failed;
    {
        const std::scoped_lock lock(mutex_);
        running_ = false;
        quit_ = false;
        idle_ = false;
        failed = failed_;
        failed_ = nullptr;
        woken_.notify_all();
    }
    if (failed) {
        std::rethrow_exception(failed);
    }
}

};  // namespace v3d::moya::integrate
