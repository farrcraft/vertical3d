/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#include "Surface.h"

namespace v3d::render::realtime {
    /**
     **/
    Surface::Surface(boost::shared_ptr<v3d::image::Image> image) {
        Uint32 rmask, gmask, bmask, amask;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
        int shift = (image->bpp() == 24) ? 8 : 0;
        rmask = 0xff000000 >> shift;
        gmask = 0x00ff0000 >> shift;
        bmask = 0x0000ff00 >> shift;
        amask = 0x000000ff >> shift;
#else // little endian, like x86
        rmask = 0x000000ff;
        gmask = 0x0000ff00;
        bmask = 0x00ff0000;
        amask = (image->bpp() == 24) ? 0 : 0xff000000;
#endif
        surface_ = SDL_CreateRGBSurfaceFrom(image->data(), image->width(), image->height(), image->bpp(), ((image->bpp() / 8) * image->width()), rmask, gmask, bmask, amask);
        if (surface_ == nullptr) {
            std::string msg("Error creating surface from image: ");
            msg += SDL_GetError();
            throw std::exception(msg.c_str());
        }
    }

    /**
     **/
    Surface::~Surface() {
        if (surface_ != nullptr) {
            SDL_FreeSurface(surface_);
        }
    }

    /**
     **/
    SDL_Surface* Surface::surface() {
        return surface_;
    }

};  // namespace v3d::render::realtime
