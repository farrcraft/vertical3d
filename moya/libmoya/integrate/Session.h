/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/accel/Assembler.h>
#include <moya/libmoya/accel/Assembly.h>
#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/film/Channel.h>
#include <moya/libmoya/film/Film.h>
#include <moya/libmoya/film/Filter.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/geometry/Surfaces.h>
#include <moya/libmoya/integrate/Profile.h>
#include <moya/libmoya/integrate/Render.h>
#include <moya/libmoya/integrate/Trails.h>
#include <moya/libmoya/integrate/internal/Crop.h>
#include <moya/libmoya/integrate/internal/FirstHits.h>
#include <moya/libmoya/integrate/internal/Kept.h>
#include <moya/libmoya/integrate/internal/LightPaths.h>
#include <moya/libmoya/integrate/internal/Records.h>
#include <moya/libmoya/integrate/internal/Tallies.h>
#include <moya/libmoya/integrate/internal/TrailSteps.h>
#include <moya/libmoya/kernel/camera/Camera.h>
#include <moya/libmoya/kernel/integrate/Settings.h>
#include <moya/libmoya/kernel/sample/ZSobol.h>
#include <moya/libmoya/light/Lights.h>
#include <moya/libmoya/output/Channel.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/texture/Textures.h>
#include <moya/libmoya/view/Tracing.h>
#include <moya/libmoya/volume/Volumes.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::log {
class Logger;
};  // namespace v3d::log

namespace v3d::moya::integrate {

/**
 * A render of a scene that takes the edits clients commit to it while it renders. It keeps every
 * structure it builds from one version of the scene to the next.
 *
 * **Between two passes, it applies what was committed.** advance() applies every batch committed
 * since the last, asks the scene what changed, and rebuilds only what the changes touched. The
 * hierarchies, the materials, the textures, the volumes and the lights are each kept where the
 * changes leave them as they were. Any new version starts the film again, at the first sample.
 * A pass never sees half a batch, and a batch committed during a pass is not in its samples.
 *
 * **An edited session ends where a fresh render does.** A session that took edits, and sampled to
 * its end, gives the bytes in every channel that render() gives for the edited scene.
 *
 * **The scene's owner is the session.** It applies the scene and reads it, so while a session
 * lives nothing else applies or reads the scene. Clients edit and commit it from any thread. Once
 * the session has built past a version it forgets the scene's removals up to it.
 *
 * **It runs on a thread of its own, or on its caller's.** advance(), picture() and the counts
 * are called by one thread while no thread of the session's runs. start() runs passes on a thread
 * of the session's, until stop(). edited() and wait() may be called from any thread.
 *
 * The checkpoint holds the version the session renders, counted from the edit that last started
 * the film. A session resumes from it only when it first builds. The device, the scene and the
 * configuration outlive the session.
 **/
class Session final {
 public:
    /**
     * A session over a scene, which builds nothing until the first advance().
     *
     * @throws std::runtime_error when the CPU device's library cannot be loaded
     **/
    Session(device::CpuDevice & device, scene::Scene & scene, const boost::shared_ptr<log::Logger> & logger, const colour::Config & config,
        const Checkpoints & checkpoints = {});

    Session(const Session &) = delete;
    Session & operator=(const Session &) = delete;
    Session(Session &&) = delete;
    Session & operator=(Session &&) = delete;

    /** Stops the session's thread, if it runs. **/
    ~Session();

    /**
     * Says that a client committed a batch. A pass in progress stops before its next tile and is
     * dropped, and a session that was waiting applies the batch. A client that commits without
     * saying so has its batch applied once the pass in progress ends.
     **/
    void edited();

    /**
     * Applies what was committed, rebuilds what it changed, and runs one pass. A pass that an
     * edit or the checkpoints' stop dropped is not counted.
     *
     * @return whether a pass ran, which is false once the film holds every sample this version
     *         takes, every pixel has stopped, or the checkpoints' stop answered true
     * @throws what render() throws
     **/
    bool advance();

    /**
     * The film as the last pass left it, with each Cryptomatte layer's ranks and manifest, its
     * deep samples, and the denoised copies the scene asks for.
     **/
    film::Snapshot picture() const;

    /** The scene's version the session renders. **/
    std::uint64_t version() const {
        return version_;
    }

    /** The passes the film holds, counted from the version that last started it. **/
    std::uint32_t passes() const {
        return done_;
    }

