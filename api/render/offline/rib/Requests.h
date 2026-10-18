/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <vector>

namespace v3d::render::offline::rib {

/**
 * The arguments of the RI requests that take several values of one type. Each is named at the
 * call, so values of the same type cannot be passed in the wrong order:
 *
 *     handler->clipping({.hither = 0.1f, .yon = 100.0f});
 **/

/**
 * RiDeclare: a name and the declaration it is given, such as "uniform point".
 **/
struct Declare {
    std::string name;
    std::string type;
};

/**
 * RiFormat. RI reads a side of zero or less as the renderer's default for that side, and such
 * a side arrives as 0. Every other side is from 1 to largestResolution.
 **/
struct Format {
    unsigned int width = 0;
    unsigned int height = 0;
    float pixelAspect = 1.0f;
};

/**
 * RiScreenWindow: the screen space rectangle the image covers.
 **/
struct ScreenWindow {
    float left = -1.0f;
    float right = 1.0f;
    float bottom = -1.0f;
    float top = 1.0f;
};

/**
 * RiCropWindow: the part of the image to render, as fractions of it.
 **/
struct CropWindow {
    float xmin = 0.0f;
    float xmax = 1.0f;
    float ymin = 0.0f;
    float ymax = 1.0f;
};

/**
 * RiClipping: the camera space depths of the near and far clipping planes.
 **/
struct Clipping {
    float hither = 0.0f;
    float yon = 0.0f;
};

/**
 * RiDepthOfField. An infinite fstop is a pinhole camera.
 **/
struct DepthOfField {
    float fstop = 0.0f;
    float focalLength = 0.0f;
    float focalDistance = 0.0f;
};

/**
 * RiShutter: when the shutter opens and closes.
 **/
struct Shutter {
    float open = 0.0f;
    float close = 0.0f;
};

/**
 * RiDisplay: where the image goes, what kind of output it is, and which channels it holds.
 **/
struct Display {
    std::string name;
    std::string type;
    std::string mode;
};

/**
 * RiLightSource and RiAreaLightSource.
 *
 * RIB 3.03 writes the handle as a sequence number and later RIB writes a string. Both are read,
 * and it is a string here either way, so a renderer keying a map on it handles one form.
 **/
struct LightSource {
    std::string shader;  /**< the light shader's name **/
    std::string handle;  /**< what a later Illuminate names this light by **/
};

/**
 * RiMakeTexture: an image made into a texture file.
 **/
struct MakeTexture {
    std::string picture;
    std::string texture;
    std::string swrap;
    std::string twrap;
    std::string filter;  /**< the name of the filter the texture is made with **/
    float swidth = 1.0f;
    float twidth = 1.0f;
};

/**
 * RiPointsPolygons: the faces of a mesh, as indices into its "P" array.
 **/
struct PointsPolygons {
    std::vector<unsigned int> counts;   /**< how many vertices each polygon has **/
    std::vector<unsigned int> indices;  /**< the vertices of every polygon in turn **/
};

/**
 * RiSphere: a sphere about the origin, cut at two heights and swept through an angle in
 * degrees.
 **/
struct Sphere {
    /** The thetamax of a whole turn, which closes the sphere. **/
    static constexpr float FULL_THETAMAX = 360.0f;

    float radius = 1.0f;
    float zmin = -1.0f;
    float zmax = 1.0f;
    float thetamax = FULL_THETAMAX;
};

};  // namespace v3d::render::offline::rib
