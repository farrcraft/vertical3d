/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/image/Image.h>
#include <api/type/camera/Profile.h>
#include <vertical3d/src/scene/RenderSettings.h>
#include <vertical3d/src/scene/Scene.h>

#include <cstdint>
#include <map>
#include <string>
#include <tuple>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::editor::render {

/**
 * What renders the scene while the user works: the render window now, and later a live render
 * and a rendered viewport.
 *
 * An edit reaches a running render. update() hands the render whatever its picture shows that
 * has changed, and the render takes it without being cancelled. A renderer works off the
 * caller's thread, and the caller polls status() and picture() for what it has. show() says
 * which channels the caller wants pictures of and how often.
 **/
class Delegate {
 public:
    enum class State : std::uint8_t {
        Idle,       /**< nothing has been rendered **/
        Rendering,  /**< a render is running **/
        Finished,   /**< the render holds every sample of the scene as it was last updated **/
        Stopped,    /**< the render was cancelled, and its picture holds what it sampled **/
        Failed      /**< the last render could not read the scene or could not finish **/
    };

    struct Status {
        State state = State::Idle;
        /** How much of the running or last render is done, from 0 to 1. **/
        float done = 0.0F;
        /** The samples each pixel of the latest picture holds, and those it holds when done. **/
        std::uint32_t samples = 0;
        std::uint32_t total = 0;
        /** A count that moves each time a new picture is ready. **/
        std::uint64_t pictures = 0;
    };

    /**
     * What a picture shows: the beauty, the beauty denoised, or the albedo or the normal that guide
     * the denoiser.
     **/
    enum class Channel : std::uint8_t { Beauty, Denoised, Albedo, Normal };

    /** The least seconds from one picture to the next until show() names another. **/
    static constexpr double kInterval = 0.1;

    /**
     * Which pictures the caller wants, and how often.
     **/
    struct Showing {
        /** The least seconds between two pictures, but for the first of each edit and the last. **/
        double interval = kInterval;
        /** Whether pictures are denoised too. **/
        bool denoised = false;
        /** Whether pictures show the albedo and the normal too. **/
        bool guides = false;

        bool operator==(const Showing& other) const = default;
    };

    Delegate();
    virtual ~Delegate();
    Delegate(const Delegate&) = delete;
    Delegate& operator=(const Delegate&) = delete;

    /**
     * Start rendering the scene from a camera, from its first sample. A render already running
     * starts again.
     **/
    void render(const scene::Scene& scene, const v3d::type::camera::Profile& from, const scene::RenderSettings& settings);

    /**
     * Hand the render the scene, the camera and the settings if they differ from those it last
     * had. A running render takes them between two passes and samples them from the first
     * sample, and a stopped or finished one starts again. Nothing happens before the first
     * render, or when nothing differs.
     *
     * @return whether the render took a change
     **/
    bool update(const scene::Scene& scene, const v3d::type::camera::Profile& from, const scene::RenderSettings& settings);

    /**
     * Stop a running render. Its latest picture stays, and holds what was sampled before the stop.
     **/
    virtual void cancel() = 0;

    /**
     * Let a stopped render sample on from the samples its picture holds. It ends on the picture a
     * render never stopped ends on. Nothing happens to a render that is not stopped.
     **/
    virtual void resume() = 0;

    virtual Status status() const = 0;

    /**
     * Say which pictures to make, and how often. A running render makes them from its next
     * picture, and a render that has finished or stopped makes its latest picture again. A render
     * started later makes them from its first.
     **/
    virtual void show(const Showing& showing) = 0;

    /**
     * @param channel what the picture shows. A picture that does not show it gives its beauty:
     *        one not denoised yet, or a coarse picture, which shows no albedo or normal
     * @return the latest picture, which improves while a render runs, or an empty pointer before
     *         the first
     **/
    virtual boost::shared_ptr<v3d::image::Image> picture(Channel channel) const = 0;

 protected:
    /**
     * Begin a render from its first sample, ending any render already running.
     **/
    virtual void start(const scene::Scene& scene, const v3d::type::camera::Profile& from, const scene::RenderSettings& settings) = 0;

    /**
     * Hand the render a scene, a camera or settings that differ from those it last had.
     *
     * @return whether the render took a change
     **/
    virtual bool edit(const scene::Scene& scene, const v3d::type::camera::Profile& from, const scene::RenderSettings& settings) = 0;

 private:
    /**
     * Everything a picture shows: each item's document id, revision and parent, the values an
     * evaluation of the current frame gives each one, the camera and the settings. Two equal
     * signatures render the same picture.
     **/
    struct Signature {
        std::vector<std::tuple<std::int64_t, std::uint64_t, std::int64_t>> items;
        std::vector<std::map<std::string, float>> poses;
        glm::vec3 eye{ 0.0F };
        glm::vec3 direction{ 0.0F };
        glm::vec3 up{ 0.0F };
        bool orthographic = false;
        float fov = 0.0F;
        float zoom = 0.0F;
        glm::vec2 clipping{ 0.0F };
        scene::RenderSettings settings;

        static Signature of(const scene::Scene& scene, const v3d::type::camera::Profile& from, const scene::RenderSettings& settings);
        bool operator==(const Signature& other) const = default;
    };

    bool rendered_ = false;
    Signature last_;
};

};  // namespace v3d::editor::render
