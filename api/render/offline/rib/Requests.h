/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <array>
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
    float pixelAspect = 1.0F;
};

/**
 * RiScreenWindow: the screen space rectangle the image covers.
 **/
struct ScreenWindow {
    float left = -1.0F;
    float right = 1.0F;
    float bottom = -1.0F;
    float top = 1.0F;
};

/**
 * RiCropWindow: the part of the image to render, as fractions of it.
 **/
struct CropWindow {
    float xmin = 0.0F;
    float xmax = 1.0F;
    float ymin = 0.0F;
    float ymax = 1.0F;
};

/**
 * RiClipping: the camera space depths of the near and far clipping planes.
 **/
struct Clipping {
    float hither = 0.0F;
    float yon = 0.0F;
};

/**
 * RiDepthOfField. An infinite fstop is a pinhole camera.
 **/
struct DepthOfField {
    float fstop = 0.0F;
    float focalLength = 0.0F;
    float focalDistance = 0.0F;
};

/**
 * RiShutter: when the shutter opens and closes.
 **/
struct Shutter {
    float open = 0.0F;
    float close = 0.0F;
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
 * RIB 3.03 writes the handle as a sequence number and later RIB writes a string. The reader
 * accepts both and stores the handle as a string, so a renderer keys its lights on one type.
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
    float swidth = 1.0F;
    float twidth = 1.0F;
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
    static constexpr float FULL_THETAMAX = 360.0F;

    float radius = 1.0F;
    float zmin = -1.0F;
    float zmax = 1.0F;
    float thetamax = FULL_THETAMAX;
};

/**
 * RiBasis: the matrices cubic patches and curves are evaluated through, and how many control
 * points each segment moves on, in u and in v. A curve runs in v.
 *
 * A matrix is in RI's order, row by row, so a point at t is [t^3 t^2 t 1] times the matrix
 * times the segment's four control points. The default is Bezier in both, stepping by 3.
 **/
struct Basis {
    using Matrix = std::array<float, 16>;

    static constexpr Matrix BEZIER{ -1, 3, -3, 1, 3, -6, 3, 0, -3, 3, 0, 0, 1, 0, 0, 0 };
    static constexpr unsigned int BEZIER_STEP = 3;

    Matrix u = BEZIER;
    unsigned int ustep = BEZIER_STEP;
    Matrix v = BEZIER;
    unsigned int vstep = BEZIER_STEP;
};

/**
 * RiCurves: curves of control points, each a run of the "P" array. A linear curve is straight
 * lines through its points. A cubic curve is evaluated through the current basis in v. A
 * periodic curve joins its end to its start.
 **/
struct Curves {
    bool cubic = false;
    std::vector<unsigned int> vertices;  /**< how many control points each curve has **/
    bool periodic = false;
};

};  // namespace v3d::render::offline::rib
