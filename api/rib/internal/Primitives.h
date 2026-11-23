/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/rib/Handler.h>
#include <api/rib/Lexer.h>

#include <string>

#include "Operands.h"

namespace v3d::rib::internal {

/**
 * Reads RIB's geometric primitives and calls a handler for each: the polygons, the points and
 * the curves, the subdivision meshes, the quadrics, and RenderMan's volumes.
 **/
class Primitives final {
 public:
    /** @param operands what reads each request's arguments, which outlives this **/
    explicit Primitives(Operands * operands);

    /** Whether a request names a primitive this reads. **/
    static bool recognises(const std::string & name);

    /**
     * Read a primitive's arguments and call the handler for it.
     *
     * @param name a request that recognises() accepts
     * @return false when the arguments do not parse
     **/
    bool read(const std::string & name, Lexer * lexer, Handler * handler);

 private:
    /** A quadric has a varying value at each of the four corners of its parameter space. **/
    static constexpr unsigned int kQuadricCorners = 4;

    /** How many numbers a quadric's request gives before its parameters, or 0 for no quadric. **/
    static unsigned int quadricArguments(const std::string & name);

    /** A Polygon, whose vertex count is the length of its points. **/
    bool polygon(Lexer * lexer, Handler * handler);
    bool pointsPolygons(Lexer * lexer, Handler * handler);
    /** A Points request, whose count is the length of its points. **/
    bool points(Lexer * lexer, Handler * handler);
    /** A Curves request: its type, its counts, its wrap and its parameters. **/
    bool curves(Lexer * lexer, Handler * handler);
    /**
     * A SubdivisionMesh request: its scheme, its faces, its tags and their arguments, and its
     * parameters. The tags' arguments are checked against the counts each tag gives.
     **/
    bool subdivision(Lexer * lexer, Handler * handler);
    /**
     * A Volume request: its type, its box, its three voxel counts, and its parameters, of one
     * varying value per voxel.
     **/
    bool volume(Lexer * lexer, Handler * handler);
    /**
     * A quadric: a fixed run of numbers, then a parameter list of one value per corner of its
     * parameter space.
     **/
    bool quadric(const std::string & name, Lexer * lexer, Handler * handler);

    Operands * operands_;
};

};  // namespace v3d::rib::internal
