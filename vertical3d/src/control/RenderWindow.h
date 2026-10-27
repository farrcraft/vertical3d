/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <api/image/Image.h>
#include <api/render/realtime/Handle.h>
#include <api/type/camera/Profile.h>
#include <vertical3d/src/document/Document.h>
#include <vertical3d/src/render/Batch.h>
#include <vertical3d/src/render/Delegate.h>
#include <vertical3d/src/render/RenderPanels.h>
#include <vertical3d/src/scene/Scene.h>

#include <cstdint>
#include <functional>

#include <boost/filesystem/path.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 * The Render menu: the render window and what renders into it, the render settings panel, and
 * a batch of frames.
 *
 * While the window is open, the scene is rendered again whenever it has changed, and the
 * window shows the render's state and its latest picture.
 **/
class RenderWindow final {
 public:
    /**
     * Turn a picture into a texture the ui can draw.
     **/
    typedef std::function<v3d::render::realtime::TextureHandle(const boost::shared_ptr<v3d::image::Image>&)> Upload;

    /**
     * @param delegate what renders the window
     * @param batch what renders a batch of frames
     **/
    RenderWindow(const boost::shared_ptr<RenderPanels>& panels, const boost::shared_ptr<Delegate>& delegate,
        const boost::shared_ptr<Batch>& batch, const Upload& upload);

    /**
     * Open the window, and render the scene into it from a camera.
     **/
    void show(const Scene& scene, const v3d::type::camera::Profile& from);

    /**
     * Stop the render and the batch, or stop the render and close the window.
     **/
    void stop();
    void close();

    /**
     * Open the settings panel on the document's settings, or put it away.
     **/
    void openSettings(const RenderSettings& settings);
    void closeSettings();

    /**
     * Give the document the settings the panel holds, and put the panel away. Settings that are
     * not valid keep the panel up and change nothing.
     **/
    Outcome applySettings(Document* document);

    /**
     * Render every frame of the timeline from each scene camera into a directory, or from a
     * view when the scene has no camera.
     **/
    Outcome batch(const Scene* scene, const v3d::type::camera::Profile& view, const boost::filesystem::path& into);

    /**
     * @return how the batch is getting on
     **/
    Batch::Status batchStatus() const;

    /**
     * While the window is open, render again when the scene has changed, and show what the
     * render has got to.
     **/
    void tick(const Scene& scene, const v3d::type::camera::Profile& from);

    bool open() const;

    /**
     * Stop the render's thread and the batch's. They report to what the editor is about to
     * destroy, so they are waited for.
     **/
    void release();

 private:
    /**
     * Show the render's state, and its picture when there is a new one.
     **/
    void sync();

    boost::shared_ptr<RenderPanels> panels_;
    boost::shared_ptr<Delegate> delegate_;
    boost::shared_ptr<Batch> batch_;
    Upload upload_;
    // the count of the delegate's last picture that is shown, and the state last shown
    std::uint64_t shownPictures_;
    Delegate::State shownState_;
    int shownDone_;
};

};  // namespace v3d::editor
