/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "CreatePoly.h"

#include <cmath>
#include <vector>

#include <boost/make_shared.hpp>
#include <glm/gtc/constants.hpp>

namespace v3d::editor {

namespace {

/**
 * How many segments a round primitive is approximated by.
 **/
const unsigned int sides = 8;

/**
 * A ring of points about the y axis, at a given height.
 **/
std::vector<glm::vec3> ring(float radius, float height) {
    const float delta = glm::two_pi<float>() / static_cast<float>(sides);
    std::vector<glm::vec3> points;
    for (unsigned int index = 0; index < sides; index++) {
        const float angle = delta * static_cast<float>(index);
        points.push_back(glm::vec3(std::cos(angle) * radius, height, std::sin(angle) * radius));
    }
    return points;
}

};  // namespace

/**
 **/
boost::shared_ptr<v3d::brep::BRep> create_poly_cube() {
    const float half = 0.5f;
    boost::shared_ptr<v3d::brep::BRep> mesh = boost::make_shared<v3d::brep::BRep>();

    std::vector<glm::vec3> points;

    // front
    points.push_back(glm::vec3(-half, -half, -half));
    points.push_back(glm::vec3(half, -half, -half));
    points.push_back(glm::vec3(half, half, -half));
    points.push_back(glm::vec3(-half, half, -half));
    mesh->addFace(points, glm::vec3(0.0f, 0.0f, -1.0f));

    // right
    points.clear();
    points.push_back(glm::vec3(half, -half, -half));
    points.push_back(glm::vec3(half, -half, half));
    points.push_back(glm::vec3(half, half, half));
    points.push_back(glm::vec3(half, half, -half));
    mesh->addFace(points, glm::vec3(1.0f, 0.0f, 0.0f));

    // top
    points.clear();
    points.push_back(glm::vec3(-half, half, -half));
    points.push_back(glm::vec3(half, half, -half));
    points.push_back(glm::vec3(half, half, half));
    points.push_back(glm::vec3(-half, half, half));
    mesh->addFace(points, glm::vec3(0.0f, 1.0f, 0.0f));

    // left
    points.clear();
    points.push_back(glm::vec3(-half, -half, half));
    points.push_back(glm::vec3(-half, -half, -half));
    points.push_back(glm::vec3(-half, half, -half));
    points.push_back(glm::vec3(-half, half, half));
    mesh->addFace(points, glm::vec3(-1.0f, 0.0f, 0.0f));

    // back
    points.clear();
    points.push_back(glm::vec3(half, -half, half));
    points.push_back(glm::vec3(-half, -half, half));
    points.push_back(glm::vec3(-half, half, half));
    points.push_back(glm::vec3(half, half, half));
    mesh->addFace(points, glm::vec3(0.0f, 0.0f, 1.0f));

    // bottom
    points.clear();
    points.push_back(glm::vec3(half, -half, -half));
    points.push_back(glm::vec3(-half, -half, -half));
    points.push_back(glm::vec3(-half, -half, half));
    points.push_back(glm::vec3(half, -half, half));
    mesh->addFace(points, glm::vec3(0.0f, -1.0f, 0.0f));

    return mesh;
}

/**
 **/
boost::shared_ptr<v3d::brep::BRep> create_poly_plane() {
    const float half = 0.5f;
    boost::shared_ptr<v3d::brep::BRep> mesh = boost::make_shared<v3d::brep::BRep>();

    std::vector<glm::vec3> vertices;
    vertices.push_back(glm::vec3(-half, 0.0f, -half));
    vertices.push_back(glm::vec3(half, 0.0f, -half));
    vertices.push_back(glm::vec3(half, 0.0f, half));
    vertices.push_back(glm::vec3(-half, 0.0f, half));
    mesh->addFace(vertices, glm::vec3(0.0f, 1.0f, 0.0f));

    return mesh;
}

/**
 **/
boost::shared_ptr<v3d::brep::BRep> create_poly_cone() {
    const float radius = 0.5f;
    const float half = 0.5f;
    boost::shared_ptr<v3d::brep::BRep> mesh = boost::make_shared<v3d::brep::BRep>();

    const std::vector<glm::vec3> base = ring(radius, -half);
    const glm::vec3 apex(0.0f, half, 0.0f);

    for (unsigned int index = 0; index < sides; index++) {
        const glm::vec3& from = base[index];
        const glm::vec3& to = base[(index + 1) % sides];

        std::vector<glm::vec3> face;
        face.push_back(from);
        face.push_back(to);
        face.push_back(apex);

        mesh->addFace(face, glm::normalize(glm::cross(to - from, apex - from)));
    }

    return mesh;
}

/**
 **/
boost::shared_ptr<v3d::brep::BRep> create_poly_cylinder() {
    const float radius = 0.5f;
    const float half = 0.5f;
    boost::shared_ptr<v3d::brep::BRep> mesh = boost::make_shared<v3d::brep::BRep>();

    const std::vector<glm::vec3> bottom = ring(radius, -half);
    const std::vector<glm::vec3> top = ring(radius, half);

    for (unsigned int index = 0; index < sides; index++) {
        const unsigned int next = (index + 1) % sides;

        std::vector<glm::vec3> face;
        face.push_back(bottom[index]);
        face.push_back(bottom[next]);
        face.push_back(top[next]);
        face.push_back(top[index]);

        // the wall faces outward, which is the ring point's own direction from the axis
        mesh->addFace(face, glm::normalize(glm::vec3(bottom[index].x + bottom[next].x, 0.0f, bottom[index].z + bottom[next].z)));
    }

    return mesh;
}

};  // namespace v3d::editor
