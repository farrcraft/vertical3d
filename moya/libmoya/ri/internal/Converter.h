/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/film/Sampling.h>
#include <api/render/offline/rib/Parameters.h>
#include <api/render/offline/rib/Requests.h>
#include <moya/libmoya/ri/RenderMan.h>
#include <moya/libmoya/ri/Renderer.h>

#include <cstdarg>
#include <optional>
#include <string>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>

namespace v3d::moya::ri::internal {

/**
 * Turns the arguments of a RenderMan Interface C entry point into the request and the typed
 * parameter list a RIBHandler takes. RI_NULL becomes the empty name, a C filter function
 * becomes a Filter, and untyped parameter arrays become a typed list.
 *
 * A conversion returns nothing for arguments that the RIB reader would refuse in a file, and
 * the entry point then skips the request.
 **/
class Converter {
 public:
    /**
     * @param renderer the renderer whose active handler types a parameter list and logs each
     *        warning. The handler is asked for only when a conversion reads it, so a request
     *        skipped for its arguments alone does not begin a context
     **/
    explicit Converter(Renderer & renderer);

    /**
     * A name as the handler takes it. RI_NULL is the empty name, which the handler ignores
     * wherever RI needs a name, and reads as RI's meaning for null elsewhere.
     **/
    static std::string text(const char* token);

    /**
     * Read a variadic parameter list into the two arrays the vector form takes.
     *
     * RI terminates the list with RI_NULL, because a va_list carries no count. A variadic entry
     * point that calls this has no body of its own. It hands the arrays to its vector form, so
     * the two forms of a request cannot drift apart.
     **/
    static void collect(va_list arguments, std::vector<RtToken>* tokens, std::vector<RtPointer>* parms);

    /**
     * Read RiMotionBegin's times. Each time is promoted to a double on its way through the
     * ellipsis. A count below zero is no times.
     **/
    static std::vector<float> times(va_list arguments, RtInt n);

    /**
     * An RtMatrix is sixteen floats in RI's row major order under a row vector convention.
     * glm stores column major under a column vector one. Reading them in order is the change
     * of convention, and a transpose would undo it.
     **/
    static glm::mat4x4 matrix(RtMatrix transform);

    /**
     * The image size RiFormat names, from its resolution in x and y. A side the RIB reader
     * would refuse is refused here too, and the request is skipped.
     **/
    static std::optional<v3d::render::offline::rib::Format> format(const glm::ivec2 & resolution, RtFloat aspect);

    /**
     * The filter RiPixelFilter names. The C interface names a filter by its function, and only
     * the filters RI declares are known. Any other function, or a width the RIB reader would
     * refuse, skips the request.
     **/
    static std::optional<v3d::render::offline::film::Filter> filter(RtFilterFunc filterfunc, RtFloat xwidth, RtFloat ywidth);

    /**
     * A parameter list, typed by what the active handler has declared.
     *
     * @param vertices how many vertices the request's primitive has, which bounds a varying or a
     *        vertex array. A request that is not geometry has one, because a uniform value is one
     *        element whatever it is attached to
     **/
    v3d::render::offline::rib::ParameterList parameters(RtInt n, RtToken tokens[], RtPointer parms[],
        unsigned int vertices = 1) const;

    /**
     * RiPointsPolygons' faces. A vertex or varying value is one element per point the faces
     * index, so the arrays are as long as the largest index plus one. A count or an index below
     * zero skips the request.
     *
     * @param points set to the number of points the faces index
     **/
    static std::optional<v3d::render::offline::rib::PointsPolygons> pointsPolygons(RtInt npolys, const RtInt nverts[],
        const RtInt verts[], unsigned int* points);

    /**
     * RiCurves' kind and the number of control points of each curve. A kind other than linear
     * or cubic, a wrap other than periodic or nonperiodic, or a count below zero skips the
     * request.
     *
     * @param total set to the number of control points the curves have in all
     **/
    std::optional<v3d::render::offline::rib::Curves> curves(RtToken type, RtInt nvcurves, const RtInt nvertices[],
        RtToken wrap, unsigned int* total) const;

    /**
     * RiCurves' parameter list. Only "P" and "constantwidth" are read. How many values a
     * varying parameter holds depends on the basis's step, and the arrays here carry no
     * length, so any other parameter is skipped rather than read past.
     *
     * @param vertices how many control points the curves have in all
     **/
    v3d::render::offline::rib::ParameterList curveParameters(RtInt n, RtToken tokens[], RtPointer parms[],
        unsigned int vertices) const;

    /**
     * RiBasis' two matrices and steps. RI's own order is row by row, and a Basis holds a matrix
     * the same way. A step outside 1 to 4 is not one a basis has, and the request is dropped.
     **/
    std::optional<v3d::render::offline::rib::Basis> basis(RtBasis ubasis, RtInt ustep, RtBasis vbasis, RtInt vstep) const;

 private:
    Renderer* renderer_;
};

};  // namespace v3d::moya::ri::internal
