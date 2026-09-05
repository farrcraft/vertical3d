/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <vector>

#include "RIBParameters.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace v3d::render::offline {

    /**
     * What a RIB reader hands a renderer, per ADR-0025.
     *
     * The methods mirror the RI request set, which is what decides what a scene can say. A
     * method that does not correspond to an RI request does not belong here.
     *
     * **Every method has an empty body rather than being pure virtual.** The RI standard asks
     * a renderer to accept a request for a feature it does not support, and a request added
     * later then breaks neither renderer. The cost is that a misspelled override is silent, so
     * every override carries `override`.
     **/
    class RIBHandler {
     public:
        virtual ~RIBHandler() { }

        virtual void version(float number) { (void)number; }
        virtual void declare(const std::string & name, const std::string & declaration) {
            (void)name;
            (void)declaration;
        }
        virtual void option(const std::string & name, const ParameterList & parameters) {
            (void)name;
            (void)parameters;
        }

        // the camera
        virtual void format(unsigned int width, unsigned int height, float pixelAspect) {
            (void)width;
            (void)height;
            (void)pixelAspect;
        }
        virtual void frameAspectRatio(float aspect) { (void)aspect; }
        virtual void screenWindow(float left, float right, float bottom, float top) {
            (void)left;
            (void)right;
            (void)bottom;
            (void)top;
        }
        virtual void cropWindow(float xmin, float xmax, float ymin, float ymax) {
            (void)xmin;
            (void)xmax;
            (void)ymin;
            (void)ymax;
        }
        virtual void projection(const std::string & name, const ParameterList & parameters) {
            (void)name;
            (void)parameters;
        }
        virtual void clipping(float hither, float yon) {
            (void)hither;
            (void)yon;
        }
        virtual void display(const std::string & name, const std::string & type, const std::string & mode,
            const ParameterList & parameters) {
            (void)name;
            (void)type;
            (void)mode;
            (void)parameters;
        }

        // block structure
        virtual void frameBegin(int frame) { (void)frame; }
        virtual void frameEnd() { }
        virtual void worldBegin() { }
        virtual void worldEnd() { }
        virtual void attributeBegin() { }
        virtual void attributeEnd() { }
        virtual void transformBegin() { }
        virtual void transformEnd() { }

        // the transformation stack
        virtual void identity() { }
        virtual void transform(const glm::mat4x4 & matrix) { (void)matrix; }  // NOLINT(build/include_what_you_use) - RiTransform, not std::transform
        virtual void concatTransform(const glm::mat4x4 & matrix) { (void)matrix; }
        virtual void translate(float dx, float dy, float dz) {
            (void)dx;
            (void)dy;
            (void)dz;
        }
        virtual void rotate(float angle, float dx, float dy, float dz) {
            (void)angle;
            (void)dx;
            (void)dy;
            (void)dz;
        }
        virtual void scale(float sx, float sy, float sz) {
            (void)sx;
            (void)sy;
            (void)sz;
        }

        // the graphics state
        virtual void color(const glm::vec3 & value) { (void)value; }
        virtual void opacity(const glm::vec3 & value) { (void)value; }
        virtual void shadingRate(float size) { (void)size; }
        virtual void attribute(const std::string & name, const ParameterList & parameters) {
            (void)name;
            (void)parameters;
        }
        virtual void surface(const std::string & name, const ParameterList & parameters) {
            (void)name;
            (void)parameters;
        }
        virtual void lightSource(const std::string & name, const ParameterList & parameters) {
            (void)name;
            (void)parameters;
        }

        // geometry
        /**
         * One closed planar convex polygon. RIB carries no vertex count - it is the length of
         * the "P" array, which the reader has already divided out.
         **/
        virtual void polygon(unsigned int vertices, const ParameterList & parameters) {
            (void)vertices;
            (void)parameters;
        }
        /**
         * @param counts how many vertices each polygon has
         * @param indices the vertices of every polygon in turn, indexing the "P" array
         **/
        virtual void pointsPolygons(const std::vector<unsigned int> & counts,
            const std::vector<unsigned int> & indices, const ParameterList & parameters) {
            (void)counts;
            (void)indices;
            (void)parameters;
        }
        virtual void sphere(float radius, float zmin, float zmax, float thetamax, const ParameterList & parameters) {
            (void)radius;
            (void)zmin;
            (void)zmax;
            (void)thetamax;
            (void)parameters;
        }
    };

};  // namespace v3d::render::offline
