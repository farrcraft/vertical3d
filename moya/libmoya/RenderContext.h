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
#include <api/render/offline/sl/ShaderLibrary.h>
#include <api/render/offline/trace/Primitive.h>
#include <api/render/offline/trace/Scene.h>

#include "Polygon.h"
#include "FrameBuffer.h"
#include "Samples.h"
#include "Shading.h"

#include <cstddef>
#include <vector>
#include <map>
#include <string>

namespace v3d::moya {

class GridShader;
class Hider;

/**
    *	holds the current graphics state
    *	multiple contexts may exist at once, but only one is ever active at any
    *	time
    */
class RenderContext {
 public:
        RenderContext();
        explicit RenderContext(const std::string & name);
        ~RenderContext();

        typedef std::vector<boost::shared_ptr<Polygon> > PolygonList;

        /**
            *	maps to RiWorldBegin()
            *	freezes all rendering options, world to camera transformation 
            *	is set to current transormation, current transformation is set to identity
            *	the view options are frozen beyond this call
            *	the framebuffer will be allocated here
            */
        void prepareWorld();
        /**
            *	maps to RiWorldEnd()
            *	once rendering is done, objects, lights and other stuff set
            *	after prepareWorld() are destroyed and the memory reclaimed
            *	saving the output file is done here also.
            *	the results might not even include a rendered image depending
            *	on the context. we might just output a rib file.
            */
        void render();

        // get methods
        unsigned int imageWidth() const;
        unsigned int imageHeight() const;
        float pixelAspect() const;

        // manipulators
        /**
            *	maps to RiFormat(xres, yres, aspect)
            *	sets the pixel resolution and aspect ratio of the image to 
            *	be rendered
            *	default values will be used when not called
            */
        void imageResolution(int xres, int yres, float aspect);
        /**
            *	maps to RiFrameAspectRatio(aspect)
            *	the ratio of the width of the whole image to its height. Set by
            *	imageResolution() from the pixel resolution unless this named one, so
            *	RiFormat and RiFrameAspectRatio can be set independently.
            */
        void frameAspectRatio(float aspect);
        /**
            *	maps to RiScreenWindow(left, right, bottom, top)
            *	the rectangle of screen space the image covers. Defaults from the frame
            *	aspect - [-a, a] by [-1, 1] for an image wider than it is tall - so a
            *	4:3 image does not stretch a square window across itself.
            */
        void screenWindow(float left, float right, float bottom, float top);
        /**
            *	maps to RiClipping(hither, yon)
            *	sets the position of the near and far clipping planes
            */
        void clipping(float near, float far);

        void projection(std::string name, float fov = 90.0);
        /**
            *	maps to RiHider()
            *	"hidden", RI's default, is the reyes hider. "raytrace" casts a
            *	primary ray through every sample instead. Any other name is
            *	reported, and the hider stays as it was.
            */
        void hider(const std::string & name);
        bool raytracing() const;
        /**
            *	The reyes pass: every bucket's grids hidden into a fresh set of samples, and
            *	those resolved into the planes. What the reyes hider renders with.
            */
        void bucket(v3d::render::offline::FrameBuffer* planes);
        /** The near clipping plane, RiClipping's hither. **/
        float hither() const;
        /**
            *	maps to RiMotionBegin() and RiMotionEnd(). Each transform request between
            *	them is the current transformation at the next of the times.
            */
        void motionBegin(const std::vector<float> & times);
        void motionEnd();
        /**
            *	Whether the projection is a perspective one, which is the only kind a lens
            *	can blur.
            */
        bool perspective() const;

        /**
            *	maps to RiDisplay()
            *	names where the render's samples go once the buckets are done. A type of
            *	"file" is what render() writes through image::Factory, which picks the
            *	format from the name's extension.
            */
        void display(const std::string & name, const std::string & type, const std::string & mode);
        const std::string & displayName() const;

