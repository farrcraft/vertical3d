/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>
#include <api/render/offline/rib/Declarations.h>
#include <api/render/offline/sl/ShaderLibrary.h>

#include "Polygon.h"
#include "FrameBuffer.h"
#include "Shading.h"

#include <vector>
#include <map>
#include <string>

namespace v3d::moya {

class GridShader;

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
            *	imageResolution() from the pixel resolution unless this named one, which
            *	is what makes RiFormat and RiFrameAspectRatio independent.
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
            *	maps to RiDisplay()
            *	names where the render's samples go once the buckets are done. A type of
            *	"file" is what render() writes through image::Factory, which picks the
            *	format from the name's extension.
            */
        void display(const std::string & name, const std::string & type, const std::string & mode);
        const std::string & displayName() const;

        /**
            *	maps to RiTransformBegin() and RiTransformEnd()
            *	pop restores what push saved, which is what makes the pair a bracket
            *	rather than a discard.
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
            *	maps to RiSurface()
            *	the shader a primitive added from here on is shaded by. A scene that names
            *	none draws "constant", which is the shader that means no shading and is
            *	the picture this renderer drew before there was a language.
            */
        void surface(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters);
        /**
            *	maps to RiLightSource()
            *	creates a light and switches it on in the current attribute state. The
            *	light itself belongs to the frame; which lights are on is an attribute,
            *	which is what makes Illuminate inside an AttributeBegin block local to it.
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
            *	The surface shader and the lights a primitive submitted now is shaded by.
            */
        Shading shading();

        /**
            *	What runs a surface shader over a grid.
            *
            *	Kept for the render rather than made per grid, because it holds the
            *	register files: a thousand grids over one program size one once.
            */
        GridShader & shader();

        /**
            *	maps to RiPolygon()
            *	polygon will be placed into a starting bucket when it is initially added
            */
        void addPolygon(const boost::shared_ptr<Polygon>& poly);

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
            *	Where this context says what it could not do. The reader has its own for
            *	what it reads; this one is for what happens after that.
            */
        const boost::shared_ptr<v3d::log::Logger> & logger() const;

        /**
            *	The buckets the world was prepared into. Null until prepareWorld().
            */
        boost::shared_ptr<FrameBuffer> framebuffer() const;

        unsigned int bucketWidth() const;
        unsigned int bucketHeight() const;
        unsigned int gridSize() const;
        float shadingRate() const;

 protected:
        void initialize();

 private:
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
            glm::mat4x4 transform = glm::mat4x4(1.0f);
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
        std::vector<glm::mat4x4> transforms_;
        std::vector<Attributes> attributes_;
        std::map<std::string, glm::mat4x4> coordinateSystems_;
        v3d::render::offline::rib::Declarations declarations_;
        boost::shared_ptr<v3d::log::Logger> logger_;
        boost::shared_ptr<v3d::render::offline::sl::ShaderLibrary> shaders_;
        boost::shared_ptr<GridShader> shader_;
        std::vector<LightSource> lights_;
        v3d::render::offline::sl::InstancePtr surface_;
        glm::mat4x4 surfacePlacement_ = glm::mat4x4(1.0f);
        std::vector<std::string> lit_;
        v3d::render::offline::sl::InstancePtr imager_;
        boost::shared_ptr<FrameBuffer> frameBuffer_;
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
        // display options. An empty name is no output, which is the RI default of a
        // framebuffer this renderer does not have
        std::string displayName_;
        std::string displayType_;
        std::string displayMode_;
        glm::mat4x4 transform_ = glm::mat4x4(1.0f);  // world to camera transformation matrix / current transformation matrix
        float near_ = 1.0e-10f;  // near clipping plane
        float far_ = 1.0e38f;  // far clipping plane
        // other clipping planes
        float fStop_ = 1.0e38f;  // for depth of field
        float focalLength_ = 0.0f;
        float focalDistance_ = 0.0f;
        float shutterOpen_ = 0.0f;
        float shutterClose_ = 0.0f;


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
