/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>
#include <api/render/offline/FrameBuffer.h>
#include <api/render/offline/rib/Requests.h>

#include <string>

#include <boost/shared_ptr.hpp>
#include <glm/vec4.hpp>

namespace v3d::moya {

/**
 * Where a finished frame goes, and which part of it is kept: RiDisplay and RiCropWindow.
 **/
class Display final {
 public:
    /**
     * RiDisplay. A display of type "file" is written through image::Factory, which picks the
     * format from the name's extension. An empty name or any other type writes nothing. The
     * mode is stored and not read.
     **/
    void request(const v3d::render::offline::rib::Display & display);
    const std::string & name() const;

    /**
     * RiCropWindow: the part of the image to render, as fractions of its width and height,
     * each held to [0, 1].
     *
     * @return false, keeping the window in force, for a window with no area
     **/
    bool crop(const v3d::render::offline::rib::CropWindow & window);
    /**
     * The pixels inside the crop window of a picture: columns x to y and rows z to w, each
     * range including its start and excluding its end. RI renders the columns from
     * ceil(width * xmin) to ceil(width * xmax) - 1, and the rows likewise.
     **/
    glm::uvec4 cropped(unsigned int width, unsigned int height) const;

    /**
     * Finish a frame: every pixel outside the crop window is emptied, and a display of type
     * "file" has the colour channels written.
     *
     * A filter wider than a pixel spreads samples from inside the window into the pixels just
     * past it, so the pixels are cleared once the hider and the imager are done rather than
     * skipped. An empty pixel has nothing in any plane, and the depth plane at the farthest
     * depth.
     **/
    void finish(v3d::render::offline::FrameBuffer* planes, const boost::shared_ptr<v3d::log::Logger> & logger) const;

 private:
    // an empty name writes no file. RI's default display is a framebuffer, which moya does
    // not have
    std::string name_;
    std::string type_;
    std::string mode_;
    v3d::render::offline::rib::CropWindow crop_;
};

};  // namespace v3d::moya
