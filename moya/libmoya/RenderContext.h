/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>
#include <api/render/offline/MovingTransform.h>
#include <api/render/offline/Sampling.h>
#include <api/render/offline/Textures.h>
#include <api/render/offline/rib/Declarations.h>
#include <api/render/offline/rib/Requests.h>
#include <api/render/offline/sl/ShaderLibrary.h>
#include <api/render/offline/trace/Primitive.h>
#include <api/render/offline/trace/Scene.h>
#include <moya/libmoya/reyes/Samples.h>

#include "Polygon.h"
#include "FrameBuffer.h"
#include "Shading.h"

#include <cstddef>
#include <vector>
#include <map>
#include <string>

namespace v3d::moya {

namespace reyes {
class GridShader;
};  // namespace reyes

class Hider;

/**
 * The current graphics state. Several contexts may exist at once, and only the last one
 * created is active.
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
         **/
        void render();

        unsigned int imageWidth() const;
        unsigned int imageHeight() const;
        float pixelAspect() const;

        /** The picture's size when no Format names one. **/
        static constexpr unsigned int defaultWidth = 320;
        static constexpr unsigned int defaultHeight = 240;
        /** RI's default frame aspect ratio, the aspect of defaultWidth by defaultHeight. **/
        static constexpr float defaultFrameAspectRatio = 4.0f / 3.0f;
        /** RI's default "fov" for a "perspective" projection, in degrees. **/
        static constexpr float defaultFov = 90.0f;

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
         * space. An empty name is "orthographic". "perspective" reads fov, the full angle
         * in degrees across screen x from -1 to 1. Any other name is kept, and projects
         * through the identity. The current transformation is reset to the identity.
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
         * The reyes pass. Every bucket's grids are hidden into a new set of samples, and the
         * samples are resolved into the planes. The reyes hider renders through this.
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
         * maps to RiDisplay()
         * Names where render() writes the image. A display of type "file" is written
         * through image::Factory, which picks the format from the name's extension. An
         * empty name or any other type writes nothing. The mode is stored and not read.
         **/
        void display(const v3d::render::offline::rib::Display & display);
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
         * Saves and restores the current transformation, colour, opacity, shading rate,
         * surface shader and the set of lights that are on. An end with nothing saved does
         * nothing.
         **/
        void attributeBegin();
        void attributeEnd();
        /**
         * maps to RiCoordinateSystem()
         * Saves the current transformation as a named coordinate system.
         * The predefined names are object, world, camera, screen, raster and NDC.
         **/
        void saveCoordinateSystem(const std::string & name);
        /**
         * maps to RiCoordSysTransform()
         * Replaces the current transformation with the named coordinate system. The name
         * is not checked: one that is neither predefined nor saved is added as a matrix of
         * zeros, which becomes the current transformation.
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
         * "limits" "bucketsize" and "gridsize", and each refuses a zero.
         **/
        void shadingRate(float size);
        void bucketSize(unsigned int width, unsigned int height);
        void gridSize(unsigned int size);
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
         * maps to RiLightSource()
         * Creates a light and switches it on in the current attribute state. The light
         * lasts for the frame. Which lights are on is an attribute, so an Illuminate inside
         * an AttributeBegin block is local to that block. A light that does not compile is
         * left out.
         **/
        void lightSource(const v3d::render::offline::rib::LightSource & request,
            const v3d::render::offline::rib::ParameterList & parameters);
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
         * reyes hider this is the first pass. A polygon outside the clipping planes or the
         * frustum is culled. One that is too large or crosses the eye plane is marked for
         * splitting. A polygon that is not culled goes in the bucket that holds the upper
         * left corner of its raster bound. The reyes hider requires prepareWorld() first.
         **/
        void addPolygon(const boost::shared_ptr<Polygon>& poly);
        /**
         * maps to RiSphere()
         * Only the ray hider draws one, intersected where it is defined; the reyes
         * hider does not dice spheres. A sphere whose radius is not positive is
         * logged and not drawn.
         *
         * @return false when the hider cannot draw spheres
         **/
        bool addSphere(const v3d::render::offline::rib::Sphere & shape);

        /**
         * The matrix of a named coordinate system. A name that is neither predefined nor
         * saved is added as a matrix of zeros.
         **/
        glm::mat4x4 coordinateSystem(const std::string & name);

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
        /** Save as "screen" the projection after the transformation in force at RiProjection. **/
        void screenTransform();
        /** The matrix the projection name, field of view, screen window and clipping give. **/
        glm::mat4x4 projectionMatrix() const;
        /**
         * Add a polygon to the traced scene as a fan of triangles, placed by the current
         * transformation and shaded by the given state.
         **/
        void trace(const Polygon & poly, const Shading & state);
        /** Give a traced primitive the state's surface, opacity and lights, in world space. **/
        void shade(v3d::render::offline::trace::Primitive* primitive, const Shading & state);
        /**
         * The state's lights placed in world space, as one shared set. The set is built
         * again only when the lights that are on, or the number of lights, change.
         **/
        const v3d::render::offline::trace::Lights & tracedLights(const Shading & state);

