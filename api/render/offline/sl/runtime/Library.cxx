/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

/*
    The bodies behind the signatures in sl/Builtins.h: what a CALL instruction does.

    A second translation unit for Machine rather than a class of its own, because every
    body here reads the live mask, writes the register file and says what it could not do,
    and all three of those are the machine's own state. What is here is most of the language
    by volume and almost none of it by mechanism.
*/

#include "Machine.h"

#include <api/render/offline/sl/Builtins.h>
#include <api/render/offline/sl/Types.h>

#include <cmath>
#include <string>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/vec3.hpp>

namespace v3d::render::offline::sl::runtime {

namespace {

/**
 * Which body a call runs.
 *
 * A name rather than the index of the signature that matched, because two signatures of
 * one name differ only in how many arguments they take - `atan(y, x)` and `atan(x)` are one
 * body that asks how many it was given.
 **/
enum class Body {
    /** Declared, and answering its default until something implements it. **/
    NONE,
    // every component of the answer is this function of the same component of each
    // argument, which is what makes abs() of a colour the three absolute values
    ABS, SIGN, FLOOR, CEIL, ROUND, SQRT, EXP, LOG, RADIANS, DEGREES,
    SIN, COS, TAN, ASIN, ACOS, ATAN, MOD, POW, MIN, MAX, CLAMP, MIX, STEP, SMOOTHSTEP,
    // a triple read as a direction rather than as three numbers
    LENGTH, DISTANCE, NORMALIZE, FACEFORWARD, REFLECT, REFRACT,
    // one component of one value, named or indexed
    XCOMP, YCOMP, ZCOMP, SETXCOMP, SETYCOMP, SETZCOMP, COMP, SETCOMP,
    // a named coordinate space, which is the renderer's answer rather than the machine's
    PTRANSFORM, VTRANSFORM, NTRANSFORM, CTRANSFORM, MTRANSFORM, DEPTH,
    // a matrix
    DETERMINANT, TRANSLATE, ROTATE, SCALE,
    PRINTF
};

const float PI = 3.14159265358979323846f;

Body lookup(const std::string & name) {
    static const struct { const char* name; Body body; } table[] = {
        { "abs", Body::ABS }, { "sign", Body::SIGN }, { "floor", Body::FLOOR },
        { "ceil", Body::CEIL }, { "round", Body::ROUND }, { "sqrt", Body::SQRT },
        { "exp", Body::EXP }, { "log", Body::LOG }, { "radians", Body::RADIANS },
        { "degrees", Body::DEGREES }, { "sin", Body::SIN }, { "cos", Body::COS },
        { "tan", Body::TAN }, { "asin", Body::ASIN }, { "acos", Body::ACOS },
        { "atan", Body::ATAN }, { "mod", Body::MOD }, { "pow", Body::POW },
        { "min", Body::MIN }, { "max", Body::MAX }, { "clamp", Body::CLAMP },
        { "mix", Body::MIX }, { "step", Body::STEP }, { "smoothstep", Body::SMOOTHSTEP },
        { "length", Body::LENGTH }, { "distance", Body::DISTANCE },
        { "normalize", Body::NORMALIZE }, { "faceforward", Body::FACEFORWARD },
        { "reflect", Body::REFLECT }, { "refract", Body::REFRACT },
        { "xcomp", Body::XCOMP }, { "ycomp", Body::YCOMP }, { "zcomp", Body::ZCOMP },
        { "setxcomp", Body::SETXCOMP }, { "setycomp", Body::SETYCOMP },
        { "setzcomp", Body::SETZCOMP }, { "comp", Body::COMP }, { "setcomp", Body::SETCOMP },
        { "ptransform", Body::PTRANSFORM }, { "vtransform", Body::VTRANSFORM },
        { "ntransform", Body::NTRANSFORM }, { "ctransform", Body::CTRANSFORM },
        { "mtransform", Body::MTRANSFORM }, { "depth", Body::DEPTH },
        { "determinant", Body::DETERMINANT }, { "translate", Body::TRANSLATE },
        { "rotate", Body::ROTATE }, { "scale", Body::SCALE }, { "printf", Body::PRINTF }
    };
    for (const auto & entry : table) {
        if (name == entry.name) {
            return entry.body;
        }
    }
    return Body::NONE;
}

float sign(float value) {
    if (value < 0.0f) {
        return -1.0f;
    }
    return value > 0.0f ? 1.0f : 0.0f;
}

float clamp(float value, float low, float high) {
    if (value < low) {
        return low;
    }
    return value > high ? high : value;
}

/**
 * The Hermite between two edges rather than the linear one, with the edges first and the
 * value last.
 **/
float smoothstep(float low, float high, float value) {
    if (high == low) {
        return value < low ? 0.0f : 1.0f;
    }
    const float t = clamp((value - low) / (high - low), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

/**
 * The maths of one argument, which is most of the table and is the same function of every
 * component of what it was given.
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
            return std::floor(x + 0.5f);
        case Body::SQRT:
            // the root of a negative is not a number, and a shader that took one should
            // render black rather than render nothing at all
            return x <= 0.0f ? 0.0f : std::sqrt(x);
        case Body::EXP:
            return std::exp(x);
        case Body::LOG:
            return std::log(x);
        case Body::RADIANS:
            return x * PI / 180.0f;
        case Body::DEGREES:
            return x * 180.0f / PI;
        case Body::SIN:
            return std::sin(x);
        case Body::COS:
            return std::cos(x);
        case Body::TAN:
            return std::tan(x);
        case Body::ASIN:
            return std::asin(clamp(x, -1.0f, 1.0f));
        case Body::ACOS:
            return std::acos(clamp(x, -1.0f, 1.0f));
        default:
            return std::atan(x);
    }
}

/**
 * One component of the answer, out of the same component of each argument.
 **/
float number(Body body, const float* given, std::size_t count) {
    switch (body) {
        case Body::LOG:
            // the two argument form is the logarithm to a base, which is the ratio of two
            return count == 2 ? std::log(given[0]) / std::log(given[1]) : once(body, given[0]);
        case Body::ATAN:
            return count == 2 ? std::atan2(given[0], given[1]) : once(body, given[0]);
        case Body::MOD:
            // RI's mod takes the sign of the divisor rather than of the dividend, so that
            // mod(-1, 3) is 2 and a value walked backwards round a period stays in it
            return given[1] == 0.0f ? 0.0f : given[0] - given[1] * std::floor(given[0] / given[1]);
        case Body::POW:
            return std::pow(given[0], given[1]);
        case Body::MIN:
            return given[0] < given[1] ? given[0] : given[1];
        case Body::MAX:
            return given[0] > given[1] ? given[0] : given[1];
        case Body::CLAMP:
            return clamp(given[0], given[1], given[2]);
        case Body::MIX:
            return given[0] + (given[1] - given[0]) * given[2];
        case Body::STEP:
            return given[1] < given[0] ? 0.0f : 1.0f;
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
    return length == 0.0f ? value : value / length;
}

/**
 * A direction turned to lie on the same side of the surface as the reference does, which is
 * how a shader answers a surface facing away without knowing which way it faces.
 **/
glm::vec3 faceforward(const glm::vec3 & normal, const glm::vec3 & incident,
    const glm::vec3 & reference) {
    return glm::dot(-incident, reference) < 0.0f ? -normal : normal;
}

glm::vec3 refract(const glm::vec3 & incident, const glm::vec3 & normal, float eta) {
    const float cosine = glm::dot(incident, normal);
    const float k = 1.0f - eta * eta * (1.0f - cosine * cosine);
    // a negative discriminant is total internal reflection, where nothing is refracted
    return k < 0.0f ? glm::vec3(0.0f) : eta * incident - (eta * cosine + std::sqrt(k)) * normal;
}

/**
 * Everything one call needs at one shading point. A class rather than eight arguments,
 * since the only thing that changes between two points of a batch is which point it is.
 **/
class Site final {
 public:
    Body body = Body::NONE;
    /** Where the answer goes. **/
    Value* target = nullptr;
    /**
     * The value written, which is the answer for every body but a setter - `setxcomp`
     * writes the argument it was handed and answers nothing.
     **/
    Value* written = nullptr;
    const std::vector<const Value*>* given = nullptr;
    /** The matrix a named coordinate space came to, for the bodies that take one. **/
    glm::mat4x4 matrix = glm::mat4x4(1.0f);

    const Value & argument(std::size_t which) const {
        return *(*given)[which];
    }

    std::size_t count() const {
        return given->size();
    }
};

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
            // two arguments means the reference is the normal itself, which is what a
            // shader with no Ng to hand asks for
            const glm::vec3 normal = site.argument(0).triple(point);
            const glm::vec3 reference = site.count() == 3 ? site.argument(2).triple(point) : normal;
            site.target->triple(point, faceforward(normal, site.argument(1).triple(point), reference));
            return;
        }
        case Body::REFLECT: {
            const glm::vec3 incident = site.argument(0).triple(point);
            const glm::vec3 normal = site.argument(1).triple(point);
            site.target->triple(point, incident - 2.0f * glm::dot(incident, normal) * normal);
            return;
        }
        default:
            site.target->triple(point, refract(site.argument(0).triple(point),
                site.argument(1).triple(point), site.argument(2).number(point)));
            return;
    }
}

void component(const Site & site, unsigned int point) {
    // xcomp, ycomp and zcomp are consecutive and so are their setters, which is what makes
    // the name the index
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
 * The group that works through a named space. Named for the space rather than for the
 * transform, since Machine::transform is the instruction the cast form compiles to.
 **/
void spaces(const Site & site, unsigned int point) {
    // the value is the last argument in every form: a space name or two comes first
    const Value & value = site.argument(site.count() - 1);
    switch (site.body) {
        case Body::PTRANSFORM:
            site.target->triple(point, sl::ptransform(site.matrix, value.triple(point)));
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
            site.target->number(point, sl::ptransform(site.matrix, value.triple(point)).z);
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
 * The group where every component of the answer is the same function of that component of
 * each argument, and a one component argument is read for all of them - which is RI's
 * promotion rather than a zero fill.
 **/
void componentwise(const Site & site, unsigned int point) {
    float given[3] = { 0.0f, 0.0f, 0.0f };
    const std::size_t count = site.count() < 3 ? site.count() : 3;
    for (unsigned int i = 0; i < site.target->components(); i++) {
        for (std::size_t which = 0; which < count; which++) {
            const Value & argument = site.argument(which);
            given[which] = argument.component(point, argument.components() == 1 ? 0 : i);
        }
        site.target->component(point, i, number(site.body, given, site.count()));
    }
}

void apply(const Site & site, unsigned int point) {
    switch (site.body) {
        case Body::LENGTH:
        case Body::DISTANCE:
        case Body::NORMALIZE:
        case Body::FACEFORWARD:
        case Body::REFLECT:
        case Body::REFRACT:
            geometry(site, point);
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
 * How a printf conversion prints its value. SL's are the types rather than C's widths: %c
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
            // a conversion with nothing left to print says so rather than reading past the
            // arguments, which is the one printf mistake that would otherwise take a render down
            line += "(missing)";
        }
    }
    return line;
}

/** Whether the body works through the matrix of a named coordinate space. **/
bool transforming(Body body) {
    return body == Body::PTRANSFORM || body == Body::VTRANSFORM ||
        body == Body::NTRANSFORM || body == Body::MTRANSFORM;
}

/**
 * Whether the body writes the argument it was handed rather than answering a value, which
 * decides both which register the mask is asked about and where the answer goes.
 **/
bool setter(Body body) {
    return body == Body::SETXCOMP || body == Body::SETYCOMP ||
        body == Body::SETZCOMP || body == Body::SETCOMP;
}

};  // namespace

void Machine::builtin(const Instruction & instruction) {
    const std::vector<Signature> & table = builtins();
    const std::size_t index = static_cast<std::size_t>(instruction.left);
    if (instruction.left < 0 || index >= table.size()) {
        report("a call names no standard library function");
        return;
    }
    const Body body = lookup(table[index].name);
    if (body == Body::NONE) {
        report("'" + table[index].name + "' is declared and does nothing yet, so it answers its default");
        return;
    }
    if (instruction.arguments.empty()) {
        return;
    }

    Site site;
    site.body = body;
    std::vector<const Value*> given;
    given.reserve(instruction.arguments.size());
    for (int reg : instruction.arguments) {
        given.push_back(&file_[static_cast<std::size_t>(reg)]);
    }
    site.given = &given;
    site.target = &file_[static_cast<std::size_t>(instruction.target)];
    site.written = setter(body) ? &file_[static_cast<std::size_t>(instruction.arguments[0])] : site.target;

    // a named space is one matrix for the whole batch, since a string is uniform - which is
    // the reason a string may be uniform only, and the reason this is not a lookup per point
    if (transforming(body)) {
        site.matrix = space(given[0]->text());
        if (given.size() == 3) {
            // "from" and "to": out of the space the value is in, and into the other
            site.matrix = space(given[1]->text()) * glm::inverse(site.matrix);
        }
    } else if (body == Body::DEPTH) {
        site.matrix = space("NDC");
    } else if (body == Body::CTRANSFORM && given[0]->text() != "rgb") {
        // there is one colour space here and it is the one a framebuffer holds; a scene
        // asking for another gets its colours back unchanged rather than wrong
        report("the colour space \"" + given[0]->text() + "\" is not one this renderer knows");
    }

    const unsigned int count = site.written->storage() == Storage::VARYING ? batch_ : 1;
    for (unsigned int point = 0; point < count; point++) {
        if (!writable(*site.written, point)) {
            continue;
        }
        if (body == Body::PRINTF) {
            // a line per shading point, because a person who wrote a printf asked to be told
            // every time rather than once
            printed_.push_back(format(given, point));
            continue;
        }
        apply(site, point);
    }
}

};  // namespace v3d::render::offline::sl::runtime
