/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

/*
    The bodies behind the signatures in sl/Builtins.h: what a CALL instruction does.

    These are Machine member functions, in a second translation unit. Every body here reads
    the live mask, writes the register file and reports what it could not do, and all three
    are Machine's state. Control flow and masking are in Machine.cxx.
*/

#include "Instruction.h"
#include "Machine.h"
#include "Renderer.h"
#include "Value.h"

#include <api/render/offline/Noise.h>
#include <api/render/offline/Texture.h>
#include <api/render/offline/Units.h>
#include <api/render/offline/sl/Builtins.h>
#include <api/render/offline/sl/Types.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/vec3.hpp>

namespace v3d::render::offline::sl::runtime {

namespace {

typedef Signature::Body Body;

float sign(float value) {
    if (value < 0.0F) {
        return -1.0F;
    }
    return value > 0.0F ? 1.0F : 0.0F;
}

/** The bounds a value is clamped to. **/
struct Range {
    float low = 0.0F;
    float high = 1.0F;
};

/**
 * A value held to a range. Unlike std::clamp, a range whose low end is above its high end is
 * not an error: a shader may pass one, and the low end wins.
 **/
float clamp(float value, const Range & range) {
    if (value < range.low) {
        return range.low;
    }
    return value > range.high ? range.high : value;
}

/**
 * Hermite interpolation between two edges, with the edges first and the value last.
 **/
float smoothstep(float low, float high, float value) {
    if (high == low) {
        return value < low ? 0.0F : 1.0F;
    }
    // 3t^2 - 2t^3, as t^2 (3 - 2t)
    constexpr float squareCoefficient = 3.0F;
    constexpr float cubeCoefficient = 2.0F;
    const float t = clamp((value - low) / (high - low), {.low = 0.0F, .high = 1.0F});
    return t * t * (squareCoefficient - (cubeCoefficient * t));
}

/**
 * The functions of one argument. Each applies alike to every component of its argument.
 **/
float once(Body body, float x) {
    switch (body) {
        case Body::ABS:
            return std::fabs(x);
        case Body::SIGN:
            return sign(x);
        case Body::FLOOR:
            return std::floor(x);
        case Body::CEIL:
            return std::ceil(x);
        case Body::ROUND:
            return std::floor(x + 0.5F);
        case Body::SQRT:
            // the root of a negative is not a number, and a shader that took one should
            // render black rather than render nothing at all
            return x <= 0.0F ? 0.0F : std::sqrt(x);
        case Body::EXP:
            return std::exp(x);
        case Body::LOG:
            return std::log(x);
        case Body::RADIANS:
            return x * PI / HALF_TURN_DEGREES;
        case Body::DEGREES:
            return x * HALF_TURN_DEGREES / PI;
        case Body::SIN:
            return std::sin(x);
        case Body::COS:
            return std::cos(x);
        case Body::TAN:
            return std::tan(x);
        case Body::ASIN:
            return std::asin(clamp(x, {.low = -1.0F, .high = 1.0F}));
        case Body::ACOS:
            return std::acos(clamp(x, {.low = -1.0F, .high = 1.0F}));
        default:
            return std::atan(x);
    }
}

/**
 * One component of the result, from the same component of each argument.
 **/
float number(Body body, const float* given, std::size_t count) {
    switch (body) {
        case Body::LOG:
            // the two argument form is the logarithm to a base: the ratio of two natural logarithms
            return count == 2 ? std::log(given[0]) / std::log(given[1]) : once(body, given[0]);
        case Body::ATAN:
            return count == 2 ? std::atan2(given[0], given[1]) : once(body, given[0]);
        case Body::MOD:
            // RI's mod takes the sign of the divisor, not of the dividend. mod(-1, 3) is 2, so
            // a value decreasing past zero stays inside the period
            return given[1] == 0.0F ? 0.0F : given[0] - (given[1] * std::floor(given[0] / given[1]));
        case Body::POW:
            return std::pow(given[0], given[1]);
        case Body::MIN:
            return given[0] < given[1] ? given[0] : given[1];
        case Body::MAX:
            return given[0] > given[1] ? given[0] : given[1];
        case Body::CLAMP:
            return clamp(given[0], {.low = given[1], .high = given[2]});
        case Body::MIX:
            return given[0] + ((given[1] - given[0]) * given[2]);
        case Body::STEP:
            return given[1] < given[0] ? 0.0F : 1.0F;
        case Body::SMOOTHSTEP:
            return smoothstep(given[0], given[1], given[2]);
        default:
            return once(body, given[0]);
    }
}

glm::vec3 unit(const glm::vec3 & value) {
    // glm::normalize of a zero direction is not a number, and a shader that normalized one
    // should render black rather than render nothing at all
    const float length = glm::length(value);
    return length == 0.0F ? value : value / length;
}

/**
 * A direction turned to lie on the same side of the surface as the reference does, so a
 * shader can handle a surface facing away without knowing which way it faces.
 **/
glm::vec3 faceforward(const glm::vec3 & normal, const glm::vec3 & incident,
    const glm::vec3 & reference) {
    return glm::dot(-incident, reference) < 0.0F ? -normal : normal;
}

glm::vec3 refract(const glm::vec3 & incident, const glm::vec3 & normal, float eta) {
    const float cosine = glm::dot(incident, normal);
    const float discriminant = 1.0F - (eta * eta * (1.0F - (cosine * cosine)));
    // a negative discriminant is total internal reflection, where nothing is refracted
    return discriminant < 0.0F ? glm::vec3(0.0F) : eta * incident - ((eta * cosine) + std::sqrt(discriminant)) * normal;
}

/**
 * Everything one call needs, gathered once for the batch. Only the point index changes from
 * one shading point to the next.
 **/
class Site final {
 public:
    Body body = Body::STUB;
    /** Where the result goes. **/
    Value* target = nullptr;
    /**
     * The value written: the result for every body but a setter. `setxcomp` writes the
     * argument it was given and returns nothing.
     **/
    Value* written = nullptr;
    const std::vector<const Value*>* given = nullptr;
    /** The arguments a body returns results through, for one that writes more than one. **/
    const std::vector<Value*>* outputs = nullptr;
    /** The matrix of a named coordinate space, for the bodies that take one. **/
    glm::mat4x4 matrix = glm::mat4x4(1.0F);
    /** Whether a point through the matrix is divided by w: a space it names is projected. **/
    bool projected = false;
    /** The image texture() reads, and where: its own arguments, or the shader's s and t. **/
    const Texture* texture = nullptr;
    const Value* s = nullptr;
    const Value* t = nullptr;
    /** How the batch lies on the surface, for the derivatives. **/
    Renderer::Shape shape;

