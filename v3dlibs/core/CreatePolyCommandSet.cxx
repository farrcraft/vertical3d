/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#include <cmath>
#include <vector>

#include "CreatePolyCommandSet.h"
#include "../brep/BRep.h"
#include "../type/3dtypes.h"

#include <glm/gtc/constants.hpp>

namespace v3d::core {
    boost::shared_ptr<v3d::brep::BRep> create_poly_cube() {
        float length = 1.0;
        float height = 1.0;
        float width = 1.0;
        length /= 2.0;
        height /= 2.0;
        width /= 2.0;
        boost::shared_ptr<v3d::brep::BRep> mesh(new v3d::brep::BRep());

        // start new polygon - front
        std::vector<glm::vec3> points;
        glm::vec3 norm(0.0, 0.0, -1.0);
        // add 4 vertices
        points.push_back(glm::vec3(-width, -height, -length));
        points.push_back(glm::vec3(width, -height, -length));
        points.push_back(glm::vec3(width, height, -length));
        points.push_back(glm::vec3(-width, height, -length));
        mesh->addFace(points, norm);

        // next polygon - right
        points.clear();
        norm = glm::vec3(1.0, 0.0, 0.0);
        points.push_back(glm::vec3(width, -height, -length));
        points.push_back(glm::vec3(width, -height, length));
        points.push_back(glm::vec3(width, height, length));
        points.push_back(glm::vec3(width, height, -length));
        mesh->addFace(points, norm);

        // next polygon - top
        points.clear();
        norm = glm::vec3(0.0, 1.0, 0.0);
        points.push_back(glm::vec3(-width, height, -length));
        points.push_back(glm::vec3(width, height, -length));
        points.push_back(glm::vec3(width, height, length));
        points.push_back(glm::vec3(-width, height, length));
        mesh->addFace(points, norm);

        // next polygon - left
        points.clear();
        norm = glm::vec3(-1.0, 0.0, 0.0);
        points.push_back(glm::vec3(-width, -height, length));
        points.push_back(glm::vec3(-width, -height, -length));
        points.push_back(glm::vec3(-width, height, -length));
        points.push_back(glm::vec3(-width, height, length));
        mesh->addFace(points, norm);

        // next polygon - back
        points.clear();
        norm = glm::vec3(0.0, 0.0, 1.0);
        points.push_back(glm::vec3(width, -height, length));
        points.push_back(glm::vec3(-width, -height, length));
        points.push_back(glm::vec3(-width, height, length));
        points.push_back(glm::vec3(width, height, length));
        mesh->addFace(points, norm);

        // next polygon - bottom
        points.clear();
        norm = glm::vec3(0.0, -1.0, 0.0);
        points.push_back(glm::vec3(width, -height, -length));
        points.push_back(glm::vec3(-width, -height, -length));
        points.push_back(glm::vec3(-width, -height, length));
        points.push_back(glm::vec3(width, -height, length));
        mesh->addFace(points, norm);

        return mesh;
    }

    boost::shared_ptr<v3d::brep::BRep> create_poly_plane() {
        float length = 1.0;
        float width = 1.0;
        length /= 2.0;
        width /= 2.0;

        boost::shared_ptr<v3d::brep::BRep> mesh(new v3d::brep::BRep());

        glm::vec3 norm(0.0, 1.0, 0.0);

        // add 4 vertices
        std::vector<glm::vec3> vertices;
        vertices.push_back(glm::vec3(-width, 0.0, -length));
        vertices.push_back(glm::vec3(width, 0.0, -length));
        vertices.push_back(glm::vec3(width, 0.0, length));
        vertices.push_back(glm::vec3(-width, 0.0, length));
        mesh->addFace(vertices, norm);

        return mesh;
    }

    boost::shared_ptr<v3d::brep::BRep> create_poly_cone() {
        int sides = 8;
        float height = 1.0;
        float radius = 0.5;

        float delta = glm::two_pi<float>() / sides;

        boost::shared_ptr<v3d::brep::BRep> mesh(new v3d::brep::BRep());

        // create points in cone
        std::vector<glm::vec3> points;
        for (int k = 0; k <= sides; k++) {
            glm::vec3 p;
            // int v;
            p[0] = cos(delta * k) * radius;
            p[1] = sin(delta * k) * radius;
            points.push_back(p);
        }
        // top point
        glm::vec3 v3(0.0, 0.0, height);
        points.push_back(v3);

        // create polygons in cone
        for (int k = 1; k <= sides; k++) {
            glm::vec3 norm(1.0, 0.0, 0.0);
            std::vector<glm::vec3> face;
            glm::vec3 v1, v2;
            v1 = points[k - 1];
            if (k == points.size()) {
                v2 = points[0];
            } else {
                v2 = points[k];
            }

            // add vertices to polygon
            face.push_back(v1);
            face.push_back(v2);
            face.push_back(v3);

            // add polygon to mesh
            mesh->addFace(face, norm);
        }

        return mesh;
    }

    boost::shared_ptr<v3d::brep::BRep> create_poly_cylinder() {
        int sides = 8;
        float height = 1.0;
        float radius = 0.5;

        float delta = glm::two_pi<float>() / sides;

        boost::shared_ptr<v3d::brep::BRep> mesh(new v3d::brep::BRep());

        // create points in cylinder
        std::vector<glm::vec3> points;
        std::vector<glm::vec3> points_top;
        for (int k = 0; k <= sides; k++) {
            glm::vec3 p;
            p[0] = cos(delta * k) * radius;
            p[1] = sin(delta * k) * radius;
            p[2] = 0.0;
            points.push_back(p);
            // corresponding top point
            p[2] += height;
            points_top.push_back(p);
        }

        // create each polygon side of the cylinder
        for (int k = 1; k <= sides; k++) {
            std::vector<glm::vec3> face;
            glm::vec3 v1, v2, v3, v4;
            glm::vec3 norm(1.0, 0.0, 0.0);

            // get vertex indices
            v1 = points[k - 1];
            if (k == points.size()) {
                v2 = points[0];
            } else {
                v2 = points[k];
            }
            // top two points (z + height)
            if (k == points_top.size()) {
                v3 = points_top[0];
            } else {
                v3 = points_top[k];
            }
            v4 = points_top[k - 1];

            // add vertices to polygon
            face.push_back(v1);
            face.push_back(v2);
            face.push_back(v3);
            face.push_back(v4);

            // add polygon to mesh
            mesh->addFace(face, norm);
        }

        return mesh;
    }

};  // namespace v3d::core