        /**
            *	maps to RiTransformBegin() and RiTransformEnd()
            *	pop restores what push saved, so the pair brackets a change rather than
            *	discarding the saved state.
            */
        void pushTransform();
        void popTransform();
        /**
            *	maps to RiAttributeBegin() and RiAttributeEnd()
            *	the current transform, colour, opacity and shading rate push and pop
            *	together - a scene with two objects nests them and expects all of it back.
            */
        void attributeBegin();
        void attributeEnd();
        /**
            *	maps to RiCoordinateSystem()
            *	saves the current transformation as a custom named coordinate system
            *	default reserved names: object, world, camera, screen, raster, NDC
            */
        void saveCoordinateSystem(const std::string & name);
        /**
            *	maps to RiCoordSysTransform()
            *	replaces the current transformation with the named coordinate system
            *	behavior is undefined if name isn't a reserved name or previously saved name
            *
            *	[FIXME]: should these *not* start with "set" ?
            */
        void setCoordinateSystem(const std::string & name);
        void setIdentityTransform();
        void setTransform(const glm::mat4x4 & trans);
        /**
            *	maps to RiConcatTransform()
            *	the given transform applies before what the current one already holds,
            *	which is the direction RI concatenates in: a translate inside a rotate
            *	moves along the rotated axes.
            */
        void concatTransform(const glm::mat4x4 & trans);

        void translate(float dx, float dy, float dz);
        void rotate(float angle, float dx, float dy, float dz);
        void scale(float sx, float sy, float sz);

        /**
            *	maps to RiColor() and RiOpacity()
            *	the colour a primitive added from here on carries, unless it brings a
            *	"Cs" of its own. It is not a material - there is no light and no shader
            *	behind it.
            */
        void color(const glm::vec3 & value);
        glm::vec3 color() const;
        void opacity(const glm::vec3 & value);
        glm::vec3 opacity() const;

        /**
            *	maps to RiShadingRate() and to Option "limits"
            */
        void shadingRate(float size);
        void bucketSize(unsigned int width, unsigned int height);
        void gridSize(unsigned int size);
        /**
            *	The most sample motions one moving grid caches. A grid that sweeps more samples
            *	than this works out each micropolygon's motion afresh instead, which is slower
            *	and needs no memory. A million by default, which is 64 MB.
            */
        void motionCache(std::size_t entries);
        std::size_t motionCache() const;

        /**
            *	maps to RiSurface()
            *	the shader a primitive added from here on is shaded by. A scene that names
            *	none draws "constant", the shader that means no shading.
            */
        void surface(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters);
        /**
            *	maps to RiLightSource()
            *	creates a light and switches it on in the current attribute state. The
            *	light itself belongs to the frame; which lights are on is an attribute,
            *	so an Illuminate inside an AttributeBegin block is local to that block.
            */
        void lightSource(const std::string & name, const std::string & handle,
            const v3d::render::offline::rib::ParameterList & parameters);
        /**
            *	maps to RiIlluminate()
            */
        void illuminate(const std::string & handle, bool on);
        /**
            *	maps to RiImager()
            */
        void imager(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters);
        /**
            *	Where a .sl file that is not built in is looked for, from
            *	Option "searchpath" "shader".
            */
        void searchpath(const std::string & path);

        /**
            *	The images the scene's shaders read, each once, and where a relative
            *	name is looked for, from Option "searchpath" "texture".
            */
        v3d::render::offline::Textures & textures();

        /**
            *	The surface shader and the lights a primitive submitted now is shaded by.
            */
        Shading shading();

        /**
            *	What runs a surface shader over a grid.
            *
            *	Kept for the render rather than made per grid, because it holds the
            *	register files that later grids reuse.
            */
        GridShader & shader();

        /**
            *	The scene a shader's trace() and transmission() calls are traced
            *	through: every primitive the scene gave, in world space, as it was
            *	given rather than as the hider split it.
            */
        v3d::render::offline::trace::Scene & traced();

