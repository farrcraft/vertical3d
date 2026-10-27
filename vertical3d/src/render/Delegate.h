/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

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

namespace v3d::editor {

/**
 * What renders the scene while the user works: the render window now, and later a live render
 * and a rendered viewport.
 *
 * A render reads the scene when it starts, so an edit made while it runs is not in its picture.
 * update() starts it again when anything its picture shows has changed. A renderer works off
 * the caller's thread, and the caller polls status() and picture() for what it has.
 **/
class Delegate {
 public:
    enum class State : std::uint8_t {
        Idle,       /**< nothing has been rendered **/
        Rendering,  /**< a render is running **/
        Finished,   /**< the last render ran to its end, and its picture is the latest **/
        Stopped,    /**< the last render was cancelled **/
        Failed      /**< the last render could not read the scene or could not finish **/
    };

    struct Status {
        State state = State::Idle;
        /** How much of the running or last render is done, from 0 to 1. **/
        float done = 0.0F;
        /** A count that moves each time a new picture is ready. **/
        std::uint64_t pictures = 0;
    };

    Delegate();
    virtual ~Delegate();
    Delegate(const Delegate&) = delete;
    Delegate& operator=(const Delegate&) = delete;

    /**
     * Start rendering the scene from a camera, cancelling any render already running.
     **/
    void render(const Scene& scene, const v3d::type::camera::Profile& from, const RenderSettings& settings);

    /**
     * Render again if the scene, the camera or the settings differ from those the last render
     * was started with. Nothing happens before the first render, or when nothing differs.
     *
     * @return whether a render was started
     **/
    bool update(const Scene& scene, const v3d::type::camera::Profile& from, const RenderSettings& settings);

    /**
     * Stop a running render. Its picture so far is not kept, and the last finished one stays.
     **/
    virtual void cancel() = 0;

    virtual Status status() const = 0;

    /**
     * @return the picture of the last render that finished, or an empty pointer before one has
     **/
    virtual boost::shared_ptr<v3d::image::Image> picture() const = 0;

 protected:
    /**
     * Begin one render. A running render has already been cancelled.
     **/
    virtual void start(const Scene& scene, const v3d::type::camera::Profile& from, const RenderSettings& settings) = 0;

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
        RenderSettings settings;

        static Signature of(const Scene& scene, const v3d::type::camera::Profile& from, const RenderSettings& settings);
        bool operator==(const Signature& other) const = default;
    };

    bool rendered_ = false;
    Signature last_;
};

};  // namespace v3d::editor
