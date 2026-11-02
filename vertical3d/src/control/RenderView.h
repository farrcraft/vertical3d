/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
**/

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

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

#include <boost/filesystem/path.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor::control {

/**
 * The render view: an editor that shows what the render is doing and its latest picture, and
 * what renders into it and a batch of frames.
 *
 * Its first row holds a button that renders again, one that stops the render, and the render's
 * state. Below them the picture is fitted to the rest of the region without changing its shape.
 * While any area shows a render view, the scene is rendered again whenever it has
 * changed. Every render view shows the same render.
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

    /**
     * Turn a picture into a texture the ui can draw.
     **/
    typedef std::function<v3d::render::realtime::resource::TextureHandle(const boost::shared_ptr<v3d::image::Image>&)> Upload;

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
     * render's state and its picture when there is a new one.
     *
     * @param shown whether any area shows a render view
     **/
    void tick(const scene::Scene& scene, const v3d::type::camera::Profile& from, bool shown);

    /**
     * Draw one render view into a region of the window.
     *
     * @param editor the editor's id, which keeps its buttons apart from another's
     * @return what a click on its buttons asked for
     **/
    Asked draw(v3d::ui::Immediate* ui, std::size_t editor, const glm::vec4& region);

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
     * Take up the render's state, and its picture when there is a new one.
     **/
    void sync();

    boost::shared_ptr<render::Delegate> delegate_;
    boost::shared_ptr<render::Batch> batch_;
    Upload upload_;
    // the count of the delegate's last picture that is taken up, and the state last said
    std::uint64_t shownPictures_;
    render::Delegate::State shownState_;
    int shownDone_;
    std::string state_;
    v3d::ui::Image picture_;
    glm::uvec2 size_;
};

};  // namespace v3d::editor::control