    /** Whether the last build made a new film, in place of emptying the one before. **/
    bool madeFilm() const {
        return madeFilm_;
    }

    /**
     * Runs passes on a thread of the session's until stop(). Once advance() runs no pass, the
     * thread waits for edited(). Nothing happens while the thread runs.
     **/
    void start();

    /**
     * Waits until the session's thread runs no pass and nothing was said to be committed since.
     * Returns at once while no thread runs.
     *
     * @throws what advance() threw on the session's thread, which stops it
     **/
    void wait();

    /**
     * Stops the session's thread after the pass in progress, which is dropped, and waits for it.
     *
     * @throws what advance() threw on the session's thread
     **/
    void stop();

 private:
    friend film::Snapshot render(device::CpuDevice & device, const scene::Scene & scene, const boost::shared_ptr<log::Logger> & logger,
        const colour::Config & config, const Checkpoints & checkpoints, Profile * profile, Trails * trails);
    friend film::Snapshot render(device::CpuDevice & device, const scene::Scene & scene, const geometry::Prototypes & prototypes,
        const boost::shared_ptr<log::Logger> & logger, const colour::Config & config, const Checkpoints & checkpoints, Profile * profile,
        Trails * trails);

    /**
     * A session that takes no edits, over the prototypes given or, when there are none, those
     * gathered from the scene. It fills the profile and the trails, either of which may be null.
     **/
    Session(device::CpuDevice & device, const scene::Scene & scene, const geometry::Prototypes * prototypes, const boost::shared_ptr<log::Logger> & logger,
        const colour::Config & config, const Checkpoints & checkpoints, Profile * profile, Trails * trails);

    /** Applies what was committed and, at a new version or the first, rebuilds. **/
    void refresh();

    /** Brings every structure up to the scene's version, and starts the film again. **/
    void build();

    /** Writes the checkpoint when the passes since it was last written are not in it. **/
    void written();

    /** Fills the profile and the trails, and reports the textures read. **/
    void finish();

    /** The prototypes the structures were built from. **/
    const geometry::Prototypes & prototypes() const;

    device::CpuDevice & device_;
    const scene::Scene & scene_;
    scene::Scene * edits_ { nullptr };
    boost::shared_ptr<log::Logger> logger_;
    const colour::Config & config_;
    Checkpoints checkpoints_;
    Profile * profile_ { nullptr };
    Trails * trails_ { nullptr };

    /** The prototypes given, which an assembly is built over once, or an assembler. **/
    std::optional<geometry::Prototypes> given_;
    std::optional<accel::Assembly> assembly_;
    std::optional<accel::Assembler> assembler_;

    std::optional<view::Tracing> tracing_;
    std::optional<geometry::Surfaces> surfaces_;
    std::optional<volume::Volumes> volumes_;
    std::optional<texture::Textures> textures_;
    std::optional<light::Lights> lights_;
    std::optional<film::Filter> filter_;

    kernel::camera::Camera camera_ {};
    glm::uvec2 image_ { 0U };
    internal::Window window_ {};
    std::optional<kernel::sample::ZSobol> sampler_;
    kernel::integrate::Settings settings_ {};
    float threshold_ { 0.0F };
    std::uint32_t least_ { 0 };
    std::vector<output::Channel> outputs_;

    std::optional<film::Film> film_;
    std::optional<internal::FirstHits> firstHits_;
    std::optional<internal::LightPaths> lightPaths_;
    std::optional<internal::Kept> kept_;
    std::optional<internal::Records> records_;
    std::optional<internal::Tallies> tallies_;
    std::optional<internal::TrailSteps> trail_;

    std::uint64_t version_ { 0 };
    bool built_ { false };
    bool madeFilm_ { false };
    std::uint64_t print_ { 0 };
    std::uint32_t done_ { 0 };
    std::uint32_t written_ { 0 };
    std::uint32_t first_ { 0 };
    bool ended_ { false };
    double passing_ { 0.0 };
    std::chrono::steady_clock::time_point last_ {};

    /** Set by edited(), and taken by the next apply. **/
    std::atomic<bool> edited_ { false };

    /** Set while stop() waits for the session's thread, which drops the pass in progress. **/
    std::atomic<bool> quitting_ { false };

    /** The session's thread, and what it shares with the threads that call it. **/
    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable woken_;
    bool running_ { false };
    bool quit_ { false };
    bool idle_ { false };
    bool told_ { false };
    std::exception_ptr failed_;
};

};  // namespace v3d::moya::integrate
