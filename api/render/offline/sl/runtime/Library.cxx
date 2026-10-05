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

#include <api/render/offline/Noise.h>
#include <api/render/offline/Texture.h>
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

typedef Signature::Body Body;

const float PI = 3.14159265358979323846f;

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
            // RI's mod takes the sign of the divisor rather than of the dividend, so that
            // mod(-1, 3) is 2 and a value decreasing past zero stays inside the period
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
 * A direction turned to lie on the same side of the surface as the reference does, so a
 * shader can handle a surface facing away without knowing which way it faces.
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
    glm::mat4x4 matrix = glm::mat4x4(1.0f);
    /** The image texture() reads, and where: its own arguments, or the shader's s and t. **/
    const Texture* texture = nullptr;
    const Value* s = nullptr;
    const Value* t = nullptr;

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
    const float k = 1.0f - eta * eta * (1.0f - cosine * cosine);
    float reflected = 1.0f;
    if (k > 0.0f) {
        const float through = std::sqrt(k);
        const float across = (eta * cosine - through) / (eta * cosine + through);
        const float along = (cosine - eta * through) / (cosine + eta * through);
        reflected = 0.5f * (across * across + along * along);
    }
    const std::vector<Value*> & outputs = *site.outputs;
    outputs[0]->number(point, reflected);
    outputs[1]->number(point, 1.0f - reflected);
    if (outputs.size() == 4) {
        outputs[2]->triple(point, incident - 2.0f * glm::dot(incident, normal) * normal);
        outputs[3]->triple(point, refract(incident, normal, eta));
    }
}

/**
 * Where three components of noise are read from, so that a colour of noise is three patterns
 * rather than one grey one. The offsets are far apart and off the lattice.
 **/
const float STREAMS[3][3] = {
    { 0.0f, 0.0f, 0.0f }, { 31.416f, 47.853f, 12.793f }, { -73.218f, 9.631f, 58.437f }
};

void pattern(const Site & site, unsigned int point) {
    if (site.body == Body::TEXTURE) {
        const float s = site.s == nullptr ? 0.0f : site.s->number(point);
        const float t = site.t == nullptr ? 0.0f : site.t->number(point);
        const glm::vec3 colour = site.texture == nullptr ? glm::vec3(0.0f) : site.texture->sample(s, t);
        if (site.target->components() == 1) {
            site.target->number(point, colour.r);
        } else {
            site.target->triple(point, colour);
        }
        return;
    }
    // one float is a line through the noise and two are a plane of it
    glm::vec3 at(0.0f);
    if (site.count() == 1 && site.argument(0).components() == 3) {
        at = site.argument(0).triple(point);
    } else {
        at.x = site.argument(0).number(point);
        at.y = site.count() > 1 ? site.argument(1).number(point) : 0.0f;
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
 * The group where every component of the result is the same function of that component of
 * each argument. A one component argument is read for all of them, as RI promotes it,
 * rather than a zero fill.
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
            // a conversion with nothing left to print is reported rather than reading past
            // the arguments, which would crash the render
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
 * Whether the body writes the argument it was given rather than returning a value. This
 * decides which register the mask is checked against and where the result goes.
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
    for (int reg : instruction.arguments) {
        given.push_back(&file_[static_cast<std::size_t>(reg)]);
    }
    site.given = &given;
    site.target = &answer;
    if (body == Body::TRANSMISSION || body == Body::TRACE) {
        shadowed(body == Body::TRACE, *given[0], *given[1], &answer);
        return;
    }
    site.written = setter(body) ? &file_[static_cast<std::size_t>(instruction.arguments[0])] : site.target;
    std::vector<Value*> outputs = written(instruction, table[index].outputs);
    if (!outputs.empty()) {
        // the mask is checked against the first: the compiler gives every one the
        // same storage
        site.written = outputs.front();
        site.outputs = &outputs;
    }

    // a named space is one matrix for the whole batch, since a string is always uniform, so
    // this is not a lookup per point
    if (transforming(body)) {
        site.matrix = space(given[0]->text());
        if (given.size() == 3) {
            // "from" and "to": out of the space the value is in, and into the other
            site.matrix = space(given[1]->text()) * glm::inverse(site.matrix);
        }
    } else if (body == Body::DEPTH) {
        site.matrix = space("NDC");
    } else if (body == Body::TEXTURE) {
        site.texture = texture(given, &site.s, &site.t);
    } else if (body == Body::CTRANSFORM && given[0]->text() != "rgb") {
        // there is one colour space here and it is the one a framebuffer holds; a scene
        // naming another gets its colours back unchanged
        report("the colour space \"" + given[0]->text() + "\" is not one this renderer knows");
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

const Texture* Machine::texture(const std::vector<const Value*> & given, const Value** s, const Value** t) {
    if (given.size() == 3) {
        *s = given[1];
        *t = given[2];
    } else {
        *s = s_ < 0 ? nullptr : &file_[static_cast<std::size_t>(s_)];
        *t = t_ < 0 ? nullptr : &file_[static_cast<std::size_t>(t_)];
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
            target->triple(point, glm::vec3(0.0f));
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
        if (!renderer_->light(index, surface, &direction_, &colour_, &reached, &isAmbient)) {
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
        The result when the renderer cannot do it. All the light gets through, so a scene
        renders unshadowed rather than not at all, and a ray returns black rather than
        something plausible.
    */
    const unsigned int wide = target->storage() == Storage::VARYING ? batch_ : 1;
    const float answer = ray ? 0.0f : 1.0f;
    for (unsigned int point = 0; point < wide; point++) {
        if (writable(*target, point)) {
            target->triple(point, glm::vec3(answer));
        }
    }
    report(ray ? "'trace' is not answered by this renderer, so a ray comes back black"
        : "'transmission' has nothing to cast a shadow here, so all the light gets through");
}

};  // namespace v3d::render::offline::sl::runtime
