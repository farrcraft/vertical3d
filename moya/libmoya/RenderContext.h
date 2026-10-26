/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>
#include <api/render/offline/Area.h>
#include <api/render/offline/Camera.h>
#include <api/render/offline/CameraOptions.h>
#include <api/render/offline/MovingTransform.h>
#include <api/render/offline/Sampling.h>
#include <api/render/offline/Textures.h>
#include <api/render/offline/rib/Declarations.h>
#include <api/render/offline/rib/Requests.h>
#include <api/render/offline/sl/ShaderLibrary.h>
#include <api/render/offline/trace/Primitive.h>
#include <api/render/offline/trace/Scene.h>
#include <moya/libmoya/reyes/Samples.h>

#include "Attributes.h"
#include "FrameBuffer.h"
#include "Polygon.h"
#include "Shading.h"
#include "Transforms.h"

#include <cstddef>
#include <functional>
#include <vector>
#include <map>
#include <string>

namespace v3d::moya {

namespace reyes {
class GridShader;
};  // namespace reyes

class Hider;

/**
 * The current graphics state. Several contexts may exist at once. The one most recently begun
 * or made active with RiContext is active.
 **/
class RenderContext {
 public:
        RenderContext();
        explicit RenderContext(const std::string & name);
        ~RenderContext();

        typedef std::vector<boost::shared_ptr<Polygon> > PolygonList;

        /**
         * maps to RiWorldBegin()
         * Builds the screen transform from the camera options as they stand, with the
         * default projection when the scene named none. Saves the current transformation as
         * the camera coordinate system and resets it to the identity. Builds the raster
         * transform, and allocates the framebuffer at the current resolution and bucket size.
         * Nothing refuses an option set after this call.
         **/
        void prepareWorld();
        /**
         * maps to RiWorldEnd()
         * Runs the hider into the framebuffer's planes, then the imager if there is one. A
         * display of type "file" then has its colour channels written. Lights, the traced
         * scene and the framebuffer are kept. Does nothing before prepareWorld().
         *
         * Under the reyes hider, when no shader the scene bound calls trace or transmission,
         * the traced scene's primitives are dropped before the hider runs, since no ray will
         * meet them. An area light keeps its own copy of its geometry.
         *
         * A render the progress callback stops leaves the planes as far as the hider got, runs
         * no imager and writes no file.
         **/
        void render();

        /**
         * Told how much of the frame the hider has finished, from 0 to 1, after each bucket of
         * the reyes hider and each row of the ray-trace hider. It returns false to stop the
         * render there. It runs on whatever thread renders.
         **/
        using Progress = std::function<bool(float done)>;

        /**
         * Set what is told the render's progress. An empty callback is told nothing, and the
         * render runs to its end. The callback outlives every world, as an option does not.
         **/
        void progress(const Progress& callback);

        /**
         * Tell the progress callback how far the hider has got.
         *
         * @return false once the callback has asked to stop, and from then until the next
         *         render begins
         **/
        bool report(float done);

        /**
         * @return whether the last render was stopped before its end
         **/
        bool stopped() const noexcept;

        unsigned int imageWidth() const;
        unsigned int imageHeight() const;
        float pixelAspect() const;

        /** The camera options' defaults, which CameraOptions states. **/
        static constexpr unsigned int defaultWidth = v3d::render::offline::CameraOptions::defaultWidth;
        static constexpr unsigned int defaultHeight = v3d::render::offline::CameraOptions::defaultHeight;
        static constexpr float defaultFov = v3d::render::offline::CameraOptions::defaultFov;

        /**
         * maps to RiFormat(xres, yres, aspect)
         * Sets the pixel resolution, x then y, and pixel aspect ratio of the image.
         * A side of zero or less takes the default for that side, defaultWidth or
         * defaultHeight. A side above largestResolution is refused, and the context keeps
         * the size it had. An aspect that is not a positive finite number is square pixels.
         * @return false when a side is refused
         **/
        bool imageResolution(const glm::ivec2 & resolution, float aspect);
        /**
         * maps to RiFrameAspectRatio(aspect)
         * The ratio of the width of the whole image to its height. imageResolution() sets it
         * from the pixel resolution until a scene calls this, and from then on leaves it.
         **/
        void frameAspectRatio(float aspect);
        /**
         * maps to RiScreenWindow(left, right, bottom, top)
         * The rectangle of screen space the image covers. Until a scene calls this, it
         * follows the frame aspect a: [-a, a] by [-1, 1] when a is at least 1, and
         * [-1, 1] by [-1/a, 1/a] otherwise.
         **/
        void screenWindow(const v3d::render::offline::rib::ScreenWindow & window);
        /**
         * maps to RiClipping(hither, yon)
         * Sets the eye space depth of the near and far clipping planes.
         **/
        void clipping(const v3d::render::offline::rib::Clipping & planes);

