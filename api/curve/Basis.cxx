/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Basis.h"

#include <cstddef>
#include <type_traits>
#include <variant>
#include <vector>

#include "Curve.h"
#include "Segment.h"
#include "Spline.h"

#include <glm/vec3.hpp>

namespace v3d::curve {

namespace {

constexpr float THIRD = 1.0F / 3.0F;

/**
 * The highest order whose spans a Bezier segment matches exactly.
 **/
constexpr unsigned int CUBIC_ORDER = 4;

BasisCurve convert(const Poly& poly, bool closed, unsigned int /*pieces*/) {
    return { .basis = Basis::Linear, .periodic = closed, .points = poly.points };
}

BasisCurve convert(const Bezier& bezier, bool closed, unsigned int /*pieces*/) {
    BasisCurve basis{ .basis = Basis::Bezier, .periodic = closed, .points = {} };
    const std::size_t count = bezier.points.size();
    for (std::size_t each = 0; each < count; each++) {
        const BezierPoint& point = bezier.points[each];
        if (each > 0) {
            basis.points.push_back(point.left);
        }
        basis.points.push_back(point.position);
        if (closed || each + 1 < count) {
            basis.points.push_back(point.right);
        }
    }
    if (closed) {
        basis.points.push_back(bezier.points.front().left);
    }
    return basis;
}

BasisCurve convert(const CatmullRom& catmullRom, bool closed, unsigned int /*pieces*/) {
    const std::vector<glm::vec3>& from = catmullRom.points;
    BasisCurve basis{ .basis = Basis::CatmullRom, .periodic = closed, .points = {} };
    // A basis curve's first segment runs from its second point, so the point before the first
    // leads the list: a copy of the first on an open curve, and the last on a closed one.
    basis.points.push_back(closed ? from.back() : from.front());
    basis.points.insert(basis.points.end(), from.begin(), closed ? from.end() - 1 : from.end());
    if (!closed) {
        basis.points.push_back(from.back());
    }
    return basis;
}

BasisCurve convert(const Nurbs& nurbs, bool closed, unsigned int pieces) {
    const Spline spline(nurbs, closed);
    const std::vector<std::size_t> spans = spline.spans();
    if (spline.degree() == 1) {
        BasisCurve basis{ .basis = Basis::Linear, .periodic = closed, .points = {} };
        for (const std::size_t span : spans) {
            basis.points.push_back(spline.point(span, spline.knot(span)));
        }
        if (!closed) {
            basis.points.push_back(spline.point(spans.back(), spline.knot(spans.back() + 1)));
        }
        return basis;
    }

    // A cubic piece that matches the curve's position and derivative at both of its ends is the
    // curve itself wherever the curve is a polynomial of degree 3 or less.
    const unsigned int split = (spline.polynomial() && nurbs.order <= CUBIC_ORDER) ? 1 : pieces;
    BasisCurve basis{ .basis = Basis::Bezier, .periodic = closed, .points = {} };
    basis.points.push_back(spline.point(spans.front(), spline.knot(spans.front())));
    for (const std::size_t span : spans) {
        const float start = spline.knot(span);
        const float length = (spline.knot(span + 1) - start) / static_cast<float>(split);
        for (unsigned int piece = 0; piece < split; piece++) {
            const float from = start + (length * static_cast<float>(piece));
            const float to = from + length;
            const glm::vec3 end = spline.point(span, to);
            basis.points.push_back(spline.point(span, from) + (spline.derivative(span, from) * (length * THIRD)));
            basis.points.push_back(end - (spline.derivative(span, to) * (length * THIRD)));
            basis.points.push_back(end);
        }
    }
    if (closed) {
        // the last segment ends on the first point, which a periodic curve does not repeat
        basis.points.pop_back();
    }
    return basis;
}

};  // namespace

BasisCurve toBasis(const Curve& curve, unsigned int pieces) {
    return std::visit([&](const auto& shape) { return convert(shape, curve.closed, pieces); }, curve.shape);
}

bool exact(const Curve& curve) {
    const Nurbs* nurbs = std::get_if<Nurbs>(&curve.shape);
    if (nurbs == nullptr || nurbs->order == 2) {
        return true;
    }
    return nurbs->order <= CUBIC_ORDER && Spline(*nurbs, curve.closed).polynomial();
}

std::vector<glm::vec3> evaluate(const BasisCurve& curve, unsigned int resolution) {
    const std::vector<glm::vec3>& from = curve.points;
    const std::size_t count = from.size();
    auto at = [&](std::size_t index) { return from[index % count]; };
    auto step = [&](unsigned int piece) { return static_cast<float>(piece) / static_cast<float>(resolution); };

    std::vector<glm::vec3> points;
    switch (curve.basis) {
        case Basis::Linear:
            points = from;
            if (curve.periodic) {
                points.push_back(from.front());
            }
            break;
        case Basis::Bezier: {
            const std::size_t segments = curve.periodic ? count / 3 : (count - 1) / 3;
            points.push_back(from.front());
            for (std::size_t segment = 0; segment < segments; segment++) {
                const std::size_t first = segment * 3;
                for (unsigned int piece = 1; piece <= resolution; piece++) {
                    points.push_back(bezierAt(at(first), at(first + 1), at(first + 2), at(first + 3), step(piece)));
                }
            }
            break;
        }
        case Basis::CatmullRom: {
            const std::size_t segments = curve.periodic ? count : count - 3;
            points.push_back(from[1]);
            for (std::size_t segment = 0; segment < segments; segment++) {
                for (unsigned int piece = 1; piece <= resolution; piece++) {
                    points.push_back(catmullRomAt(at(segment), at(segment + 1), at(segment + 2), at(segment + 3), step(piece)));
                }
            }
            break;
        }
    }
    return points;
}

};  // namespace v3d::curve
