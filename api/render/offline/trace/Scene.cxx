/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Scene.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/mat3x3.hpp>
#include <glm/matrix.hpp>

namespace v3d::render::offline::trace {

namespace {

const float PI = 3.14159265358979323846f;

/**
 * The plane of a triangle, wound the way its corners are. A triangle with no area names no
 * plane and answers zero rather than a normalised nothing.
 **/
glm::vec3 plane(const glm::vec3 & a, const glm::vec3 & b, const glm::vec3 & c) {
    const glm::vec3 across = glm::cross(b - a, c - a);
    const float area = glm::length(across);
    return area > 1.0e-8f ? across / area : glm::vec3(0.0f);
}

/**
 * The nearest primitive a ray has met so far, and where on it.
 **/
class Nearest final {
 public:
    float distance = std::numeric_limits<float>::max();
    const Triangle* triangle = nullptr;
    const Sphere* sphere = nullptr;
    /** A triangle's barycentric weights, or the point on a sphere in its own space. **/
    float u = 0.0f;
    float v = 0.0f;
    glm::vec3 point = glm::vec3(0.0f);
};

/**
 * Where each motion has carried its primitives by a time from where they are stored, and the
 * way back.
 **/
class Poses final {
 public:
    std::vector<glm::mat4x4> ahead;
    std::vector<glm::mat4x4> backward;
};

void meetTriangles(const std::vector<Triangle> & triangles, const v3d::type::geometry::Ray & ray, float from,
    const Poses & poses, Nearest* found) {
    for (const Triangle & triangle : triangles) {
        const int motion = triangle.motion();
        const v3d::type::geometry::Ray local = motion < 0 ? ray :
            v3d::type::geometry::Ray(glm::vec3(poses.backward[motion] * glm::vec4(ray.origin(), 1.0f)),
                glm::mat3(poses.backward[motion]) * ray.direction());
        float distance = 0.0f;
        float u = 0.0f;
        float v = 0.0f;
        if (!local.intersects(triangle.a(), triangle.b(), triangle.c(), &distance, &u, &v)) {
            continue;
        }
        if (motion >= 0) {
            // a distance along the ray taken back is in the stored pose's units, which a
            // motion that scales does not keep
            const glm::vec3 there(poses.ahead[motion] * glm::vec4(local.origin() + local.direction() * distance, 1.0f));
            distance = glm::dot(there - ray.origin(), ray.direction());
        }
        if (distance <= from || distance >= found->distance) {
            continue;
        }
        found->distance = distance;
        found->triangle = &triangle;
        found->sphere = nullptr;
        found->u = u;
        found->v = v;
    }
}

void meetSpheres(const std::vector<Sphere> & spheres, const v3d::type::geometry::Ray & ray, float from,
    const Poses & poses, Nearest* found) {
    for (const Sphere & sphere : spheres) {
        const int motion = sphere.motion();
        // taken back unnormalised, so the parameter along it is still the ray's distance
        const glm::vec3 origin = motion < 0 ? ray.origin() :
            glm::vec3(poses.backward[motion] * glm::vec4(ray.origin(), 1.0f));
        const glm::vec3 direction = motion < 0 ? ray.direction() : glm::mat3(poses.backward[motion]) * ray.direction();
        float distance = 0.0f;
        glm::vec3 point(0.0f);
        if (!sphere.intersects(origin, direction, from, &distance, &point) || distance >= found->distance) {
            continue;
        }
        found->distance = distance;
        found->triangle = nullptr;
        found->sphere = &sphere;
        found->point = point;
    }
}

/**
 * Everything a shader reads at the nearest hit, with a moving primitive's normals carried
 * forward to the time its hit was found at.
 **/
void fill(const Nearest & found, const v3d::type::geometry::Ray & ray, const Poses & poses, Hit* hit) {
    hit->distance = found.distance;
    hit->point = ray.origin() + ray.direction() * found.distance;
    if (found.triangle != nullptr) {
        hit->primitive = found.triangle;
        hit->normal = found.triangle->shadingNormal(found.u, found.v);
        hit->geometric = found.triangle->geometricNormal();
        hit->u = found.u;
        hit->v = found.v;
        const glm::vec2 st = found.triangle->st(found.u, found.v);
        hit->s = st.x;
        hit->t = st.y;
    } else {
        hit->primitive = found.sphere;
        hit->normal = found.sphere->normal(found.point);
        hit->geometric = hit->normal;
        const glm::vec2 parameters = found.sphere->parameters(found.point);
        hit->u = parameters.x;
        hit->v = parameters.y;
        hit->s = parameters.x;
        hit->t = parameters.y;
    }
    const int motion = hit->primitive->motion();
    if (motion >= 0) {
        const glm::mat3 normals = glm::transpose(glm::inverse(glm::mat3(poses.ahead[motion])));
        hit->normal = glm::normalize(normals * hit->normal);
        if (glm::length(hit->geometric) > 0.0f) {
            hit->geometric = glm::normalize(normals * hit->geometric);
        }
    }
    hit->incident = ray.direction();
}

};  // namespace

Primitive::Primitive(const glm::vec3 & colour) : colour_(colour) {
}

const glm::vec3 & Primitive::colour() const {
    return colour_;
}

int Primitive::motion() const {
    return motion_;
}

const v3d::render::offline::sl::Placed & Primitive::surface() const {
    return surface_;
}

void Primitive::surface(const v3d::render::offline::sl::Placed & shader) {
    surface_ = shader;
}

const glm::vec3 & Primitive::opacity() const {
    return opacity_;
}

const Lights & Primitive::lights() const {
    return lights_;
}

void Primitive::lights(const Lights & lit) {
    lights_ = lit;
}

void Primitive::opacity(const glm::vec3 & value) {
    opacity_ = value;
}

Triangle::Triangle(const glm::vec3 & a, const glm::vec3 & b, const glm::vec3 & c, const glm::vec3 & colour) :
    Primitive(colour), a_(a), b_(b), c_(c), geometric_(plane(a, b, c)) {
    na_ = geometric_;
    nb_ = geometric_;
    nc_ = geometric_;
}

Triangle::Triangle(const glm::vec3 & a, const glm::vec3 & b, const glm::vec3 & c, const glm::vec3 & colour,
    const glm::vec3 & na, const glm::vec3 & nb, const glm::vec3 & nc) :
    Primitive(colour), a_(a), b_(b), c_(c), na_(na), nb_(nb), nc_(nc), geometric_(plane(a, b, c)) {
}

const glm::vec3 & Triangle::a() const {
    return a_;
}

const glm::vec3 & Triangle::b() const {
    return b_;
}

const glm::vec3 & Triangle::c() const {
    return c_;
}

const glm::vec3 & Triangle::geometricNormal() const {
    return geometric_;
}

glm::vec3 Triangle::shadingNormal(float u, float v) const {
    const glm::vec3 normal = na_ * (1.0f - u - v) + nb_ * u + nc_ * v;
    // interpolating unit normals does not give a unit one back, and three corners whose
    // normals cancel give none at all - the plane is what is left to answer with
    const float length = glm::length(normal);
    return length > 0.0f ? normal / length : geometric_;
}

void Triangle::st(const glm::vec2 & a, const glm::vec2 & b, const glm::vec2 & c) {
    sta_ = a;
    stb_ = b;
    stc_ = c;
}

glm::vec2 Triangle::st(float u, float v) const {
    return sta_ * (1.0f - u - v) + stb_ * u + stc_ * v;
}

Sphere::Sphere(float radius, float zmin, float zmax, float thetamax, const glm::mat4x4 & placement,
    const glm::vec3 & colour) :
    Primitive(colour), radius_(radius),
    zmin_(std::clamp(std::min(zmin, zmax), -radius, radius)),
    zmax_(std::clamp(std::max(zmin, zmax), -radius, radius)),
    thetamax_(std::clamp(thetamax, 0.0f, 360.0f) * PI / 180.0f),
    toObject_(glm::inverse(placement)) {
}

bool Sphere::intersects(const glm::vec3 & origin, const glm::vec3 & direction, float from,
    float* along, glm::vec3* point) const {
    if (!(radius_ > 0.0f)) {
        return false;
    }
    // an affine map keeps a line's parameter, so a root here is a distance in the caller's
    // units without being carried back
    const glm::vec3 o(toObject_ * glm::vec4(origin, 1.0f));
    const glm::vec3 d = glm::mat3(toObject_) * direction;
    const float a = glm::dot(d, d);
    const float b = 2.0f * glm::dot(o, d);
    const float c = glm::dot(o, o) - radius_ * radius_;
    const float discriminant = b * b - 4.0f * a * c;
    if (a <= 0.0f || discriminant < 0.0f) {
        return false;
    }
    const float root = std::sqrt(discriminant);
    // the nearer root first, and the far one when the near is behind or cut away: a ray
    // from inside, or one through the open top of a cut sphere, meets the inside
    const float roots[2] = { (-b - root) / (2.0f * a), (-b + root) / (2.0f * a) };
    for (const float t : roots) {
        if (t <= from) {
            continue;
        }
        const glm::vec3 p = o + d * t;
        if (p.z < zmin_ || p.z > zmax_) {
            continue;
        }
        float phi = std::atan2(p.y, p.x);
        if (phi < 0.0f) {
            phi += 2.0f * PI;
        }
        if (phi > thetamax_) {
            continue;
        }
        *along = t;
        *point = p;
        return true;
    }
    return false;
}

glm::vec3 Sphere::normal(const glm::vec3 & point) const {
    const glm::mat3 normals = glm::transpose(glm::mat3(toObject_));
    return glm::normalize(normals * (point / radius_));
}

glm::vec2 Sphere::parameters(const glm::vec3 & point) const {
    float phi = std::atan2(point.y, point.x);
    if (phi < 0.0f) {
        phi += 2.0f * PI;
    }
    const float low = std::asin(zmin_ / radius_);
    const float high = std::asin(zmax_ / radius_);
    const float latitude = std::asin(std::clamp(point.z / radius_, -1.0f, 1.0f));
    return glm::vec2(thetamax_ > 0.0f ? phi / thetamax_ : 0.0f,
        high > low ? (latitude - low) / (high - low) : 0.0f);
}

Scene::Scene() {
}

v3d::type::camera::Camera & Scene::camera() {
    return camera_;
}

const v3d::type::camera::Camera & Scene::camera() const {
    return camera_;
}

int Scene::motion(const v3d::render::offline::MovingTransform & placed) {
    if (!placed.moving()) {
        return -1;
    }
    // the pieces of one primitive share a motion, and so do the next primitive's when
    // nothing has been placed in between
    if (motions_.empty() || motions_.back().open() != placed.open() || motions_.back().close() != placed.close() ||
        motions_.back().times() != placed.times()) {
        motions_.push_back(placed);
    }
    return static_cast<int>(motions_.size()) - 1;
}

void Scene::add(const Triangle & triangle) {
    triangles_.push_back(triangle);
}

void Scene::add(const Triangle & triangle, const v3d::render::offline::MovingTransform & placed) {
    Triangle moving(triangle);
    moving.motion_ = motion(placed);
    triangles_.push_back(moving);
}

const std::vector<Triangle> & Scene::triangles() const {
    return triangles_;
}

void Scene::add(const Sphere & sphere, const v3d::render::offline::MovingTransform & placed) {
    Sphere moving(sphere);
    moving.motion_ = motion(placed);
    spheres_.push_back(moving);
}

const std::vector<Sphere> & Scene::spheres() const {
    return spheres_;
}

void Scene::add(const v3d::render::offline::sl::Placed & light) {
    lights_.push_back(light);
}

const std::vector<v3d::render::offline::sl::Placed> & Scene::lights() const {
    return lights_;
}

bool Scene::nearest(const v3d::type::geometry::Ray & ray, float from, Hit* hit, float time) const {
    Poses poses;
    poses.ahead.resize(motions_.size());
    poses.backward.resize(motions_.size());
    for (std::size_t i = 0; i < motions_.size(); i++) {
        poses.ahead[i] = motions_[i].at(time) * glm::inverse(motions_[i].open());
        poses.backward[i] = glm::inverse(poses.ahead[i]);
    }

    Nearest found;
    meetTriangles(triangles_, ray, from, poses, &found);
    meetSpheres(spheres_, ray, from, poses, &found);
    const bool met = found.triangle != nullptr || found.sphere != nullptr;
    if (met && hit != nullptr) {
        fill(found, ray, poses, hit);
    }
    return met;
}

const glm::vec3 & Scene::background() const {
    return background_;
}

void Scene::background(const glm::vec3 & colour) {
    background_ = colour;
}

unsigned int Scene::traceDepth() const {
    return traceDepth_;
}

void Scene::traceDepth(unsigned int depth) {
    traceDepth_ = depth;
}

};  // namespace v3d::render::offline::trace