        /**
            *	maps to RiPolygon()
            *	polygon will be placed into a starting bucket when it is initially added
            */
        void addPolygon(const boost::shared_ptr<Polygon>& poly);
        /**
            *	maps to RiSphere()
            *	Only the ray hider draws one, intersected where it is defined; the reyes
            *	hider does not dice spheres. A sphere whose radius is not positive is
            *	logged and not drawn.
            *
            *	@return false when the hider cannot draw spheres
            */
        bool addSphere(float radius, float zmin, float zmax, float thetamax);

        /**
            *	Get the matrix for a named coordinate system.
            */
        glm::mat4x4 coordinateSystem(const std::string & name);

        /**
            *	maps to RiDeclare()
            *	What the C entry points type a parameter by. The reader keeps its own for
            *	the file it is reading; this one is the other path's, and the two are
            *	separate because a file and a program are separate scenes.
            */
        v3d::render::offline::rib::Declarations & declarations();

        /**
            *	Where this context reports what it could not do. The reader has its own for
            *	what it reads; this one is for what happens after that.
            */
        const boost::shared_ptr<v3d::log::Logger> & logger() const;

        /**
            *	The buckets the world was prepared into. Null until prepareWorld().
            */
        boost::shared_ptr<FrameBuffer> framebuffer() const;
        /**
            *	The frame's samples, which the hider writes into during render(). Placed
            *	when render() begins, from sampling() as it stands then.
            */
        Samples & samples();

        unsigned int bucketWidth() const;
        unsigned int bucketHeight() const;
        unsigned int gridSize() const;
        float shadingRate() const;

        /**
            *	How the frame is sampled: what RiPixelSamples, RiPixelFilter,
            *	RiPixelVariance, RiShutter and RiDepthOfField asked for.
            */
        v3d::render::offline::Sampling & sampling();
        const v3d::render::offline::Sampling & sampling() const;
        /**
            *	How many samples a pixel took in the last render under the ray hider, or
            *	zero under the reyes hider, which takes what PixelSamples names.
            */
        unsigned int samplesTaken(unsigned int column, unsigned int row) const;

 protected:
        void initialize();

 private:
        /** Save as "screen" the projection appended to the transformation RiProjection saw. **/
        void screenTransform();
        /** The projection the named projection, field of view, screen window and clipping make. **/
        glm::mat4x4 projectionMatrix() const;
        /**
            *	Add a primitive the scene gave to the traced scene, as triangles placed by
            *	the current transformation and shaded as the hider will shade it.
            */
        void trace(const Polygon & poly, const Shading & state);
        /** The surface, opacity and lights a traced primitive made now is shaded by. **/
        void shade(v3d::render::offline::trace::Primitive* primitive, const Shading & state);
        /** The lights in a hider's state, placed in world space, as one shared set. **/
        const v3d::render::offline::trace::Lights & tracedLights(const Shading & state);

        /*
            Every option carries the default the RI standard gives it. They are stated here
            rather than in a constructor initialiser list because there are two constructors
            and a member set by only one of them reads as a default while being indeterminate.
            RI_EPSILON and RI_INFINITY are 1.0e-10 and 1.0e38; RenderMan.h is the C interface
            and is deliberately not included here.
        */
        /**
            *	What RiAttributeBegin saves and RiAttributeEnd puts back.
            */
        class Attributes {
         public:
            v3d::render::offline::MovingTransform transform;
            glm::vec3 color = glm::vec3(1.0f);
            glm::vec3 opacity = glm::vec3(1.0f);
            float shadingRate = 1.0f;
            v3d::render::offline::sl::InstancePtr surface;
            glm::mat4x4 surfacePlacement = glm::mat4x4(1.0f);
            /**
                *	Which lights are switched on, by handle. The lights themselves are the
                *	frame's; this is the part of them an AttributeEnd puts back.
                */
            std::vector<std::string> lit;
        };

