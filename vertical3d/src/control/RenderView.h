/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/image/Image.h>
#include <api/render/realtime/resource/Handle.h>
#include <api/type/camera/Profile.h>
#include <api/ui/Image.h>
#include <api/ui/Immediate.h>
#include <vertical3d/src/document/Document.h>
#include <vertical3d/src/render/Batch.h>
#include <vertical3d/src/render/Delegate.h>
#include <vertical3d/src/scene/Scene.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <boost/filesystem/path.hpp>
#include <boost/json.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor::control {

/**
 * The render view: an editor that shows what the render is doing and its latest picture, and
 * what renders into it and a batch of frames.
 *
 * Its first row holds a button that renders again, one that stops the render, the channel the
 * view shows, and the render's state with the samples its picture holds. Below them the picture
 * is fitted to the rest of the region without changing its shape. While any area shows a render
 * view, the scene is rendered again whenever it has changed, and each picture is shown as it
 * comes.
 *
 * **Every render view shows the same render**, each in a channel of its own: the beauty, the
 * denoised beauty, or the albedo or the normal that guide the denoiser. The channel is kept in
 * the editor's state under CHANNEL, so a saved workspace keeps it. It is not a render setting,
 * and changing it does not render again. A picture that does not show the channel, such as a
 * coarse one or one not yet denoised, is shown as its beauty.
 *
 * **The view sets how often pictures come**, from how often it is drawn, and never more often
 * than one every LEAST_INTERVAL seconds. Pictures are denoised, and show the albedo and the
 * normal, only while a view shows that channel.
 **/
class RenderView final {
 public:
    /**
     * The kind of a render view.
     **/
    static constexpr std::string_view KIND = "render-view";

    /**
     * What a click on a render view's buttons asks for.
     **/
    enum class Asked : std::uint8_t { Nothing, Render, Stop };

    typedef render::Delegate::Channel Channel;

    /**
     * The key of the editor's state that names its channel.
     **/
    static constexpr std::string_view CHANNEL = "channel";

    /**
     * The least seconds from one picture to the next. A picture costs the render a copy of
     * its film.
     **/
    static constexpr double LEAST_INTERVAL = 0.1;

    /**
     * The width in pixels of the box that chooses the channel.
     **/
    static constexpr float CHANNEL_WIDTH = 180.0F;

    /**
     * Turn a picture into a texture the ui can draw, in place of the last one uploaded to a slot.
     * Each channel has a slot of its own.
     **/
    typedef std::function<v3d::render::realtime::resource::TextureHandle(const std::string& slot, const boost::shared_ptr<v3d::image::Image>&)> Upload;

    /**
     * @param delegate what renders the view
     * @param batch what renders a batch of frames
     * @param upload what turns a picture into a texture, which may be empty
     **/
    RenderView(const boost::shared_ptr<render::Delegate>& delegate, const boost::shared_ptr<render::Batch>& batch, const Upload& upload);

    /**
     * Render the scene from a camera.
     **/
    void render(const scene::Scene& scene, const v3d::type::camera::Profile& from);

    /**
     * Stop the render and the batch.
     **/
    void stop();

    /**
     * Render every frame of the timeline from each scene camera into a directory, or from a
     * view when the scene has no camera.
     **/
    document::Outcome batch(const scene::Scene* scene, const v3d::type::camera::Profile& view, const boost::filesystem::path& into);

    /**
     * @return how the batch is getting on
     **/
    render::Batch::Status batchStatus() const;

    /**
     * Render again when the scene has changed, while a render view is shown, and take up the
     * render's state and its picture in each channel shown when there is a new one.
     *
     * @param shown the channel of each render view an area shows, empty when none does
     * @param since the last tick
     **/
    void tick(const scene::Scene& scene, const v3d::type::camera::Profile& from, const std::vector<Channel>& shown, std::chrono::milliseconds since);

    /**
     * Draw one render view into a region of the window.
     *
     * @param editor the editor's id, which keeps its buttons apart from another's
     * @param channel read and written - the channel the view shows. Its first row chooses it
     * @return what a click on its buttons asked for
     **/
    Asked draw(v3d::ui::Immediate* ui, std::size_t editor, const glm::vec4& region, Channel* channel);

    /**
     * @return a channel's name in an editor's state: `beauty`, `denoised`, `albedo` or `normal`
     **/
    static std::string_view name(Channel channel);

    /**
     * @return the channel an editor's state names, or the beauty when it names none of the four
     **/
    static Channel channelOf(const boost::json::object& state);

    /**
     * @return what the render is doing, as the first row says it
     **/
    const std::string& state() const noexcept;

    /**
     * Stop the render's thread and the batch's. They report to what the editor is about to
     * destroy, so they are waited for.
     **/
    void release();

    /**
     * @return the largest size of a picture's shape that fits in a room, or 0 by 0 when either
     *         is empty
     **/
    static glm::vec2 fit(const glm::uvec2& picture, const glm::vec2& room);

 private:
    /**
     * A channel's latest picture, as the view draws it.
     **/
    struct Shown {
        v3d::ui::Image picture;
        glm::uvec2 size{ 0 };
        // the count of the delegate's picture it was taken from, or 0 before the first
        std::uint64_t taken = 0;
    };

    /**
     * Take up the render's state, and its picture in each channel shown when there is a new one.
     **/
    void sync();

    boost::shared_ptr<render::Delegate> delegate_;
    boost::shared_ptr<render::Batch> batch_;
    Upload upload_;
    std::string state_;
    // by the channel's value
    std::array<Shown, 4> pictures_;
    // the channels last shown, the seconds a tick takes on average, and what the delegate was
    // last asked to show
    std::vector<Channel> shown_;
    double frame_;
    std::optional<render::Delegate::Showing> showing_;
};

};  // namespace v3d::editor::control