    const Value & argument(std::size_t which) const {
        return *(*given)[which];
    }

    std::size_t count() const {
        return given->size();
    }
};

/**
 * The unpolarised reflectance of a dielectric, the mean of its two polarisations, and the
 * reflected and refracted directions with it.
 *
 * The incident direction and the normal are normalised first, and the normal is taken to
 * face against the incident direction, as refract() takes it. Past the critical angle
 * everything is reflected and the refracted direction is zero, as refract() returns it.
 **/
void fresnel(const Site & site, unsigned int point) {
    const glm::vec3 incident = unit(site.argument(0).triple(point));
    const glm::vec3 normal = unit(site.argument(1).triple(point));
    const float eta = site.argument(2).number(point);
    const float cosine = std::fabs(glm::dot(incident, normal));
    const float discriminant = 1.0F - (eta * eta * (1.0F - (cosine * cosine)));
    float reflected = 1.0F;
    if (discriminant > 0.0F) {
        const float through = std::sqrt(discriminant);
        const float across = ((eta * cosine) - through) / ((eta * cosine) + through);
        const float along = (cosine - (eta * through)) / (cosine + (eta * through));
        reflected = 0.5F * ((across * across) + (along * along));
    }
    const std::vector<Value*> & outputs = *site.outputs;
    outputs[0]->number(point, reflected);
    outputs[1]->number(point, 1.0F - reflected);
    if (outputs.size() == 4) {
        outputs[2]->triple(point, incident - 2.0F * glm::dot(incident, normal) * normal);
        outputs[3]->triple(point, refract(incident, normal, eta));
    }
}

/**
 * The offsets each of three noise components is read at, so that a colour of noise is three
 * patterns and not one grey one. The offsets are far apart and off the lattice.
 **/
const float STREAMS[3][3] = {
    { 0.0F, 0.0F, 0.0F }, { 31.416F, 47.853F, 12.793F }, { -73.218F, 9.631F, 58.437F }
};

void pattern(const Site & site, unsigned int point) {
    if (site.body == Body::TEXTURE) {
        const float s = site.s == nullptr ? 0.0F : site.s->number(point);
        const float t = site.t == nullptr ? 0.0F : site.t->number(point);
        const glm::vec3 colour = site.texture == nullptr ? glm::vec3(0.0F) : site.texture->sample(s, t);
        if (site.target->components() == 1) {
            site.target->number(point, colour.r);
        } else {
            site.target->triple(point, colour);
        }
        return;
    }
    // one float is a line through the noise and two are a plane of it
    glm::vec3 at(0.0F);
    if (site.count() == 1 && site.argument(0).components() == 3) {
        at = site.argument(0).triple(point);
    } else {
        at.x = site.argument(0).number(point);
        at.y = site.count() > 1 ? site.argument(1).number(point) : 0.0F;
    }
    for (unsigned int i = 0; i < site.target->components() && i < 3; i++) {
        const glm::vec3 stream(STREAMS[i][0], STREAMS[i][1], STREAMS[i][2]);
        site.target->component(point, i, offline::noise(at + stream));
    }
}

void geometry(const Site & site, unsigned int point) {
    switch (site.body) {
        case Body::LENGTH:
            site.target->number(point, glm::length(site.argument(0).triple(point)));
            return;
        case Body::DISTANCE:
            site.target->number(point,
                glm::length(site.argument(0).triple(point) - site.argument(1).triple(point)));
            return;
        case Body::NORMALIZE:
            site.target->triple(point, unit(site.argument(0).triple(point)));
            return;
        case Body::FACEFORWARD: {
            // two arguments means the reference is the normal itself, for a shader that
            // has no Ng
            const glm::vec3 normal = site.argument(0).triple(point);
            const glm::vec3 reference = site.count() == 3 ? site.argument(2).triple(point) : normal;
            site.target->triple(point, faceforward(normal, site.argument(1).triple(point), reference));
            return;
        }
        case Body::REFLECT: {
            const glm::vec3 incident = site.argument(0).triple(point);
            const glm::vec3 normal = site.argument(1).triple(point);
            site.target->triple(point, incident - 2.0F * glm::dot(incident, normal) * normal);
            return;
        }
        default:
            site.target->triple(point, refract(site.argument(0).triple(point),
                site.argument(1).triple(point), site.argument(2).number(point)));
            return;
    }
}

void component(const Site & site, unsigned int point) {
    // xcomp, ycomp and zcomp are consecutive and so are their setters, so the offset from
    // the first gives the component index
    switch (site.body) {
        case Body::XCOMP:
        case Body::YCOMP:
        case Body::ZCOMP:
            site.target->number(point, site.argument(0).component(point,
                static_cast<unsigned int>(site.body) - static_cast<unsigned int>(Body::XCOMP)));
            return;
        case Body::SETXCOMP:
        case Body::SETYCOMP:
        case Body::SETZCOMP:
            site.written->component(point,
                static_cast<unsigned int>(site.body) - static_cast<unsigned int>(Body::SETXCOMP),
                site.argument(1).number(point));
            return;
        case Body::COMP:
            site.target->number(point, site.argument(0).component(point,
                static_cast<unsigned int>(site.argument(1).number(point))));
            return;
        default:
            site.written->component(point,
                static_cast<unsigned int>(site.argument(1).number(point)),
                site.argument(2).number(point));
            return;
    }
}

/**
 * The built-ins that work through a named space. A cast to a space compiles to the TRANSFORM
 * instruction instead, which Machine::transform() runs.
 **/
void spaces(const Site & site, unsigned int point) {
    // the value is the last argument in every form: a space name or two comes first
    const Value & value = site.argument(site.count() - 1);
    switch (site.body) {
        case Body::PTRANSFORM:
            site.target->triple(point, site.projected ? sl::project(site.matrix, value.triple(point)) :
                sl::ptransform(site.matrix, value.triple(point)));
            return;
        case Body::VTRANSFORM:
            site.target->triple(point, sl::vtransform(site.matrix, value.triple(point)));
            return;
        case Body::NTRANSFORM:
            site.target->triple(point, sl::ntransform(site.matrix, value.triple(point)));
            return;
        case Body::MTRANSFORM:
            site.target->matrix(point, site.matrix * value.matrix(point));
            return;
        case Body::DEPTH:
            site.target->number(point, sl::project(site.matrix, value.triple(point)).z);
            return;
        default:
            // there is one colour space here and it is the one a framebuffer holds
            site.target->triple(point, value.triple(point));
            return;
    }
}

void matrices(const Site & site, unsigned int point) {
    switch (site.body) {
        case Body::DETERMINANT:
            site.target->number(point, glm::determinant(site.argument(0).matrix(point)));
            return;
        // glm::translate(m, t) is m times a translation, which a column vector meets first.
        // RenderMan's translate(m, t) also applies t before m, as ConcatTransform does, and
        // rotate and scale follow the same rule
        case Body::TRANSLATE:
            site.target->matrix(point,
                glm::translate(site.argument(0).matrix(point), site.argument(1).triple(point)));
            return;
        case Body::ROTATE:
            site.target->matrix(point, glm::rotate(site.argument(0).matrix(point),
                site.argument(1).number(point), site.argument(2).triple(point)));
            return;
        default:
            site.target->matrix(point,
                glm::scale(site.argument(0).matrix(point), site.argument(1).triple(point)));
            return;
    }
}

/**
 * The built-ins where each component of the result is the same function of that component of
 * each argument. A one-component argument is read for every component, as RI promotes a
 * float. The missing components are not read as zero.
 **/
void componentwise(const Site & site, unsigned int point) {
    float given[3] = { 0.0F, 0.0F, 0.0F };
    const std::size_t count = site.count() < 3 ? site.count() : 3;
    for (unsigned int i = 0; i < site.target->components(); i++) {
        for (std::size_t which = 0; which < count; which++) {
            const Value & argument = site.argument(which);
            given[which] = argument.component(point, argument.components() == 1 ? 0 : i);
        }
        site.target->component(point, i, number(site.body, given, site.count()));
    }
}

/** Along u or along v, for a derivative. **/
enum class Direction : std::uint8_t { U, V };

/**
 * The two points a derivative at a point differences, and the step between them, or false
 * where the shape gives the point no neighbour that way. On a grid the neighbour is the next
 * point, or the previous one on the last row or column.
 **/
bool neighbours(const Renderer::Shape & shape, unsigned int point, Direction direction,
    unsigned int* from, unsigned int* to, float* step) {
    const bool alongU = direction == Direction::U;
    *step = alongU ? shape.du : shape.dv;
    if (!(*step > 0.0F)) {
        return false;
    }
    switch (shape.layout) {
        case Renderer::Shape::Layout::STENCIL:
            *from = 0;
            *to = alongU ? 1U : 2U;
            return true;
        case Renderer::Shape::Layout::GRID: {
            const unsigned int across = shape.across == 0 ? 1U : shape.across;
            const unsigned int index = alongU ? point / across : point % across;
            const unsigned int count = alongU ? shape.along : across;
            const unsigned int stride = alongU ? across : 1U;
            if (count < 2) {
                return false;
            }
            *from = index + 1 < count ? point : point - stride;
            *to = *from + stride;
            return true;
        }
        case Renderer::Shape::Layout::LONE:
            return false;
    }
    return false;
}

/**
 * One component of a value's derivative along u or v at a point. A uniform value is the same
 * at every point, so its derivative is zero.
 **/
float slope(const Value & value, unsigned int component, const Renderer::Shape & shape, unsigned int point,
    Direction direction) {
    unsigned int from = 0;
    unsigned int to = 0;
    float step = 0.0F;
    if (value.storage() != Storage::VARYING || !neighbours(shape, point, direction, &from, &to, &step)) {
        return 0.0F;
    }
    return (value.component(to, component) - value.component(from, component)) / step;
}

/** A position's derivative along u or v at a point. **/
glm::vec3 slope(const Value & value, const Renderer::Shape & shape, unsigned int point, Direction direction) {
    return { slope(value, 0, shape, point, direction), slope(value, 1, shape, point, direction),
        slope(value, 2, shape, point, direction) };
}

/**
 * Du, Dv, Deriv, area and calculatenormal.
 *
 * Deriv(num, den) is RI's Du(num) / Du(den) + Dv(num) / Dv(den), with a term dropped where
 * den does not change in that direction. area(P) is the area of the patch a step along u and
 * a step along v span. calculatenormal(P) is Du(P) ^ Dv(P), which on a diced polygon points
 * the way its Ng does.
 **/
void derivatives(const Site & site, unsigned int point) {
    const Value & value = site.argument(0);
    switch (site.body) {
        case Body::DU:
        case Body::DV: {
            const Direction direction = site.body == Body::DU ? Direction::U : Direction::V;
            for (unsigned int i = 0; i < site.target->components(); i++) {
                site.target->component(point, i, slope(value, i, site.shape, point, direction));
            }
            return;
        }
        case Body::DERIV: {
            const float byU = slope(site.argument(1), 0, site.shape, point, Direction::U);
            const float byV = slope(site.argument(1), 0, site.shape, point, Direction::V);
            for (unsigned int i = 0; i < site.target->components(); i++) {
                float rate = 0.0F;
                if (byU != 0.0F) {
                    rate += slope(value, i, site.shape, point, Direction::U) / byU;
                }
                if (byV != 0.0F) {
                    rate += slope(value, i, site.shape, point, Direction::V) / byV;
                }
                site.target->component(point, i, rate);
            }
            return;
        }
        case Body::AREA: {
            const glm::vec3 u = slope(value, site.shape, point, Direction::U) * site.shape.du;
            const glm::vec3 v = slope(value, site.shape, point, Direction::V) * site.shape.dv;
            site.target->number(point, glm::length(glm::cross(u, v)));
            return;
        }
        default:
            site.target->triple(point, glm::cross(slope(value, site.shape, point, Direction::U),
                slope(value, site.shape, point, Direction::V)));
            return;
    }
}

void apply(const Site & site, unsigned int point) {
    switch (site.body) {
        case Body::DU:
        case Body::DV:
        case Body::DERIV:
        case Body::AREA:
        case Body::CALCULATENORMAL:
            derivatives(site, point);
            return;
        case Body::LENGTH:
        case Body::DISTANCE:
        case Body::NORMALIZE:
        case Body::FACEFORWARD:
        case Body::REFLECT:
        case Body::REFRACT:
            geometry(site, point);
            return;
        case Body::FRESNEL:
            fresnel(site, point);
            return;
        case Body::TEXTURE:
        case Body::NOISE:
            pattern(site, point);
            return;
        case Body::XCOMP:
        case Body::YCOMP:
        case Body::ZCOMP:
        case Body::SETXCOMP:
        case Body::SETYCOMP:
        case Body::SETZCOMP:
        case Body::COMP:
        case Body::SETCOMP:
            component(site, point);
            return;
        case Body::PTRANSFORM:
        case Body::VTRANSFORM:
        case Body::NTRANSFORM:
        case Body::CTRANSFORM:
        case Body::MTRANSFORM:
        case Body::DEPTH:
            spaces(site, point);
            return;
        case Body::DETERMINANT:
        case Body::TRANSLATE:
        case Body::ROTATE:
        case Body::SCALE:
            matrices(site, point);
            return;
        default:
            componentwise(site, point);
            return;
    }
}

/**
 * How a printf conversion prints its value. SL's conversions name types, not C's formats: %c
 * is a colour, %p a point, %v a vector, %n a normal and %m a matrix.
 **/
std::string printed(char conversion, const Value & value, unsigned int point) {
    if (conversion == 's') {
        return value.text();
    }
    unsigned int wide = 1;
    if (conversion == 'c' || conversion == 'p' || conversion == 'v' || conversion == 'n') {
        wide = 3;
    } else if (conversion == 'm') {
        wide = 16;
    }
    std::string text;
    for (unsigned int i = 0; i < wide; i++) {
        if (i > 0) {
            text += " ";
        }
        text += std::to_string(value.component(point, i));
    }
    return wide == 1 ? text : "(" + text + ")";
}

std::string format(const std::vector<const Value*> & given, unsigned int point) {
    const std::string & text = given[0]->text();
    std::string line;
    std::size_t next = 1;
    for (std::size_t i = 0; i < text.size(); i++) {
        if (text[i] != '%' || i + 1 == text.size()) {
            line += text[i];
            continue;
        }
        const char conversion = text[++i];
        if (conversion == '%') {
            line += '%';
        } else if (next < given.size()) {
            line += printed(conversion, *given[next++], point);
        } else {
            // a conversion with no argument left prints a marker, and nothing reads past the
            // end of the arguments
            line += "(missing)";
        }
    }
    return line;
}

/** Whether the body differences neighbouring shading points. **/
bool derivative(Body body) {
    return body == Body::DU || body == Body::DV || body == Body::DERIV || body == Body::AREA ||
        body == Body::CALCULATENORMAL;
}

/** Whether the body works through the matrix of a named coordinate space. **/
bool transforming(Body body) {
    return body == Body::PTRANSFORM || body == Body::VTRANSFORM ||
        body == Body::NTRANSFORM || body == Body::MTRANSFORM;
}

};  // namespace

glm::mat4x4 Machine::between(const std::vector<const Value*> & given, bool* divided) {
    glm::mat4x4 matrix = space(given[0]->text());
    *divided = projected(given[0]->text());
    if (given.size() == 3) {
        // "from" and "to": out of the space the value is in, and into the other. Either end
        // being projected puts a w other than one into the point
        matrix = space(given[1]->text()) * glm::inverse(matrix);
        *divided = *divided || projected(given[1]->text());
    }
    return matrix;
}

void Machine::colourSpaces(const std::vector<const Value*> & given) {
    // the only colour space is rgb, the one a framebuffer holds. A scene naming another gets
    // its colours back unchanged. Every argument before the colour names a space: the one to
    // convert into, or the one to convert from and then the one to convert into
    for (std::size_t which = 0; which + 1 < given.size(); which++) {
        if (given[which]->text() != "rgb") {
            report("the colour space \"" + given[which]->text() + "\" is not one this renderer knows");
        }
    }
}

void Machine::builtin(const Instruction & instruction) {
    const std::vector<Signature> & table = builtins();
    const std::size_t index = static_cast<std::size_t>(instruction.left);
    if (instruction.left < 0 || index >= table.size()) {
        report("a call names no standard library function");
        return;
    }
    const Body body = table[index].body;
    if (body == Body::STUB || body == Body::SOURCE) {
        report("'" + table[index].name + "' is declared and does nothing yet, so it answers its default");
        return;
    }
    Value & answer = file_[static_cast<std::size_t>(instruction.target)];
    if (body == Body::AMBIENT) {
        ambient(&answer);
        return;
    }
    if (instruction.arguments.empty()) {
        return;
    }

    Site site;
    site.body = body;
    std::vector<const Value*> given;
    given.reserve(instruction.arguments.size());
    for (const int reg : instruction.arguments) {
        given.push_back(&file_[static_cast<std::size_t>(reg)]);
    }
    site.given = &given;
    site.target = &answer;
    if (body == Body::TRANSMISSION || body == Body::TRACE) {
        shadowed(body == Body::TRACE, *given[0], *given[1], &answer);
        return;
    }
    // a function that changes an argument in place writes that register, not its result.
    // The mask is checked against that register
    const int updates = table[index].updates;
    site.written = updates >= 0 && static_cast<std::size_t>(updates) < instruction.arguments.size() ?
        &file_[static_cast<std::size_t>(instruction.arguments[static_cast<std::size_t>(updates)])] : site.target;
    std::vector<Value*> outputs = written(instruction, table[index].outputs);
    if (!outputs.empty()) {
        // the mask is checked against the first: the compiler gives every one the
        // same storage
        site.written = outputs.front();
        site.outputs = &outputs;
    }

    // a space name is a string, and a string is always uniform, so the matrix is looked up
    // once for the whole batch
    if (transforming(body)) {
        site.matrix = between(given, &site.projected);
    } else if (body == Body::DEPTH) {
        site.matrix = space("NDC");
    } else if (derivative(body) && renderer_ != nullptr) {
        site.shape = renderer_->shape();
    } else if (body == Body::TEXTURE) {
        Coordinates coordinates;
        site.texture = texture(given, &coordinates);
        site.s = coordinates.s;
        site.t = coordinates.t;
    } else if (body == Body::CTRANSFORM) {
        colourSpaces(given);
    }

    const unsigned int count = site.written->storage() == Storage::VARYING ? batch_ : 1;
    for (unsigned int point = 0; point < count; point++) {
        if (!writable(*site.written, point)) {
            continue;
        }
        if (body == Body::PRINTF) {
            // a line per shading point, not deduplicated as the machine's reports are
            printed_.push_back(format(given, point));
            continue;
        }
        apply(site, point);
    }
}

std::vector<Value*> Machine::written(const Instruction & instruction, int first) {
    std::vector<Value*> outputs;
    if (first < 0) {
        return outputs;
    }
    for (std::size_t which = static_cast<std::size_t>(first); which < instruction.arguments.size(); which++) {
        outputs.push_back(&file_[static_cast<std::size_t>(instruction.arguments[which])]);
    }
    return outputs;
}

const Texture* Machine::texture(const std::vector<const Value*> & given, Coordinates* coordinates) {
    if (given.size() == 3) {
        coordinates->s = given[1];
        coordinates->t = given[2];
    } else {
        coordinates->s = s_ < 0 ? nullptr : &file_[static_cast<std::size_t>(s_)];
        coordinates->t = t_ < 0 ? nullptr : &file_[static_cast<std::size_t>(t_)];
    }
    // a name is uniform, so the image is found once for the batch
    const std::string & name = given[0]->text();
    const Texture* found = renderer_ == nullptr ? nullptr : renderer_->texture(name);
    if (found == nullptr) {
        report("the texture \"" + name + "\" cannot be read, so it answers black");
    }
    return found;
}

void Machine::ambient(Value* target) {
    const unsigned int wide = target->storage() == Storage::VARYING ? batch_ : 1;
    for (unsigned int point = 0; point < wide; point++) {
        if (writable(*target, point)) {
            target->triple(point, glm::vec3(0.0F));
        }
    }
    const unsigned int count = renderer_ == nullptr ? 0 : renderer_->lights();
    if (count == 0) {
        return;
    }
    const Value & surface = point_ >= 0 ? file_[static_cast<std::size_t>(point_)] : direction_;
    for (unsigned int index = 0; index < count; index++) {
        std::vector<char> reached(batch_, 1);
        bool isAmbient = false;
        if (!renderer_->light(index, surface, Cone(), &direction_, &colour_, &reached, &isAmbient)) {
            continue;
        }
        if (!isAmbient) {
            // a light with a direction is an illuminance loop's, not ambient()'s
            continue;
        }
        for (unsigned int point = 0; point < wide; point++) {
            if (reached[point] != 0 && writable(*target, point)) {
                target->triple(point, target->triple(point) + colour_.triple(point));
            }
        }
    }
}

void Machine::shadowed(bool ray, const Value & from, const Value & to, Value* target) {
    if (renderer_ != nullptr &&
        (ray ? renderer_->trace(from, to, target) : renderer_->transmission(from, to, target))) {
        return;
    }
    /*
        The result when the renderer cannot answer. transmission() lets all the light
        through, so the scene renders unshadowed. trace() returns black.
    */
    const unsigned int wide = target->storage() == Storage::VARYING ? batch_ : 1;
    const float answer = ray ? 0.0F : 1.0F;
    for (unsigned int point = 0; point < wide; point++) {
        if (writable(*target, point)) {
            target->triple(point, glm::vec3(answer));
        }
    }
    report(ray ? "'trace' is not answered by this renderer, so a ray comes back black"
        : "'transmission' has nothing to cast a shadow here, so all the light gets through");
}

};  // namespace v3d::render::offline::sl::runtime
