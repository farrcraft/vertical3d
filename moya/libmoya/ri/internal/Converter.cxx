/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Converter.h"

#include <api/render/offline/Sampling.h>
#include <api/render/offline/rib/Arguments.h>
#include <api/render/offline/rib/Parameters.h>
#include <api/render/offline/rib/Requests.h>
#include <moya/libmoya/ri/RIBHandler.h>
#include <moya/libmoya/ri/RenderMan.h>
#include <moya/libmoya/ri/Renderer.h>
#include <moya/libmoya/state/RenderContext.h>

#include <algorithm>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>

namespace v3d::moya::ri::internal {

Converter::Converter(Renderer & renderer) : renderer_(&renderer) {
}

std::string Converter::text(const char* token) {
    return token != RI_NULL ? std::string(token) : std::string();
}

void Converter::collect(va_list arguments, std::vector<RtToken>* tokens, std::vector<RtPointer>* parms) {
    for (;;) {
        RtToken token = va_arg(arguments, RtToken);
        if (token == RI_NULL) {
            return;
        }
        tokens->push_back(token);
        parms->push_back(va_arg(arguments, RtPointer));
    }
}

std::vector<float> Converter::times(va_list arguments, RtInt n) {
    std::vector<float> times;
    times.reserve(static_cast<std::size_t>(std::max(n, 0)));
    for (RtInt i = 0; i < n; i++) {
        times.push_back(static_cast<float>(va_arg(arguments, double)));
    }
    return times;
}

glm::mat4x4 Converter::matrix(RtMatrix transform) {
    return glm::make_mat4(&transform[0][0]);
}

std::optional<v3d::render::offline::rib::Format> Converter::format(const glm::ivec2 & resolution, RtFloat aspect) {
    const std::optional<uint32_t> width = v3d::render::offline::formatSide(static_cast<float>(resolution.x));
    const std::optional<uint32_t> height = v3d::render::offline::formatSide(static_cast<float>(resolution.y));
    if (!width || !height) {
        return std::nullopt;
    }
    return v3d::render::offline::rib::Format{.width = *width, .height = *height, .pixelAspect = aspect};
}

std::optional<v3d::render::offline::Filter> Converter::filter(RtFilterFunc filterfunc, RtFloat xwidth, RtFloat ywidth) {
    v3d::render::offline::Filter filter = v3d::render::offline::Filter::Gaussian;
    if (filterfunc == RiBoxFilter) {
        filter = v3d::render::offline::Filter::Box;
    } else if (filterfunc == RiTriangleFilter) {
        filter = v3d::render::offline::Filter::Triangle;
    } else if (filterfunc == RiCatmullRomFilter) {
        filter = v3d::render::offline::Filter::CatmullRom;
    } else if (filterfunc == RiGaussianFilter) {
        filter = v3d::render::offline::Filter::Gaussian;
    } else if (filterfunc == RiSincFilter) {
        filter = v3d::render::offline::Filter::Sinc;
    } else {
        return std::nullopt;
    }
    if (!v3d::render::offline::filterWidth(glm::vec2(xwidth, ywidth))) {
        return std::nullopt;
    }
    return filter;
}

v3d::render::offline::rib::ParameterList Converter::parameters(RtInt n, RtToken tokens[], RtPointer parms[],
    unsigned int vertices) const {
    RIBHandler & active = renderer_->active();
    v3d::render::offline::rib::ParameterList list;
    std::vector<std::string> unresolved;
    v3d::render::offline::rib::arguments(active.declarations(), n, tokens, parms, vertices, &list,
        &unresolved);
    for (const std::string & name : unresolved) {
        // a name with no declaration has no length either, so it is dropped rather than
        // read past, as the RIB reader does for a file
        active.context().logger()->get()->warn("RI parameter '{}' was not declared and was skipped", name);
    }
    return list;
}

std::optional<v3d::render::offline::rib::PointsPolygons> Converter::pointsPolygons(RtInt npolys, const RtInt nverts[],
    const RtInt verts[], unsigned int* points) {
    v3d::render::offline::rib::PointsPolygons request;
    std::size_t total = 0;
    for (RtInt each = 0; each < npolys; each++) {
        if (nverts[each] < 0) {
            return std::nullopt;
        }
        request.counts.push_back(static_cast<unsigned int>(nverts[each]));
        total += static_cast<std::size_t>(nverts[each]);
    }
    *points = 0;
    for (std::size_t each = 0; each < total; each++) {
        if (verts[each] < 0) {
            return std::nullopt;
        }
        request.indices.push_back(static_cast<unsigned int>(verts[each]));
        *points = std::max(*points, request.indices.back() + 1);
    }
    return request;
}

std::optional<v3d::render::offline::rib::Curves> Converter::curves(RtToken type, RtInt nvcurves, const RtInt nvertices[],
    RtToken wrap, unsigned int* total) const {
    const std::string kind = text(type);
    const std::string wrapping = text(wrap);
    if ((kind != "linear" && kind != "cubic") || (wrapping != "periodic" && wrapping != "nonperiodic")) {
        renderer_->active().context().logger()->get()->warn("RiCurves takes linear or cubic and periodic or nonperiodic, not {} and {}",
            kind, wrapping);
        return std::nullopt;
    }
    v3d::render::offline::rib::Curves request;
    request.cubic = kind == "cubic";
    request.periodic = wrapping == "periodic";
    *total = 0;
    for (RtInt each = 0; each < nvcurves; each++) {
        if (nvertices[each] < 0) {
            return std::nullopt;
        }
        request.vertices.push_back(static_cast<unsigned int>(nvertices[each]));
        *total += static_cast<unsigned int>(nvertices[each]);
    }
    return request;
}

v3d::render::offline::rib::ParameterList Converter::curveParameters(RtInt n, RtToken tokens[], RtPointer parms[],
    unsigned int vertices) const {
    std::vector<RtToken> kept;
    std::vector<RtPointer> values;
    for (RtInt each = 0; each < n; each++) {
        const std::string name = text(tokens[each]);
        if (name == "P" || name == "constantwidth") {
            kept.push_back(tokens[each]);
            values.push_back(parms[each]);
        } else {
            renderer_->active().context().logger()->get()->warn("RiCurves reads \"P\" and \"constantwidth\", and skips '{}'", name);
        }
    }
    return parameters(static_cast<RtInt>(kept.size()), kept.data(), values.data(), vertices);
}

std::optional<v3d::render::offline::rib::Basis> Converter::basis(RtBasis ubasis, RtInt ustep, RtBasis vbasis, RtInt vstep) const {
    if (ustep < 1 || ustep > 4 || vstep < 1 || vstep > 4) {
        renderer_->active().context().logger()->get()->warn("RiBasis steps by {} and {}, and a basis steps by 1 to 4", ustep, vstep);
        return std::nullopt;
    }
    v3d::render::offline::rib::Basis request;
    for (std::size_t row = 0; row < 4; row++) {
        for (std::size_t column = 0; column < 4; column++) {
            request.u[(row * 4) + column] = ubasis[row][column];
            request.v[(row * 4) + column] = vbasis[row][column];
        }
    }
    request.ustep = static_cast<unsigned int>(ustep);
    request.vstep = static_cast<unsigned int>(vstep);
    return request;
}

};  // namespace v3d::moya::ri::internal