        /**
         * maps to RiProjection()
         * The projection and screen transformations together take camera space to screen
         * space. An empty name is RI_NULL, which applies no projection, so camera space is
         * screen space. "perspective" reads fov, the full angle in degrees across screen x
         * from -1 to 1. "orthographic" maps the screen window and the clipping range onto
         * the canonical volume. The current transformation is reset to the identity.
         *
         * Any other name is logged and refused, and the projection in force stays.
         **/
        void projection(std::string name, float fov = defaultFov);
        /**
         * maps to RiHider()
         * "hidden", RI's default, is the reyes hider. "raytrace" casts a
         * primary ray through every sample instead. Any other name is
         * logged, and the hider stays as it was.
         **/
        void hider(const std::string & name);
        bool raytracing() const;
        /**
         * Renders the buckets, for the reyes hider. Every bucket's grids are hidden into a new
         * set of samples, and the samples are then resolved into the planes.
         **/
        void bucket(v3d::render::offline::FrameBuffer* planes);
        /** The near clipping plane, RiClipping's hither. **/
        float hither() const;
        /**
         * maps to RiMotionBegin() and RiMotionEnd(). Each transform request between
         * them is the current transformation at the next of the times.
         **/
        void motionBegin(const std::vector<float> & times);
        void motionEnd();
        /**
         * Whether the projection is "perspective". Only a perspective projection gets
         * depth of field.
         **/
        bool perspective() const;
        /**
         * The camera as WorldBegin fixed it. Before WorldBegin its screen matrix is the one
         * the last RiProjection gave, and every other matrix is the identity.
         **/
        const v3d::render::offline::Camera & camera() const;

        /**
         * maps to RiDisplay()
         * Names where render() writes the image. A display of type "file" is written
         * through image::Factory, which picks the format from the name's extension. An
         * empty name or any other type writes nothing. The mode is stored and not read.
         **/
        void display(const v3d::render::offline::rib::Display & display);
        /**
         * The same, with the request's parameter list. No display parameter is read, so each
         * one is logged as not used.
         **/
        void display(const v3d::render::offline::rib::Display & display,
            const v3d::render::offline::rib::ParameterList & parameters);
        /**
         * maps to RiCropWindow()
         * The part of the image to render, as fractions of its width and height. The window
         * is held to [0, 1] on each axis. A window with no area is logged and refused.
         **/
        void cropWindow(const v3d::render::offline::rib::CropWindow & window);
        /**
         * The pixels inside the crop window: columns x to y and rows z to w, each range
         * including its start and excluding its end. RI renders the columns from
         * ceil(width * xmin) to ceil(width * xmax) - 1, and the rows likewise. render() leaves
         * every pixel outside them empty.
         **/
        glm::uvec4 cropped() const;
        const std::string & displayName() const;

        /**
         * maps to RiTransformBegin() and RiTransformEnd()
         * pop restores the transformation the matching push saved. A pop with nothing
         * saved does nothing.
         **/
        void pushTransform();
        void popTransform();
        /**
         * maps to RiAttributeBegin() and RiAttributeEnd()
         * Saves and restores the current transformation and the Attributes. An end with
         * nothing saved does nothing.
         **/
        void attributeBegin();
        void attributeEnd();
        /**
         * maps to RiCoordinateSystem()
         * Saves the current transformation as a named coordinate system. The predefined
         * names are object, world, camera, screen, raster and NDC. The camera sets them, and
         * a predefined name given here is logged and not saved.
         **/
        void saveCoordinateSystem(const std::string & name);
        /**
         * maps to RiCoordSysTransform()
         * Replaces the current transformation with the named coordinate system. A name that
         * is neither predefined nor saved is logged, and the transformation is left as it was.
         **/
        void setCoordinateSystem(const std::string & name);
        void setIdentityTransform();
        void setTransform(const glm::mat4x4 & trans);
        /**
         * maps to RiConcatTransform()
         * The given transform applies to a point before the current one does. This is the
         * order RI concatenates in: a translate inside a rotate moves along the rotated axes.
         **/
        void concatTransform(const glm::mat4x4 & trans);

