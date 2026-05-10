/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#include "Surface.h"

#include <string>
#include <stdexcept>

namespace v3d::render::realtime {
    /**
     **/
    Surface::Surface(boost::shared_ptr<v3d::image::Image> image) {
        SDL_PixelFormat format = (image->bpp() == 24) ? SDL_PIXELFORMAT_RGB24 : SDL_PIXELFORMAT_RGBA32;
        int pitch = (image->bpp() / 8) * image->width();
        surface_ = SDL_CreateSurfaceFrom(image->width(), image->height(), format, image->data(), pitch);
        if (surface_ == nullptr) {
            std::string msg("Error creating surface from image: ");
            msg += SDL_GetError();
            throw std::runtime_error(msg);
        }
    }

    /**
     **/
    Surface::~Surface() {
        if (surface_ != nullptr) {
            SDL_DestroySurface(surface_);
        }
    }

    /**
     **/
    SDL_Surface* Surface::surface() {
        return surface_;
    }

};  // namespace v3d::render::realtime
