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
#include <moya/libmoya/denoise/Denoiser.h>
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
#include <moya/libmoya/integrate/Built.h>
#include <moya/libmoya/integrate/Picture.h>
#include <moya/libmoya/integrate/Pictures.h>
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
#include <moya/libmoya/kernel/camera/Camera.h>
#include <moya/libmoya/kernel/film/Pixels.h>
#include <moya/libmoya/kernel/sample/ZSobol.h>
#include <moya/libmoya/light/Lights.h>
#include <moya/libmoya/light/Shading.h>
#include <moya/libmoya/material/Albedos.h>
#include <moya/libmoya/output/Checkpoint.h>
#include <moya/libmoya/output/Output.h>
#include <moya/libmoya/output/Outputs.h>
#include <moya/libmoya/output/Png.h>
#include <moya/libmoya/scene/Change.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/texture/Options.h>
#include <moya/libmoya/texture/Textures.h>
#include <moya/libmoya/view/Tracing.h>
#include <moya/libmoya/volume/Volumes.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
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
#include <glm/common.hpp>
#include <glm/vec2.hpp>

namespace v3d::moya::integrate {

namespace {

/** What a session throws when it reads a structure it has not built. **/
constexpr const char * kUnbuilt = "a render session read a structure before it built one";

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

/**
 * A film's state for a checkpoint, with every pixel's coverage and deep list, and the passes of
 * kSamplesPerPass its samples make.
 **/
film::State stateOf(const film::Film & picture, const internal::Kept & kept, std::uint32_t passes) {
    film::State out = picture.state();
    out.passes = passes;
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
    Built record;
    auto mark = std::chrono::steady_clock::now();
    // the seconds since the last lap, or since the build began
    const auto lap = [&mark] {
        const auto now = std::chrono::steady_clock::now();
        const double out = std::chrono::duration<double>(now - mark).count();
        mark = now;
        return out;
    };
    buildScene(&record, lap);
    buildFilm();
    record.film = lap();
    if (!lights_ || !volumes_) {
        throw std::logic_error(kUnbuilt);
    }
    if (assembler_) {
        record.geometries = assembler_->builtGeometries().size();
        record.groups = assembler_->builtGroups().size();
        record.compiled = assembler_->compiledMaterials().size();
    }
    record.madeLights = lights_->madeLights().size();
    record.tree = lights_->builtTree();
    record.laidOut = volumes_->laidOut();
    record.madeFilm = madeFilm_;
    lastBuild_ = record;
}

void Session::buildScene(Built * record, const std::function<double()> & lap) {
    if (assembler_) {
        assembler_->update(scene_);
    } else if (!assembly_) {
        assembly_.emplace(accel::assemble(device_, *given_));
    }
    const geometry::Prototypes & made = prototypes();
    const accel::Assembly & assembly = assembler_ ? assembler_->assembly() : *assembly_;
    tracing_.reset();
    tracing_.emplace(device_, made, assembly);
    record->geometry = lap();
    // the surfaces are made from the prototypes alone, so a version whose edits touched only
    // cameras, and gathered no geometry, group or material anew, keeps them
    const std::vector<scene::Change> changed = built_ ? scene_.changes(version_) : std::vector<scene::Change> {};
    const auto only = [&](const char * kind) {
        const std::optional<std::uint32_t> index = scene_.schema().find(kind);
        return built_ && index && std::all_of(changed.begin(), changed.end(), [&](const scene::Change & each) { return each.handle.kind == *index; });
    };
    const bool gatheredNothing = !assembler_ ||
        (assembler_->builtGeometries().empty() && assembler_->builtGroups().empty() && assembler_->compiledMaterials().empty());
    record->madeSurfaces = !surfaces_ || !gatheredNothing || (assembler_ && !only("camera"));
    if (record->madeSurfaces) {
        surfaces_.reset();
        surfaces_.emplace(device_, made, config_);
    }
    record->surfaces = lap();
    if (volumes_) {
        volumes_->update(device_, made);
    } else {
        volumes_.emplace(device_, made);
    }
    record->volumes = lap();
    const texture::Options options = texture::optionsOf(scene_);
    if (textures_ && textures_->options() == options) {
        textures_->update(device_, made.images);
    } else {
        textures_.reset();
        textures_.emplace(device_, made.images, options, config_, logger_);
    }
    record->textures = lap();
    const light::Shading shading { .surfaces = surfaces_->view(), .textures = textures_->view(), .volumes = volumes_->view() };
    if (!lights_) {
        lights_.emplace(device_, config_, logger_);
    }
    lights_->update(scene_, made, shading, reach(assembly.bounds));
    record->lights = lap();
    // the pixel filter is made from the options alone
    const std::optional<std::uint32_t> optionsKind = scene_.schema().find("options");
    record->madeFilter = !filter_ || !built_ || !optionsKind ||
        std::any_of(changed.begin(), changed.end(), [&](const scene::Change & each) { return each.handle.kind == *optionsKind; });
    if (record->madeFilter) {
        filterInto(filter_, device_, scene_);
    }
    record->filter = lap();
}

void Session::buildFilm() {
    if (!lights_) {
        throw std::logic_error(kUnbuilt);
    }
    const geometry::Prototypes & made = prototypes();
    camera_ = camera::fromScene(scene_);
    image_ = glm::uvec2 { static_cast<std::uint32_t>(camera_.resolution.x), static_cast<std::uint32_t>(camera_.resolution.y) };
    window_ = internal::windowOf(internal::option(scene_, "crop").floatArray(), image_, logger_);
    // the film holds the window's pixels alone
    const glm::uvec2 resolution = window_.size;
    frame_ = internal::count(scene_, "frame");
    sampler_.emplace(std::max(internal::count(scene_, "samples"), 1U), image_, 0, frame_, window_.origin);
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
    // the first coarse pass's blocks are the least power of two that makes the film coarse enough
    block_ = 1U;
    const std::uint32_t longer = std::max(resolution.x, resolution.y);
    while (pictures_ && pictures_->coarsest != 0U && (longer + block_ - 1U) / block_ > pictures_->coarsest) {
        block_ *= 2U;
    }
    fresh_ = true;
    unseen_ = done_ != 0U;
    settled_ = false;
    version_ = scene_.version();
    built_ = true;
    if (edits_ != nullptr) {
        edits_->forget(version_);
    }
}

bool Session::stopping() {
    if (edited_.load() || quitting_.load()) {
        return true;
    }
    if (!checkpoints_.stop) {
        return false;
    }
    const std::scoped_lock lock(asking_);
    asked_ = asked_ || checkpoints_.stop(done_);
    return asked_;
}

bool Session::advance() {
    refresh();
    if (!sampler_ || !film_ || !kept_ || !records_) {
        throw std::logic_error(kUnbuilt);
    }
    film::Film & film = *film_;
    internal::Kept & kept = *kept_;
    const internal::Records & records = *records_;
    if (ended_) {
        return false;
    }
    const std::uint32_t total = sampler_->samplesPerPixel();
    const std::uint32_t run = std::min(kSamplesPerPass, total);
    // a pass only adds samples that the noise check after the pass before it left sampling, and
    // the check runs once the film holds whole passes. A resumed render repeats the check the
    // render it continues made before this pass
    if (first_ >= total || (checkpoints_.stop && checkpoints_.stop(done_)) ||
        (first_ != 0 && first_ % run == 0 && threshold_ > 0.0F && first_ >= least_ && film.converge(threshold_) == 0U)) {
        close();
        return false;
    }

    // an edit, a stop of the thread and the checkpoints' stop are asked before each tile
    asked_ = false;
    std::function<bool()> during;
    if (checkpoints_.stop || edits_ != nullptr) {
        during = [this] { return stopping(); };
    }
    // an edit or a stop of the thread drops a pass, and the next advance goes on
    const auto dropped = [this] {
        if (asked_) {
            close();
        }
        return !asked_;
    };
    if (block_ > 1U) {
        if (!coarse(during)) {
            return dropped();
        }
        block_ /= 2U;
        return true;
    }

    // a session that makes pictures doubles the samples the film holds with each pass, until it
    // holds a whole pass
    const std::uint32_t samples = std::min(pictures_ && first_ < run ? std::max(first_, 1U) : run, total - first_);
    const std::chrono::steady_clock::time_point began = std::chrono::steady_clock::now();
    if (!sample(samples, run, during)) {
        return dropped();
    }
    kept.add(records.taken(samples), samples);
    ++done_;
    first_ += samples;
    unseen_ = true;
    const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    passing_ += std::chrono::duration<double>(now - began).count();
    if (!checkpoints_.path.empty() && first_ % run == 0 && std::chrono::duration<double>(now - last_).count() >= checkpoints_.interval) {
        output::writeCheckpoint(checkpoints_.path, stateOf(film, kept, first_ / run), print_);
        written_ = done_;
        last_ = now;
    }
    pictured(fresh_ || first_ >= total, first_ >= total);
    return true;
}

void Session::close() {
    ended_ = true;
    written();
    // the version ends on a picture of every sample, denoised as a batch render is
    if (first_ != 0U && (unseen_ || (denoiser_ && !settled_))) {
        pictured(true, true);
    }
}

bool Session::sample(std::uint32_t samples, std::uint32_t run, const std::function<bool()> & during) {
    if (!film_) {
        throw std::logic_error(kUnbuilt);
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
            .run = run,
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
    if (!during) {
        film_->pass([&](const kernel::film::Pixels & pixels) { launch(pixels); });
        return true;
    }
    return film_->attempt(launch);
}

bool Session::coarse(const std::function<bool()> & during) {
    if (!lights_ || !sampler_ || !display_) {
        throw std::logic_error(kUnbuilt);
    }
    // the blocks the window covers, of a grid that starts at the image's top left
    const std::uint32_t block = block_;
    const glm::uvec2 low = window_.origin / block;
    const glm::uvec2 high = (window_.origin + window_.size + (block - 1U)) / block;
    const glm::uvec2 size = high - low;
    film::Film film(device_, size, {});
    const internal::FirstHits hits(device_, {}, film, prototypes().primvarNames, logger_);
    const internal::LightPaths paths(device_, {}, film, lights_->labels(), logger_);
    const internal::Records records(device_, size, 1, false);
    const internal::TrailSteps trail(device_, size, false);
    kernel::camera::Camera camera = camera_;
    camera.resolution /= static_cast<float>(block);
    const kernel::sample::ZSobol sampler(1, (image_ + (block - 1U)) / block, 0, frame_, low);
    bool ran = true;
    film.pass([&](const kernel::film::Pixels & pixels) {
        const device::cpu::PathParams params { .pixels = pixels,
            .filter = filter_->view(),
            .camera = camera,
            .sampler = sampler,
            .traversal = tracing_->hierarchy(),
            .textures = textures_->view(),
            .surfaces = surfaces_->view(),
            .volumes = volumes_->view(),
            .lights = lights_->view(),
            .table = colour::acescgTable(),
            .albedos = material::ggxAlbedos(),
            .settings = settings_,
            .first = 0,
            .count = 1,
            .run = 1,
            .firstHits = hits.view(),
            .features = hits.features(),
            .trail = trail.view(),
            .expressions = paths.view(),
            .covered = records.view() };
        ran = device_.launch(size.x, size.y, params, device_.module().paths, during);
    });
    if (!ran) {
        return false;
    }

    // each pixel of the picture takes the colour of the block it is in
    const std::vector<std::uint8_t> blocks = output::displayed(film.snapshot(), *display_);
    Picture out { .version = version_, .samples = 1, .total = sampler_->samplesPerPixel(), .block = block, .resolution = window_.size, .image = image_, .origin = window_.origin };
    out.rgb.resize(static_cast<std::size_t>(window_.size.x) * window_.size.y * 3U);
    for (std::uint32_t y = 0; y < window_.size.y; ++y) {
        const std::size_t row = ((window_.origin.y + y) / block) - low.y;
        for (std::uint32_t x = 0; x < window_.size.x; ++x) {
            const std::size_t from = ((row * size.x) + ((window_.origin.x + x) / block) - low.x) * 3U;
            const std::size_t to = ((static_cast<std::size_t>(y) * window_.size.x) + x) * 3U;
            std::copy_n(blocks.begin() + static_cast<std::ptrdiff_t>(from), 3, out.rgb.begin() + static_cast<std::ptrdiff_t>(to));
        }
    }
    shown(out);
    return true;
}

void Session::pictured(bool forced, bool last) {
    if (!pictures_ || (!forced && std::chrono::duration<double>(std::chrono::steady_clock::now() - pictured_).count() < pictures_->interval)) {
        return;
    }
    if (!film_ || !sampler_ || !display_) {
        throw std::logic_error(kUnbuilt);
    }
    const film::Snapshot snapshot = film_->snapshot();
    Picture out { .version = version_,
        .samples = first_,
        .total = sampler_->samplesPerPixel(),
        .block = 1,
        .resolution = window_.size,
        .image = image_,
        .origin = window_.origin,
        .rgb = output::displayed(snapshot, *display_) };
    if (denoiser_ && (last || first_ >= pictures_->denoiseFrom)) {
        const film::Snapshot denoised { .resolution = snapshot.resolution, .stride = 3, .values = denoiser_->beauty(snapshot, outputs_, !last) };
        out.denoised = output::displayed(denoised, *display_);
    }
    if (pictures_->guides) {
        const std::optional<std::uint32_t> albedo = output::guideOf(snapshot, outputs_, "albedo");
        const std::optional<std::uint32_t> normal = output::guideOf(snapshot, outputs_, "denoisingNormal");
        if (albedo && normal) {
            out.albedo = output::displayed(snapshot, *albedo, *display_);
            out.normal = output::directions(snapshot, *normal);
        }
    }
    unseen_ = false;
    settled_ = last;
    shown(out);
}

void Session::shown(const Picture & picture) {
    if (!pictures_) {
        throw std::logic_error(kUnbuilt);
    }
    fresh_ = false;
    if (pictures_->shown) {
        pictures_->shown(picture);
    }
    pictured_ = std::chrono::steady_clock::now();
}

void Session::show(const Pictures & pictures) {
    const std::string display = pictures.display.empty() ? config_.defaultDisplay() : pictures.display;
    display_.emplace(config_.display(display, pictures.view.empty() ? config_.defaultView(display) : pictures.view));
    pictures_ = pictures;
    denoiser_.reset();
    if (pictures.denoised) {
        denoiser_.emplace(logger_);
    }
}

void Session::again() {
    if (!pictures_ || first_ == 0U) {
        return;
    }
    pictured(true, ended_);
}

void Session::resume() {
    ended_ = false;
}

void Session::written() {
    if (!sampler_ || !film_ || !kept_) {
        throw std::logic_error(kUnbuilt);
    }
    const std::uint32_t run = std::min(kSamplesPerPass, sampler_->samplesPerPixel());
    if (!checkpoints_.path.empty() && written_ != done_ && first_ % run == 0) {
        output::writeCheckpoint(checkpoints_.path, stateOf(*film_, *kept_, first_ / run), print_);
        written_ = done_;
    }
}

void Session::finish() {
    if (!kept_ || !trail_ || !lights_ || !textures_) {
        throw std::logic_error(kUnbuilt);
    }
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
    if (!built_ || !film_ || !kept_) {
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