        void translate(float dx, float dy, float dz);
        /**
         * maps to RiRotate()
         * The angle is in degrees. A positive angle turns +z towards +x about y, and +z
         * towards -y about x. RI's unit cube example needs that sign to put each face where
         * it names it.
         **/
        void rotate(float angle, const glm::vec3 & axis);
        void scale(float sx, float sy, float sz);

        /**
         * maps to RiColor() and RiOpacity()
         * The colour and opacity a primitive added from here on carries. A vertex with a
         * "Cs" of its own keeps it. The surface shader reads the colour as Cs.
         **/
        void color(const glm::vec3 & value);
        glm::vec3 color() const;
        void opacity(const glm::vec3 & value);
        glm::vec3 opacity() const;

        /**
         * shadingRate maps to RiShadingRate(). bucketSize and gridSize map to Option
         * "limits" "bucketsize" and "gridsize", and both refuse a zero. A size the command
         * line pinned is left as it is, and nothing is reported.
         **/
        void shadingRate(float size);
        void bucketSize(unsigned int width, unsigned int height);
        void gridSize(unsigned int size);
        /**
         * Sets a size and pins it, so a scene's Option "limits" does not replace it. moya's
         * `--bucket` and `--grid` pin the sizes they name. A zero is refused and pins nothing.
         **/
        void pinBucketSize(unsigned int width, unsigned int height);
        void pinGridSize(unsigned int size);
        /**
         * The most sample motions one moving grid caches. A grid that sweeps more samples
         * than this works out each micropolygon's motion again instead, which is slower
         * and needs no memory. The default is 2^20 entries, which is 64 MB.
         **/
        void motionCache(std::size_t entries);
        std::size_t motionCache() const;

        /**
         * maps to RiSurface()
         * The shader a primitive added from here on is shaded by. A scene that names
         * none draws "constant", the shader that does no shading.
         **/
        void surface(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters);
        /**
         * maps to RiDisplacement()
         * The displacement shader for the primitives that follow, in the current attribute
         * state, placed as a surface shader is.
         **/
        void displacement(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters);
        /**
         * maps to RiLightSource()
         * Creates a light and switches it on in the current attribute state. The light
         * lasts for the frame. Which lights are on is an attribute, so an Illuminate inside
         * an AttributeBegin block is local to that block. A light that does not compile is
         * left out.
         **/
        void lightSource(const v3d::render::offline::rib::LightSource & request,
            const v3d::render::offline::rib::ParameterList & parameters);
        /**
         * maps to RiAttribute()
         * `Attribute "light" "integer nsamples"` is how many points an area light made after
         * it samples at each shading point. A count below one is taken as one, and one above
         * 4096 as 4096. `Attribute "displacementbound" "float sphere"` is how far a
         * displacement shader may move a point, in the space `"string coordinatesystem"`
         * names, or in object space without one. A bound that is negative or not finite is not
         * used, with a warning. Any other
         * attribute is reported once by name and not used.
         **/
        void attribute(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters);
        /**
         * maps to RiAreaLightSource()
         * A light, as lightSource() makes one, that also keeps the geometry declared after it
         * in the same attribute block: every polygon and sphere, in world space, whichever
         * hider draws them. That geometry renders as a surface too. Geometry after the block
         * ends does not join it.
         **/
        void areaLightSource(const v3d::render::offline::rib::LightSource & request,
            const v3d::render::offline::rib::ParameterList & parameters);
        /** The geometry of the area light being declared, or null outside its attribute block. **/
        const boost::shared_ptr<v3d::render::offline::Area> & areaLight() const;
        /**
         * maps to RiIlluminate()
         **/
        void illuminate(const std::string & handle, bool on);
        /**
         * maps to RiImager()
         **/
        void imager(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters);
        /**
         * Where a .sl file that is not built in is looked for, from
         * Option "searchpath" "shader".
         **/
        void searchpath(const std::string & path);

        /**
         * The images the scene's shaders read, each loaded once, and where a relative
         * name is looked for, from Option "searchpath" "texture".
         **/
        v3d::render::offline::Textures & textures();

        /**
         * The surface shader, its placement, the opacity and the lights that are on, for
         * a primitive submitted now.
         **/
        Shading shading();

        /**
         * Runs a surface shader over a grid.
         *
         * One is made for the context rather than one per grid, because it holds the
         * register files that later grids reuse. It is created on first use.
         **/
        reyes::GridShader & shader();

