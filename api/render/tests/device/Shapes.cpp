/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Shapes.h"

#include <api/type/Model.h>

#include <cstdint>

namespace v3d::test {

/**
 **/
v3d::type::Model cube(const glm::vec4& colour) {
    v3d::type::Model model;
    const glm::vec3 normals[6] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    for (const glm::vec3& normal : normals) {
        // two axes across the face, chosen so that u x v points along the normal
        const glm::vec3 u = glm::abs(normal.y) > 0.5F ? glm::vec3(normal.y, 0, 0) * -1.0F : glm::vec3(-normal.z, 0, normal.x);
        const glm::vec3 v = glm::cross(normal, u);
        const uint32_t first = static_cast<uint32_t>(model.vertices().size());
        const glm::vec3 corners[4] = {
            normal * 0.5F - u * 0.5F - v * 0.5F, normal * 0.5F + u * 0.5F - v * 0.5F,
            normal * 0.5F + u * 0.5F + v * 0.5F, normal * 0.5F - u * 0.5F + v * 0.5F
        };
        for (const glm::vec3& corner : corners) {
            v3d::type::Model::Vertex vertex;
            vertex.position = corner;
            vertex.normal = normal;
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
    v3d::type::Model model = cube(glm::vec4(1.0F, 0.0F, 0.0F, 1.0F));
    for (v3d::type::Model::Vertex& vertex : model.vertices()) {
        vertex.position.x -= 0.75F;
    }
    const v3d::type::Model green = cube(glm::vec4(0.0F, 1.0F, 0.0F, 1.0F));
    const uint32_t base = static_cast<uint32_t>(model.vertices().size());
    const uint32_t first = static_cast<uint32_t>(model.indices().size());
    for (v3d::type::Model::Vertex vertex : green.vertices()) {
        vertex.position.x += 0.75F;
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