        /**
            *	A light the scene created, kept for the frame under the handle a later
            *	Illuminate names it by.
            */
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
            *	Whether the current transformation moves and is flat at both ends, so a
            *	primitive under it has no pose to be moved from and is left out. The first one
            *	is logged.
            */
        bool flatMotion();
        boost::shared_ptr<v3d::render::offline::sl::ShaderLibrary> shaders_;
        boost::shared_ptr<v3d::render::offline::Textures> textures_;
        boost::shared_ptr<GridShader> shader_;
        std::vector<LightSource> lights_;
        v3d::render::offline::sl::InstancePtr surface_;
        glm::mat4x4 surfacePlacement_ = glm::mat4x4(1.0f);
        std::vector<std::string> lit_;
        v3d::render::offline::trace::Scene traced_;
        /** The lights that are on, in world space, and what that set was built from. **/
        v3d::render::offline::trace::Lights tracedLights_;
        std::vector<std::string> tracedFor_;
        std::size_t tracedLightsFor_ = 0;
        v3d::render::offline::sl::InstancePtr imager_;
        boost::shared_ptr<FrameBuffer> frameBuffer_;
        boost::shared_ptr<Samples> samples_;
        // camera options
        unsigned int xres_ = 320;
        unsigned int yres_ = 240;
        float pixelAspect_ = 1.0f;
        float crop_[4] = { 0.0f, 1.0f, 0.0f, 1.0f };  // region of raster that is rendered
        float frameAspect_ = 4.0f / 3.0f;
        float screen_[4] = { -4.0f / 3.0f, 4.0f / 3.0f, -1.0f, 1.0f };  // screen coordinates, after projection, of the area to be rendered
        // whichever of these a scene named explicitly stops following the one above it:
        // RiFormat sets the frame aspect, which sets the screen window, and either can be
        // overridden without the other reverting it
        bool frameAspectNamed_ = false;
        bool screenNamed_ = false;
        bool projectionNamed_ = false;
        glm::vec3 color_ = glm::vec3(1.0f);
        glm::vec3 opacity_ = glm::vec3(1.0f);
        std::string projection_ = "orthographic";
        float fov_ = 90.0f;
        boost::shared_ptr<Hider> hider_;
        // display options. An empty name is no output, which is the RI default of a
        // framebuffer this renderer does not have
        std::string displayName_;
        std::string displayType_;
        std::string displayMode_;
        // world to camera transformation matrix / current transformation matrix, which a
        // motion block makes move
        v3d::render::offline::MovingTransform transform_;
        // the transformation in force at RiProjection, which the projection is appended to
        v3d::render::offline::MovingTransform projectionBase_;
        float near_ = 1.0e-10f;  // near clipping plane
        float far_ = 1.0e38f;  // far clipping plane
        v3d::render::offline::Sampling sampling_;


        /*
            the framebuffer is divided up into buckets of n-by-m size
            minimum grid size is:
                bucket size / shading rate = grid size
                e.g.
                    12x12/4=36
                    16x16/1=256
            the bucket size is specified in separate width and height values
            shading rate: smaller values = higher frequency shading rate (higher quality)
            shading rate = 1 = 1 sample per pixel = 256 grid size = 16x16 micropolys
            shading rate = .25 = 4 samples per pixel = 1024 grid size = 32x32 micropolys
            pixel size of grid is?
            32x32 * .25 = 8x8 pixel grid
            16x16 * 1 = 16x16 pixel grid
        */
        unsigned int bucketWidth_ = 16;
        unsigned int bucketHeight_ = 16;
        unsigned int gridSize_ = 256;
        std::size_t motionCache_ = std::size_t(1) << 20;
        float shadingRate_ = 1.0f;
};

/**
 *	A point through a matrix that may be a perspective one, with the divide.
 *
 *	An orthographic projection leaves w at one and the divide changes nothing; a
 *	perspective one writes the eye depth into w, and a point taken without dividing is
 *	then in eye units where the caller wanted pixels. A w at or behind the eye is left
 *	undivided - the primitive it came from is marked undiceable and split before it is
 *	sampled.
 */
glm::vec3 project(const glm::mat4x4 & m, const glm::vec3 & point);

};  // namespace v3d::moya