        /**
         * The scene a shader's trace() and transmission() calls are traced through. It
         * holds every primitive the scene submitted, in world space, before any split.
         **/
        v3d::render::offline::trace::Scene & traced();

        /**
         * maps to RiPolygon()
         * Under the ray hider the polygon is only added to the traced scene. Under the
         * reyes hider it is also added to the traced scene, then bounded and bucketed, and
         * prepareWorld() must come first. A polygon outside the clipping planes or the
         * frustum is culled. One that is too large or crosses the eye plane is marked for
         * splitting. A polygon that is not culled goes in the bucket that holds the upper
         * left corner of its raster bound. The pieces of a split come back through here,
         * and are bucketed again but not traced again.
         **/
        void addPolygon(const boost::shared_ptr<Polygon>& poly);
        /**
         * maps to RiSphere()
         * Only the ray hider draws one, and intersects it exactly. The reyes hider does
         * not dice spheres. A sphere whose radius is not positive is logged and not drawn.
         *
         * @return false when the hider cannot draw spheres
         **/
        bool addSphere(const v3d::render::offline::rib::Sphere & shape);

        /**
         * The matrix of a named coordinate system. "camera" is world to camera, and "screen",
         * "raster" and "NDC" are each from camera space, as the camera holds them. "object"
         * and "world" are the identity. A space the scene saved is its transformation into
         * world space. A name that is neither predefined nor saved is logged, and gives the
         * identity.
         **/
        glm::mat4x4 coordinateSystem(const std::string & name);
        /** Whether a coordinate system is predefined or saved by the scene. **/
        bool known(const std::string & name) const;

        /**
         * maps to RiDeclare()
         * The parameter types the C entry points read. The RIB reader keeps its own
         * declarations for the file it reads, separate from these.
         **/
        v3d::render::offline::rib::Declarations & declarations();

        /**
         * Where this context logs a request it cannot carry out. The RIB reader has a
         * logger of its own for errors in the file.
         **/
        const boost::shared_ptr<v3d::log::Logger> & logger() const;

        /**
         * The buckets the world was prepared into. Null until prepareWorld().
         **/
        boost::shared_ptr<FrameBuffer> framebuffer() const;
        /**
         * The frame's samples, which the reyes hider writes into. bucket() creates them
         * from sampling() as it stands then, so they do not exist under the ray hider.
         **/
        reyes::Samples & samples();

        unsigned int bucketWidth() const;
        unsigned int bucketHeight() const;
        unsigned int gridSize() const;
        float shadingRate() const;

        /**
         * How the frame is sampled: what RiPixelSamples, RiPixelFilter,
         * RiPixelVariance, RiShutter and RiDepthOfField asked for.
         **/
        v3d::render::offline::Sampling & sampling();
        const v3d::render::offline::Sampling & sampling() const;
        /**
         * How many samples a pixel took in the last render under the ray hider, or
         * zero under the reyes hider, which takes what PixelSamples names.
         **/
        unsigned int samplesTaken(const glm::uvec2 & pixel) const;

 protected:
        void initialize();

 private:
        /**
         * Add a polygon to the traced scene as a fan of triangles, placed by the current
         * transformation and shaded by the given state.
         **/
        void trace(const Polygon & poly, const Shading & state);
        /**
         * Add a polygon with a displacement shader to the traced scene as the surface the
         * shader moves it to. It is diced in camera space into a grid of tracedAcross
         * micropolygons a side, displaced, and added as two triangles for each micropolygon.
         **/
        void traceDisplaced(const Polygon & poly, const Shading & state);
        /** The micropolygons a side a displaced polygon is diced into for the traced scene. **/
        static constexpr unsigned int tracedAcross = 64;
        /** Give a traced primitive the state's surface, opacity and lights, in world space. **/
        void shade(v3d::render::offline::trace::Primitive* primitive, const Shading & state);
        /**
         * The state's lights placed in world space, as one shared set. The set is built
         * again only when the lights that are on, or the number of lights, change.
         **/
        const v3d::render::offline::trace::Lights & tracedLights(const Shading & state);
        /** Note a shader the scene bound, so render() knows whether any of them traces. **/
        void bound(const v3d::render::offline::sl::InstancePtr & shader);

        /*
         * The options below carry RI's defaults as member initialisers, so that both
         * constructors set every one of them.
         */
        /** What RiAttributeBegin saves and RiAttributeEnd puts back. **/
        class Saved final {
         public:
            v3d::render::offline::MovingTransform transform;
            Attributes attributes;
        };