        /*
         * The options below carry RI's defaults as member initialisers, so that both
         * constructors set every one of them. riEpsilon and riInfinity hold the values of
         * RI_EPSILON and RI_INFINITY, because RenderMan.h is the C interface and is not
         * included here.
         */
        static constexpr float riEpsilon = 1.0e-10f;
        static constexpr float riInfinity = 1.0e38f;
        /**
         * What RiAttributeBegin saves and RiAttributeEnd puts back.
         **/
        class Attributes {
         public:
            v3d::render::offline::MovingTransform transform;
            glm::vec3 color = glm::vec3(1.0f);
            glm::vec3 opacity = glm::vec3(1.0f);
            float shadingRate = 1.0f;
            v3d::render::offline::sl::InstancePtr surface;
            glm::mat4x4 surfacePlacement = glm::mat4x4(1.0f);
            /**
             * Which lights are switched on, by handle. The lights themselves last for the
             * frame, and AttributeEnd restores only this set.
             **/
            std::vector<std::string> lit;
        };

        /**
         * A light the scene created, kept for the frame under the handle a later
         * Illuminate names it by.
         **/
        class LightSource {
         public:
            std::string handle;
            v3d::render::offline::sl::InstancePtr shader;
            glm::mat4x4 placement = glm::mat4x4(1.0f);
        };

        std::string name_;
        std::vector<v3d::render::offline::MovingTransform> transforms_;
        std::vector<Attributes> attributes_;
        std::map<std::string, glm::mat4x4> coordinateSystems_;
        v3d::render::offline::rib::Declarations declarations_;
        boost::shared_ptr<v3d::log::Logger> logger_;
        bool flatMotionReported_ = false;

        /**
         * Whether the current transformation moves and is flat at both ends. A primitive
         * under it has no pose to be moved from and is left out. The first one is logged.
         **/
        bool flatMotion();
        boost::shared_ptr<v3d::render::offline::sl::ShaderLibrary> shaders_;
        boost::shared_ptr<v3d::render::offline::Textures> textures_;
        boost::shared_ptr<reyes::GridShader> shader_;
        std::vector<LightSource> lights_;
        v3d::render::offline::sl::InstancePtr surface_;
        glm::mat4x4 surfacePlacement_ = glm::mat4x4(1.0f);
        std::vector<std::string> lit_;
        v3d::render::offline::trace::Scene traced_;
        /** The lights that are on, in world space, and the handles and count they came from. **/
        v3d::render::offline::trace::Lights tracedLights_;
        std::vector<std::string> tracedFor_;
        std::size_t tracedLightsFor_ = 0;
        v3d::render::offline::sl::InstancePtr imager_;
        boost::shared_ptr<FrameBuffer> frameBuffer_;
        boost::shared_ptr<reyes::Samples> samples_;
        // camera options
        unsigned int xres_ = defaultWidth;
        unsigned int yres_ = defaultHeight;
        float pixelAspect_ = 1.0f;
        float crop_[4] = { 0.0f, 1.0f, 0.0f, 1.0f };  // RiCropWindow's default; nothing sets or reads it
        float frameAspect_ = defaultFrameAspectRatio;
        float screen_[4] = { -defaultFrameAspectRatio, defaultFrameAspectRatio, -1.0f, 1.0f };  // the screen window: left, right, bottom, top
        // RiFormat sets the frame aspect, and the frame aspect sets the screen window. Once a
        // scene names the frame aspect or the screen window, that one stops following. A named
        // projection means prepareWorld() only rebuilds the screen transform
        bool frameAspectNamed_ = false;
        bool screenNamed_ = false;
        bool projectionNamed_ = false;
        glm::vec3 color_ = glm::vec3(1.0f);
        glm::vec3 opacity_ = glm::vec3(1.0f);
        std::string projection_ = "orthographic";
        float fov_ = defaultFov;
        boost::shared_ptr<Hider> hider_;
        // an empty display name writes no file. RI's default display is a framebuffer,
        // which moya does not have
        std::string displayName_;
        std::string displayType_;
        std::string displayMode_;
        // the current transformation, which moves inside a motion block
        v3d::render::offline::MovingTransform transform_;
        // the transformation in force at RiProjection; the projection applies after it
        v3d::render::offline::MovingTransform projectionBase_;
        float near_ = riEpsilon;  // near clipping plane
        float far_ = riInfinity;  // far clipping plane
        v3d::render::offline::Sampling sampling_;

        /*
         * The framebuffer is tiled into buckets of bucketWidth_ by bucketHeight_ pixels.
         * gridSize_ is the number of micropolygons in a grid, so a grid is sqrt(gridSize_)
         * micropolygons a side. A primitive whose raster bound is wider or taller than
         * sqrt(gridSize_) * shadingRate_ pixels is split before it is diced. A smaller
         * shading rate gives smaller micropolygons.
         */
        unsigned int bucketWidth_ = 16;
        unsigned int bucketHeight_ = 16;
        unsigned int gridSize_ = 256;
        static constexpr unsigned int defaultMotionCacheLog2 = 20;
        std::size_t motionCache_ = std::size_t(1) << defaultMotionCacheLog2;
        float shadingRate_ = 1.0f;
};

/**
 * A point through a matrix that may be a perspective one, divided by w.
 *
 * An orthographic projection leaves w at one, so the divide changes nothing. A perspective
 * one writes the eye depth into w, and the divide is what takes the point to screen or raster
 * units. A point whose w is at or behind the eye is returned undivided. addPolygon() tests w
 * itself first, and splits a primitive with such a corner.
 **/
glm::vec3 project(const glm::mat4x4 & m, const glm::vec3 & point);

};  // namespace v3d::moya
