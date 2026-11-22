/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstddef>

namespace v3d::moya::integrate {

/**
 * What a session's last build rebuilt, and the seconds each of its structures took. A build from
 * nothing makes every structure. A build after an edit keeps what the edit did not touch.
 **/
struct Built {
    /** The seconds of the gather and the hierarchies, which a session given prototypes skips. **/
    double geometry { 0.0 };

    /**
     * The seconds of the surfaces, which hold the materials and what shading reads of the
     * geometry, the volumes, the textures, the lights and the pixel filter.
     **/
    double surfaces { 0.0 };
    double volumes { 0.0 };
    double textures { 0.0 };
    double lights { 0.0 };
    double filter { 0.0 };

    /** The seconds of the camera, the sampler and the film, with what reads and records it. **/
    double film { 0.0 };

    /**
     * The geometries and groups whose hierarchies were built, and the materials compiled. Each is
     * zero for a session given prototypes, which builds its hierarchies once.
     **/
    std::size_t geometries { 0 };
    std::size_t groups { 0 };
    std::size_t compiled { 0 };

    /** The lights made, and whether the lights were placed and their tree built. **/
    std::size_t madeLights { 0 };
    bool tree { false };

    /** Whether the volumes' grids were laid out, and whether the film was made or only emptied. **/
    bool laidOut { false };
    bool madeFilm { false };

    /** Whether the surfaces and the pixel filter were made again, not kept from the last build. **/
    bool madeSurfaces { false };
    bool madeFilter { false };

    /** The seconds of the whole build. **/
    double seconds() const {
        return geometry + surfaces + volumes + textures + lights + filter + film;
    }
};

};  // namespace v3d::moya::integrate