        /**
         * A light the scene created, kept for the frame under the handle a later
         * Illuminate names it by.
         **/
        class LightSource {
         public:
            std::string handle;
            v3d::render::offline::sl::InstancePtr shader;
            glm::mat4x4 placement = glm::mat4x4(1.0F);
            /** The geometry an area light shines from, or null for any other light. **/
            boost::shared_ptr<v3d::render::offline::Area> area;
        };

        Transforms transforms_;
        Attributes attributes_;
        std::vector<Saved> saved_;
        /** The attributes already reported as not used, so each is reported once. **/
        std::vector<std::string> unusedAttributes_;

        std::string name_;
        v3d::render::offline::rib::Declarations declarations_;
        boost::shared_ptr<v3d::log::Logger> logger_;
        bool flatMotionReported_ = false;
        bool beforeWorldReported_ = false;

        /**
         * Whether the current transformation moves and has no inverse at both ends. A
         * primitive under it has no stored pose to move from, and is left out. The first such
         * primitive is logged.
         **/
        bool flatMotion();
        /**
         * Whether WorldBegin has not yet been reached. The camera is fixed there, so a polygon
         * sent before it cannot be placed, and is left out. The first such polygon is logged.
         **/
        bool beforeWorld();
        /**
         * Place a polygon that is not yet placed with the state it was submitted under, and
         * trace it. A polygon with more than four vertices is then handed back to addPolygon()
         * as pieces.
         *
         * @return true when the polygon was handed on as pieces, and is not bucketed itself
         **/
        bool handedOn(const boost::shared_ptr<Polygon>& poly);
        boost::shared_ptr<v3d::render::offline::sl::ShaderLibrary> shaders_;
        boost::shared_ptr<v3d::render::offline::Textures> textures_;
        boost::shared_ptr<reyes::GridShader> shader_;
        std::vector<LightSource> lights_;
        v3d::render::offline::trace::Scene traced_;
        /** Whether any shader the scene bound calls trace or transmission. **/
        bool tracing_ = false;
        /** The lights that are on, in world space, and the handles and count they came from. **/
        v3d::render::offline::trace::Lights tracedLights_;
        std::vector<std::string> tracedFor_;
        std::size_t tracedLightsFor_ = 0;
        v3d::render::offline::sl::InstancePtr imager_;
        boost::shared_ptr<FrameBuffer> frameBuffer_;
        boost::shared_ptr<reyes::Samples> samples_;
        Progress progress_;
        bool stopped_ = false;
        v3d::render::offline::CameraOptions options_;
        v3d::render::offline::Camera camera_;
        v3d::render::offline::rib::CropWindow crop_;
        boost::shared_ptr<Hider> hider_;
        // an empty display name writes no file. RI's default display is a framebuffer,
        // which moya does not have
        std::string displayName_;
        std::string displayType_;
        std::string displayMode_;
        v3d::render::offline::Sampling sampling_;

        /*
         * The framebuffer is tiled into buckets of bucketWidth_ by bucketHeight_ pixels.
         * gridSize_ is the number of micropolygons in a grid, so a grid is sqrt(gridSize_)
         * micropolygons a side. The shading rate, an attribute, is the area of one
         * micropolygon in pixels, so a primitive whose raster bound is wider or taller than
         * sqrt(gridSize_ * shading rate) pixels is split before it is diced. A smaller shading
         * rate gives smaller micropolygons.
         */
        unsigned int bucketWidth_ = 16;
        unsigned int bucketHeight_ = 16;
        unsigned int gridSize_ = 256;
        /** Whether the command line pinned the bucket size or the grid size. **/
        bool bucketPinned_ = false;
        bool gridPinned_ = false;
        static constexpr unsigned int defaultMotionCacheLog2 = 20;
        std::size_t motionCache_ = std::size_t(1) << defaultMotionCacheLog2;
};

/**
 * A point through a matrix that may be a perspective one, divided by w.
 *
 * An orthographic projection leaves w at one, so the divide changes nothing. A perspective
 * one writes the eye depth into w, and the divide takes the point to screen or raster units.
 * A point whose w is at or behind the eye is returned undivided. addPolygon() tests w
 * itself first, and splits a primitive with such a corner.
 **/
glm::vec3 project(const glm::mat4x4 & matrix, const glm::vec3 & point);

};  // namespace v3d::moya
