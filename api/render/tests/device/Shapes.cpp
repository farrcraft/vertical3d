/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Shapes.h"

#include <cstdint>

namespace v3d::test {

/**
 **/
v3d::type::Model cube(const glm::vec4& colour) {
    v3d::type::Model model;
    const glm::vec3 normals[6] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    for (const glm::vec3& n : normals) {
        // two axes across the face, chosen so that u x v points along the normal
        const glm::vec3 u = glm::abs(n.y) > 0.5f ? glm::vec3(n.y, 0, 0) * -1.0f : glm::vec3(-n.z, 0, n.x);
        const glm::vec3 v = glm::cross(n, u);
        const uint32_t first = static_cast<uint32_t>(model.vertices().size());
        const glm::vec3 corners[4] = {
            n * 0.5f - u * 0.5f - v * 0.5f, n * 0.5f + u * 0.5f - v * 0.5f,
            n * 0.5f + u * 0.5f + v * 0.5f, n * 0.5f - u * 0.5f + v * 0.5f
        };
        for (const glm::vec3& corner : corners) {
            v3d::type::Model::Vertex vertex;
            vertex.position = corner;
            vertex.normal = n;
            model.vertices().push_back(vertex);
        }
        model.indices().insert(model.indices().end(), {first, first + 1, first + 2, first, first + 2, first + 3});
    }
    v3d::type::Model::Material material;
    material.baseColour = colour;
    model.materials().push_back(material);
    model.parts().push_back({0, static_cast<uint32_t>(model.indices().size()), 0});
    return model;
}

/**
 **/
v3d::type::Model twoCubes() {
    v3d::type::Model model = cube(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
    for (v3d::type::Model::Vertex& vertex : model.vertices()) {
        vertex.position.x -= 0.75f;
    }
    const v3d::type::Model green = cube(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
    const uint32_t base = static_cast<uint32_t>(model.vertices().size());
    const uint32_t first = static_cast<uint32_t>(model.indices().size());
    for (v3d::type::Model::Vertex vertex : green.vertices()) {
        vertex.position.x += 0.75f;
        model.vertices().push_back(vertex);
    }
    for (const uint32_t index : green.indices()) {
        model.indices().push_back(base + index);
    }
    model.materials().push_back(green.materials()[0]);
    model.parts().push_back({first, static_cast<uint32_t>(green.indices().size()), 1});
    return model;
}

};  // namespace v3d::test
