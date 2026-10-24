/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/log/Logger.h>
#include <api/render/offline/Sampling.h>
#include <api/render/offline/rib/Handler.h>
#include <api/render/offline/rib/Parameters.h>
#include <api/render/offline/rib/Reader.h>
#include <api/render/offline/rib/Requests.h>

#include <cmath>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

namespace {

/**
 * Counts the requests it receives, so the reader can be tested without a renderer.
 **/
class CountingHandler final : public v3d::render::offline::rib::Handler {
 public:
    void version(float number) override {
        version_ = number;
        counts_["version"]++;
    }
    void declare(const v3d::render::offline::rib::Declare & request) override {
        declared_.push_back(request.name);
        counts_["Declare"]++;
    }
    void option(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) override {
        (void)name;
        bucket_ = parameters.floats("bucketsize");
        if (parameters.has("texture")) {
            texturePath_ = parameters.string("texture");
        }
        counts_["Option"]++;
    }
    void hider(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) override {
        (void)parameters;
        hider_ = name;
        counts_["Hider"]++;
    }
    void format(const v3d::render::offline::rib::Format & request) override {
        width_ = request.width;
        height_ = request.height;
        pixelAspect_ = request.pixelAspect;
        counts_["Format"]++;
    }
    void projection(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) override {
        projection_ = name;
        fov_ = parameters.number("fov", 90.0F);
        counts_["Projection"]++;
    }
    void clipping(const v3d::render::offline::rib::Clipping & request) override {
        hither_ = request.hither;
        yon_ = request.yon;
        counts_["Clipping"]++;
    }
    void display(const v3d::render::offline::rib::Display & request, const v3d::render::offline::rib::ParameterList & parameters) override {
        (void)parameters;
        display_ = request.name + "|" + request.type + "|" + request.mode;
        counts_["Display"]++;
    }
    void depthOfField(const v3d::render::offline::rib::DepthOfField & request) override {
        lens_ = glm::vec3(request.fstop, request.focalLength, request.focalDistance);
        counts_["DepthOfField"]++;
    }
    void shutter(const v3d::render::offline::rib::Shutter & request) override {
        shutter_ = glm::vec2(request.open, request.close);
        counts_["Shutter"]++;
    }
    void pixelSamples(const glm::uvec2 & samples) override {
        samples_ = samples;
        counts_["PixelSamples"]++;
    }
    void pixelFilter(v3d::render::offline::Filter filter, float xwidth, float ywidth) override {
        filter_ = filter;
        filterWidth_ = glm::vec2(xwidth, ywidth);
        counts_["PixelFilter"]++;
    }
    void pixelVariance(float variation) override {
        variance_ = variation;
        counts_["PixelVariance"]++;
    }
    void motionBegin(const std::vector<float> & times) override {
        motionTimes_ = times;
        counts_["MotionBegin"]++;
    }
    void motionEnd() override { counts_["MotionEnd"]++; }
    void frameBegin(int frame) override {
        frame_ = frame;
        counts_["FrameBegin"]++;
    }
    void frameEnd() override { counts_["FrameEnd"]++; }
    void worldBegin() override { counts_["WorldBegin"]++; }
    void worldEnd() override { counts_["WorldEnd"]++; }
    void attributeBegin() override { counts_["AttributeBegin"]++; }
    void attributeEnd() override { counts_["AttributeEnd"]++; }
    void transform(const glm::mat4x4 & matrix) override {  // NOLINT(build/include_what_you_use)
        transform_ = matrix;
        counts_["Transform"]++;
    }
    void concatTransform(const glm::mat4x4 & matrix) override {
        transform_ = matrix;
        counts_["ConcatTransform"]++;
    }
    void translate(float dx, float dy, float dz) override {
        translate_ = glm::vec3(dx, dy, dz);
        counts_["Translate"]++;
    }
    void color(const glm::vec3 & value) override {
        color_ = value;
        counts_["Color"]++;
    }
    void surface(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) override {
        surface_ = name;
        roughness_ = parameters.number("roughness", -1.0F);
        counts_["Surface"]++;
    }
    void displacement(const std::string & name, const v3d::render::offline::rib::ParameterList & /* parameters */) override {
        displacement_ = name;
        counts_["Displacement"]++;
    }
    void lightSource(const v3d::render::offline::rib::LightSource & request, const v3d::render::offline::rib::ParameterList & parameters) override {
        lights_.push_back(request.shader + "|" + request.handle + "|" +
            std::to_string(parameters.number("intensity", -1.0F)));
        counts_["LightSource"]++;
    }
    void illuminate(const std::string & handle, bool on) override {
        illuminated_.push_back(handle + (on ? "|on" : "|off"));
        counts_["Illuminate"]++;
    }
    void imager(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) override {
        imager_ = name;
        background_ = parameters.points("background");
        counts_["Imager"]++;
    }
    void attribute(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) override {
        (void)name;
        identifier_ = parameters.string("name");
        counts_["Attribute"]++;
    }
    void polygon(unsigned int vertices, const v3d::render::offline::rib::ParameterList & parameters) override {
        vertices_ = vertices;
        points_ = parameters.points("P");
        colors_ = parameters.points("Cs");
        counts_["Polygon"]++;
    }
    void pointsPolygons(const v3d::render::offline::rib::PointsPolygons & request, const v3d::render::offline::rib::ParameterList & parameters) override {
        perPolygon_ = request.counts;
        indices_ = request.indices;
        points_ = parameters.points("P");
        counts_["PointsPolygons"]++;
    }
    void sphere(const v3d::render::offline::rib::Sphere & request, const v3d::render::offline::rib::ParameterList & parameters) override {
        (void)parameters;
        radius_ = request.radius;
        counts_["Sphere"]++;
    }

    unsigned int count(const std::string & name) const {
        const std::map<std::string, unsigned int>::const_iterator found = counts_.find(name);
        return found == counts_.end() ? 0U : found->second;
    }

    std::map<std::string, unsigned int> counts_;
    std::vector<std::string> declared_;
    std::vector<std::string> lights_;
    std::vector<std::string> illuminated_;
    std::vector<glm::vec3> background_;
    std::vector<float> bucket_;
    std::string texturePath_;
    std::vector<float> motionTimes_;
    std::vector<glm::vec3> points_;
    std::vector<glm::vec3> colors_;
    std::vector<unsigned int> perPolygon_;
    std::vector<unsigned int> indices_;
    glm::mat4x4 transform_ = glm::mat4x4(1.0F);
    glm::vec3 translate_ = glm::vec3(0.0F);
    glm::vec3 color_ = glm::vec3(0.0F);
    glm::vec3 lens_ = glm::vec3(0.0F);
    glm::vec2 shutter_ = glm::vec2(0.0F);
    glm::vec2 filterWidth_ = glm::vec2(0.0F);
    glm::uvec2 samples_ = glm::uvec2(0);
    v3d::render::offline::Filter filter_ = v3d::render::offline::Filter::Box;
    float variance_ = 0.0F;
    std::string projection_;
    std::string display_;
    std::string surface_;
    std::string displacement_;
    std::string imager_;
    std::string hider_;
    std::string identifier_;
    float version_ = 0.0F;
    float pixelAspect_ = 0.0F;
    float hither_ = 0.0F;
    float yon_ = 0.0F;
    float fov_ = 0.0F;
    float roughness_ = 0.0F;
    float radius_ = 0.0F;
    unsigned int width_ = 0;
    unsigned int height_ = 0;
    unsigned int vertices_ = 0;
    int frame_ = 0;
};

bool read(const std::string & source, CountingHandler * handler, v3d::render::offline::rib::Reader * reader) {
    std::istringstream stream(source);
    return reader->read(stream, handler);
}

BOOST_AUTO_TEST_CASE(ribreader_camera_requests_test) {
    CountingHandler handler;
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(read(
        "version 3.03\n"
        "Format 640 480 1\n"
        "Projection \"perspective\" \"fov\" 45\n"
        "Clipping 1 100\n"
        "Display \"out.png\" \"file\" \"rgb\"\n"
        "FrameBegin 1\n"
        "WorldBegin\n"
        "WorldEnd\n"
        "FrameEnd\n", &handler, &reader));

    BOOST_CHECK_CLOSE(handler.version_, 3.03F, 0.01F);
    BOOST_CHECK_EQUAL(handler.width_, 640U);
    BOOST_CHECK_EQUAL(handler.height_, 480U);
    BOOST_CHECK_EQUAL(handler.pixelAspect_, 1.0F);
    BOOST_CHECK_EQUAL(handler.projection_, "perspective");
    BOOST_CHECK_EQUAL(handler.fov_, 45.0F);
    BOOST_CHECK_EQUAL(handler.hither_, 1.0F);
    BOOST_CHECK_EQUAL(handler.yon_, 100.0F);
    BOOST_CHECK_EQUAL(handler.display_, "out.png|file|rgb");
    BOOST_CHECK_EQUAL(handler.frame_, 1);
    BOOST_CHECK_EQUAL(handler.count("WorldBegin"), 1U);
    BOOST_CHECK_EQUAL(handler.count("WorldEnd"), 1U);
}

/**
 * RIB carries no vertex count. The count is the length of the position array.
 **/
BOOST_AUTO_TEST_CASE(ribreader_polygon_test) {
    CountingHandler handler;
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(read(
        "Polygon \"P\" [-1 -1 5  1 -1 5  1 1 5  -1 1 5]\n", &handler, &reader));

    BOOST_CHECK_EQUAL(handler.count("Polygon"), 1U);
    BOOST_CHECK_EQUAL(handler.vertices_, 4U);
    BOOST_REQUIRE_EQUAL(handler.points_.size(), 4U);
    BOOST_CHECK_EQUAL(handler.points_[2].x, 1.0F);
    BOOST_CHECK_EQUAL(handler.points_[2].y, 1.0F);
    BOOST_CHECK_EQUAL(handler.points_[2].z, 5.0F);
}

/**
 * A varying parameter carries one element per vertex, so a colour per corner reads back as
 * four colours, although the file never states the count.
 **/
BOOST_AUTO_TEST_CASE(ribreader_varying_parameter_test) {
    CountingHandler handler;
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(read(
        "Polygon \"P\" [0 0 1  1 0 1  1 1 1]\n"
        "        \"Cs\" [1 0 0  0 1 0  0 0 1]\n", &handler, &reader));

    BOOST_CHECK_EQUAL(handler.vertices_, 3U);
    BOOST_REQUIRE_EQUAL(handler.colors_.size(), 3U);
    BOOST_CHECK_EQUAL(handler.colors_[2].z, 1.0F);
}

BOOST_AUTO_TEST_CASE(ribreader_points_polygons_test) {
    CountingHandler handler;
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(read(
        "PointsPolygons [3 3] [0 1 2  0 2 3] \"P\" [0 0 0  1 0 0  1 1 0  0 1 0]\n", &handler, &reader));

    BOOST_REQUIRE_EQUAL(handler.perPolygon_.size(), 2U);
    BOOST_CHECK_EQUAL(handler.perPolygon_[0], 3U);
    BOOST_REQUIRE_EQUAL(handler.indices_.size(), 6U);
    BOOST_CHECK_EQUAL(handler.indices_[4], 2U);
    BOOST_CHECK_EQUAL(handler.points_.size(), 4U);
}

/**
 * An unbracketed value is legal for a uniform parameter. Only the declaration table gives
 * how many values it takes.
 **/
BOOST_AUTO_TEST_CASE(ribreader_unbracketed_parameter_test) {
    CountingHandler handler;
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(read("Surface \"plastic\" \"roughness\" .3\n", &handler, &reader));

    BOOST_CHECK_EQUAL(handler.surface_, "plastic");
    BOOST_CHECK_CLOSE(handler.roughness_, 0.3F, 0.01F);
}

BOOST_AUTO_TEST_CASE(ribreader_declare_then_use_test) {
    CountingHandler handler;
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(read(
        "Declare \"squish\" \"uniform float\"\n"
        "Surface \"marble\" \"squish\" 5\n", &handler, &reader));

    BOOST_REQUIRE_EQUAL(handler.declared_.size(), 1U);
    BOOST_CHECK_EQUAL(handler.declared_[0], "squish");
    BOOST_CHECK_EQUAL(handler.count("Surface"), 1U);
    BOOST_CHECK(reader.error().empty());
}

/**
 * An undeclared parameter with an array is recoverable, because the array bounds itself. One
 * without an array is not, and is reported as an error rather than reading the next request
 * as a value.
 **/
BOOST_AUTO_TEST_CASE(ribreader_undeclared_parameter_test) {
    CountingHandler bracketed;
    v3d::render::offline::rib::Reader first(boost::make_shared<v3d::log::Logger>());
    BOOST_CHECK(read("Surface \"marble\" \"veins\" [1 2 3]\nWorldBegin\n", &bracketed, &first));
    BOOST_CHECK_EQUAL(bracketed.count("Surface"), 1U);
    BOOST_CHECK_EQUAL(bracketed.count("WorldBegin"), 1U);

    CountingHandler bare;
    v3d::render::offline::rib::Reader second(boost::make_shared<v3d::log::Logger>());
    BOOST_CHECK(!read("Surface \"marble\" \"veins\" 3\n", &bare, &second));
    BOOST_CHECK(second.error().contains("undeclared parameter 'veins'"));
}

/**
 * The requests that say how a pixel is sampled reach the handler with their arguments, and
 * a sample rate is a count by the time it does.
 **/
BOOST_AUTO_TEST_CASE(ribreader_sampling_requests_test) {
    CountingHandler handler;
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(read(
        "PixelSamples 4 2.6\n"
        "PixelFilter \"catmull-rom\" 3 4\n"
        "PixelVariance 0.05\n"
        "Shutter 0 0.5\n"
        "DepthOfField 8 0.1 3\n"
        "WorldBegin\n"
        "WorldEnd\n", &handler, &reader));

    BOOST_CHECK(reader.unrecognised().empty());
    BOOST_CHECK_EQUAL(handler.samples_.x, 4U);
    BOOST_CHECK_EQUAL(handler.samples_.y, 3U);
    BOOST_CHECK(handler.filter_ == v3d::render::offline::Filter::CatmullRom);
    BOOST_CHECK_EQUAL(handler.filterWidth_.x, 3.0F);
    BOOST_CHECK_EQUAL(handler.filterWidth_.y, 4.0F);
    BOOST_CHECK_CLOSE(handler.variance_, 0.05F, 1.0e-4F);
    BOOST_CHECK_EQUAL(handler.shutter_.x, 0.0F);
    BOOST_CHECK_EQUAL(handler.shutter_.y, 0.5F);
    BOOST_CHECK_EQUAL(handler.lens_.x, 8.0F);
    BOOST_CHECK_CLOSE(handler.lens_.y, 0.1F, 1.0e-4F);
    BOOST_CHECK_EQUAL(handler.lens_.z, 3.0F);
}

/**
 * A filter the reader does not recognise is reported and never reaches the handler, so the
 * renderer keeps the filter it had. The rest of the scene still reads.
 **/
BOOST_AUTO_TEST_CASE(ribreader_unknown_filter_test) {
    CountingHandler handler;
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(read(
        "PixelFilter \"mitchell\" 2 2\n"
        "PixelSamples 1 1\n", &handler, &reader));

    BOOST_CHECK_EQUAL(handler.count("PixelFilter"), 0U);
    BOOST_CHECK_EQUAL(handler.count("PixelSamples"), 1U);
    BOOST_CHECK(reader.unrecognised().empty());
}

/**
 * RIB's DepthOfField with no arguments sets a pinhole, which RI writes as an infinite
 * fstop, and a sample rate below one still takes a sample.
 **/
BOOST_AUTO_TEST_CASE(ribreader_pinhole_and_rate_test) {
    CountingHandler handler;
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(read(
        "DepthOfField\n"
        "PixelSamples 0.25 0\n", &handler, &reader));

    BOOST_CHECK_EQUAL(handler.count("DepthOfField"), 1U);
    BOOST_CHECK(std::isinf(handler.lens_.x));
    BOOST_CHECK_EQUAL(handler.samples_.x, 1U);
    BOOST_CHECK_EQUAL(handler.samples_.y, 1U);

    // a rate too large for a pixel's grid of samples, or for the conversion, is capped
    BOOST_REQUIRE(read("PixelSamples 65536 1e39\n", &handler, &reader));
    BOOST_CHECK_EQUAL(handler.samples_.x, v3d::render::offline::maximumSamples);
    BOOST_CHECK_EQUAL(handler.samples_.y, v3d::render::offline::maximumSamples);

    v3d::render::offline::Sampling sampling;
    sampling.fstop = handler.lens_.x;
    sampling.focalLength = 0.1F;
    sampling.focalDistance = 3.0F;
    BOOST_CHECK(sampling.pinhole());
    sampling.fstop = 8.0F;
    BOOST_CHECK(!sampling.pinhole());
}

/**
 * An unrecognised request is reported once per name and its arguments are skipped, so the
 * request after it is still read.
 **/
BOOST_AUTO_TEST_CASE(ribreader_unrecognised_request_test) {
    CountingHandler handler;
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(read(
        "Sides 2\n"
        "Exterior \"MyShader\" \"squish\" [5]\n"
        "Sides 1\n"
        "WorldBegin\n"
        "Polygon \"P\" [0 0 1  1 0 1  1 1 1]\n"
        "WorldEnd\n", &handler, &reader));

    BOOST_REQUIRE_EQUAL(reader.unrecognised().size(), 2U);
    BOOST_CHECK_EQUAL(reader.unrecognised()[0], "Sides");
    BOOST_CHECK_EQUAL(reader.unrecognised()[1], "Exterior");
    // everything after them still arrived
    BOOST_CHECK_EQUAL(handler.count("WorldBegin"), 1U);
    BOOST_CHECK_EQUAL(handler.vertices_, 3U);
}

/**
 * MakeTexture is recognised and makes nothing, because the image a scene names is the
 * texture. The texture search path reaches the renderer as a string.
 **/
BOOST_AUTO_TEST_CASE(ribreader_texture_requests_test) {
    CountingHandler handler;
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(read(
        "Option \"searchpath\" \"texture\" [\"maps:&\"]\n"
        "MakeTexture \"grid.png\" \"grid.tx\" \"periodic\" \"periodic\" \"gaussian\" 2 2 \"float fov\" [1]\n"
        "WorldBegin\n"
        "Polygon \"P\" [0 0 1  1 0 1  1 1 1]\n"
        "WorldEnd\n", &handler, &reader));

    BOOST_CHECK(reader.unrecognised().empty());
    BOOST_CHECK_EQUAL(handler.texturePath_, "maps:&");
    BOOST_CHECK_EQUAL(handler.vertices_, 3U);
}

/**
 * A matrix read in RIB's order needs no transpose to become glm's. The test checks the
 * convention by applying the result to a point rather than comparing storage.
 **/
BOOST_AUTO_TEST_CASE(ribreader_transform_convention_test) {
    CountingHandler handler;
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(read(
        "Transform [1 0 0 0  0 1 0 0  0 0 1 0  2 3 4 1]\n", &handler, &reader));

    const glm::vec4 moved = handler.transform_ * glm::vec4(0.0F, 0.0F, 0.0F, 1.0F);
    BOOST_CHECK_EQUAL(moved.x, 2.0F);
    BOOST_CHECK_EQUAL(moved.y, 3.0F);
    BOOST_CHECK_EQUAL(moved.z, 4.0F);
}

BOOST_AUTO_TEST_CASE(ribreader_graphics_state_test) {
    CountingHandler handler;
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(read(
        "AttributeBegin\n"
        "Color [0.9 0.2 0.1]\n"
        "Translate 1 2 3\n"
        "Attribute \"identifier\" \"name\" [\"myball\"]\n"
        "Sphere .5 0 .5 360\n"
        "AttributeEnd\n", &handler, &reader));

    BOOST_CHECK_EQUAL(handler.count("AttributeBegin"), 1U);
    BOOST_CHECK_EQUAL(handler.count("AttributeEnd"), 1U);
    BOOST_CHECK_CLOSE(handler.color_.r, 0.9F, 0.01F);
    BOOST_CHECK_EQUAL(handler.translate_.z, 3.0F);
    BOOST_CHECK_EQUAL(handler.identifier_, "myball");
    BOOST_CHECK_EQUAL(handler.radius_, 0.5F);
}

/**
 * A parse error gives the problem and its position, rather than leaving a scene that silently
 * rendered nothing.
 **/
BOOST_AUTO_TEST_CASE(ribreader_error_position_test) {
    CountingHandler handler;
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_CHECK(!read("Format 640 480 1\nClipping 1 \"near\"\n", &handler, &reader));
    BOOST_CHECK(reader.error().contains("expected a number"));
    BOOST_CHECK(reader.error().contains("line 2"));
}

BOOST_AUTO_TEST_CASE(ribreader_missing_file_test) {
    CountingHandler handler;
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_CHECK(!reader.read("data/no-such-scene.rib", &handler));
    BOOST_CHECK(reader.error().contains("could not open"));
}

/**
 * The standard's own example file end to end: two frames, nested attribute blocks, a matrix
 * spanning lines, structure comments, an inline declaration and unbracketed values.
 **/
BOOST_AUTO_TEST_CASE(ribreader_example_file_test) {
    CountingHandler handler;
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(reader.read("data/example.rib", &handler));
    BOOST_CHECK_EQUAL(reader.error(), "");

    BOOST_CHECK_EQUAL(handler.count("FrameBegin"), 2U);
    BOOST_CHECK_EQUAL(handler.count("FrameEnd"), 2U);
    BOOST_CHECK_EQUAL(handler.count("WorldBegin"), 2U);
    BOOST_CHECK_EQUAL(handler.count("Transform"), 2U);
    BOOST_CHECK_EQUAL(handler.count("Sphere"), 4U);
    BOOST_CHECK_EQUAL(handler.count("Polygon"), 1U);
    BOOST_CHECK_EQUAL(handler.count("Option"), 2U);
    BOOST_CHECK_EQUAL(handler.count("Declare"), 2U);
    BOOST_CHECK_EQUAL(handler.count("LightSource"), 3U);
    BOOST_CHECK_EQUAL(handler.count("Illuminate"), 2U);
    BOOST_CHECK_EQUAL(handler.count("Imager"), 1U);
    // the floor polygon is the only geometry with vertices in the file
    BOOST_CHECK_EQUAL(handler.vertices_, 4U);

    // the file's displacement shader reaches the handler with its name
    BOOST_CHECK_EQUAL(handler.count("Displacement"), 1U);
    BOOST_CHECK_EQUAL(handler.displacement_, "MyUserShader");
}

/**
 * A light carries a handle, and a later Illuminate names it by that handle. RIB 3.03
 * writes the handle as a sequence number and later RIB writes a string. Both reach the
 * handler as a string.
 **/
BOOST_AUTO_TEST_CASE(ribreader_light_handles_test) {
    CountingHandler handler;
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(read(
        "LightSource \"distantlight\" 1 \"intensity\" [2]\n"
        "LightSource \"pointlight\" \"fill\" \"intensity\" [0.5]\n"
        "Illuminate 1 0\n"
        "Illuminate \"fill\" 1\n", &handler, &reader));

    BOOST_REQUIRE_EQUAL(handler.lights_.size(), 2U);
    BOOST_CHECK_EQUAL(handler.lights_[0], "distantlight|1|2.000000");
    BOOST_CHECK_EQUAL(handler.lights_[1], "pointlight|fill|0.500000");
    BOOST_REQUIRE_EQUAL(handler.illuminated_.size(), 2U);
    BOOST_CHECK_EQUAL(handler.illuminated_[0], "1|off");
    BOOST_CHECK_EQUAL(handler.illuminated_[1], "fill|on");
}

/**
 * An area light reaches a handler that does not override areaLightSource() as an ordinary
 * light, so a scene using one is still lit.
 **/
BOOST_AUTO_TEST_CASE(ribreader_area_light_test) {
    CountingHandler handler;
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(read("AreaLightSource \"arealight\" 3 \"intensity\" [4]\n", &handler, &reader));
    BOOST_REQUIRE_EQUAL(handler.lights_.size(), 1U);
    BOOST_CHECK_EQUAL(handler.lights_[0], "arealight|3|4.000000");
}

/**
 * Imager names the shader run over the finished frame, and reaches the handler with its
 * parameters.
 **/
BOOST_AUTO_TEST_CASE(ribreader_imager_test) {
    CountingHandler handler;
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(read("Imager \"background\" \"background\" [0.1 0.2 0.3]\n", &handler, &reader));
    BOOST_CHECK_EQUAL(handler.imager_, "background");
    BOOST_REQUIRE_EQUAL(handler.background_.size(), 1U);
    BOOST_CHECK_CLOSE(handler.background_[0].b, 0.3F, 0.01F);
}

/**
 * Hider names how the renderer decides what the camera sees, and reaches the handler by name.
 **/
BOOST_AUTO_TEST_CASE(ribreader_hider_test) {
    CountingHandler handler;
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(read("Hider \"raytrace\"\n", &handler, &reader));
    BOOST_CHECK_EQUAL(handler.count("Hider"), 1U);
    BOOST_CHECK_EQUAL(handler.hider_, "raytrace");
    BOOST_CHECK(reader.unrecognised().empty());
}

/**
 * A motion block's times reach the handler, and so does every transform request inside it.
 * Each request is the transformation at one of the times.
 **/
BOOST_AUTO_TEST_CASE(ribreader_motion_block_test) {
    CountingHandler handler;
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(read(
        "WorldBegin\n"
        "MotionBegin [0 0.5]\n"
        "Translate 0 0 0\n"
        "Translate 1 0 0\n"
        "MotionEnd\n"
        "Polygon \"P\" [0 0 1  1 0 1  1 1 1]\n"
        "WorldEnd\n", &handler, &reader));

    BOOST_CHECK_EQUAL(handler.count("MotionBegin"), 1U);
    BOOST_CHECK_EQUAL(handler.count("MotionEnd"), 1U);
    BOOST_CHECK_EQUAL(handler.count("Translate"), 2U);
    BOOST_REQUIRE_EQUAL(handler.motionTimes_.size(), 2U);
    BOOST_CHECK_EQUAL(handler.motionTimes_[1], 0.5F);
    BOOST_CHECK_EQUAL(handler.count("Polygon"), 1U);
    BOOST_CHECK(reader.unsupported().empty());
}

/**
 * A primitive repeated inside a motion block deforms, and the default handler does not build a
 * deforming primitive. The first pose reaches the handler and the second is read and reported.
 * The request after the block still reads.
 **/
BOOST_AUTO_TEST_CASE(ribreader_deforming_motion_test) {
    CountingHandler handler;
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(read(
        "WorldBegin\n"
        "MotionBegin [0 1]\n"
        "Polygon \"P\" [0 0 1  1 0 1  1 1 1]\n"
        "Polygon \"P\" [0 0 2  2 0 2  2 2 2  0 2 2]\n"
        "MotionEnd\n"
        "Color [0.5 0.5 0.5]\n"
        "WorldEnd\n", &handler, &reader));

    BOOST_CHECK_EQUAL(handler.count("Polygon"), 1U);
    BOOST_CHECK_EQUAL(handler.vertices_, 3U);
    BOOST_REQUIRE_EQUAL(reader.unsupported().size(), 1U);
    BOOST_CHECK_EQUAL(reader.unsupported()[0], "deforming Polygon");
    BOOST_CHECK_EQUAL(handler.count("Color"), 1U);
}

namespace {

/**
 * A handler that overrides the Handler defaults to sample an area light, make a texture, and
 * build a primitive that deforms.
 **/
class CapableHandler final : public v3d::render::offline::rib::Handler {
 public:
    void areaLightSource(const v3d::render::offline::rib::LightSource & request, const v3d::render::offline::rib::ParameterList & parameters) override {
        (void)parameters;
        area_ = request.shader + "|" + request.handle;
    }
    void lightSource(const v3d::render::offline::rib::LightSource & request, const v3d::render::offline::rib::ParameterList & parameters) override {
        (void)request;
        (void)parameters;
        lights_++;
    }
    void makeTexture(const v3d::render::offline::rib::MakeTexture & request, const v3d::render::offline::rib::ParameterList & parameters) override {
        (void)parameters;
        made_ = request.picture + "|" + request.texture + "|" + request.swrap + "|" + request.twrap + "|" + request.filter + "|" +
            std::to_string(static_cast<int>(request.swidth)) + "|" + std::to_string(static_cast<int>(request.twidth));
    }
    void polygon(unsigned int vertices, const v3d::render::offline::rib::ParameterList & parameters) override {
        (void)parameters;
        vertices_.push_back(vertices);
    }
    Handler* deformation() override {
        return &later_;
    }

    /** Receives the later poses, kept apart from the first. **/
    class Later final : public v3d::render::offline::rib::Handler {
     public:
        void polygon(unsigned int vertices, const v3d::render::offline::rib::ParameterList & parameters) override {
            (void)parameters;
            vertices_.push_back(vertices);
        }
        std::vector<unsigned int> vertices_;
    };

    std::string area_;
    std::string made_;
    unsigned int lights_ = 0;
    std::vector<unsigned int> vertices_;
    Later later_;
};

};  // namespace

/**
 * A handler decides what it supports: an area light, a texture to make and a
 * deforming primitive each reach a handler that takes them, and nothing is reported.
 **/
BOOST_AUTO_TEST_CASE(ribreader_capable_handler_test) {
    CapableHandler handler;
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());
    std::istringstream stream(
        "MakeTexture \"grid.png\" \"grid.tx\" \"periodic\" \"clamp\" \"gaussian\" 2 3\n"
        "WorldBegin\n"
        "AreaLightSource \"arealight\" 3 \"intensity\" [4]\n"
        "MotionBegin [0 1]\n"
        "Polygon \"P\" [0 0 1  1 0 1  1 1 1]\n"
        "Polygon \"P\" [0 0 2  2 0 2  2 2 2  0 2 2]\n"
        "MotionEnd\n"
        "WorldEnd\n");
    BOOST_REQUIRE(reader.read(stream, &handler));

    BOOST_CHECK_EQUAL(handler.made_, "grid.png|grid.tx|periodic|clamp|gaussian|2|3");
    BOOST_CHECK_EQUAL(handler.area_, "arealight|3");
    BOOST_CHECK_EQUAL(handler.lights_, 0U);
    BOOST_CHECK((handler.vertices_ == std::vector<unsigned int>{ 3U }));
    BOOST_CHECK((handler.later_.vertices_ == std::vector<unsigned int>{ 4U }));
    BOOST_CHECK(reader.unsupported().empty());
}

/**
 * A Format, a FrameBegin or a PixelFilter width that is not a size is skipped with a warning.
 * Converting an out of range value to an unsigned count is undefined. A Format side above
 * largestResolution, infinite, or between 0 and 1 is not a size. The file still reads, and
 * the requests that are sizes still arrive.
 **/
BOOST_AUTO_TEST_CASE(ribreader_sizes_that_are_not_sizes_are_skipped_test) {
    CountingHandler handler;
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(read(
        "Format 65537 480 1\n"
        "Format 640 1e39 1\n"
        "Format 0.5 480 1\n"
        "PixelFilter \"box\" 0 1\n"
        "FrameBegin 1e30\n"
        "FrameEnd\n", &handler, &reader));
    BOOST_CHECK_EQUAL(handler.count("Format"), 0U);
    BOOST_CHECK_EQUAL(handler.filterWidth_.x, 0.0F);
    BOOST_CHECK_EQUAL(handler.count("PixelFilter"), 0U);
    BOOST_CHECK_EQUAL(handler.count("FrameBegin"), 0U);
    BOOST_CHECK_EQUAL(handler.count("FrameEnd"), 1U);

    BOOST_REQUIRE(read("Format 64 48 1\nPixelFilter \"box\" 2 2\n", &handler, &reader));
    BOOST_CHECK_EQUAL(handler.width_, 64U);
    BOOST_CHECK_EQUAL(handler.filterWidth_.x, 2.0F);
}

/**
 * A Format side of zero or less selects the renderer's default for that side, as RI reads it.
 * It arrives as 0, and the other side arrives as named.
 **/
BOOST_AUTO_TEST_CASE(ribreader_format_nonpositive_side_asks_for_the_default_test) {
    CountingHandler handler;
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(read("Format -1 480 1\n", &handler, &reader));
    BOOST_CHECK_EQUAL(handler.count("Format"), 1U);
    BOOST_CHECK_EQUAL(handler.width_, 0U);
    BOOST_CHECK_EQUAL(handler.height_, 480U);

    BOOST_REQUIRE(read("Format 640 0 1\n", &handler, &reader));
    BOOST_CHECK_EQUAL(handler.count("Format"), 2U);
    BOOST_CHECK_EQUAL(handler.width_, 640U);
    BOOST_CHECK_EQUAL(handler.height_, 0U);
}

/**
 * A pixel aspect of zero or less selects the device's own. The size still arrives, with
 * square pixels.
 **/
BOOST_AUTO_TEST_CASE(ribreader_default_pixel_aspect_test) {
    CountingHandler handler;
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(read("Format 640 480 -1\n", &handler, &reader));
    BOOST_CHECK_EQUAL(handler.width_, 640U);
    BOOST_CHECK_EQUAL(handler.height_, 480U);
    BOOST_CHECK_EQUAL(handler.pixelAspect_, 1.0F);

    BOOST_REQUIRE(read("Format 320 240 1e39\n", &handler, &reader));
    BOOST_CHECK_EQUAL(handler.width_, 320U);
    BOOST_CHECK_EQUAL(handler.pixelAspect_, 1.0F);
}

/**
 * A count or a light handle too large for an integer is an error, rather than a conversion
 * that is undefined.
 **/
BOOST_AUTO_TEST_CASE(ribreader_counts_and_handles_out_of_range_test) {
    CountingHandler handler;
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());
    BOOST_CHECK(!read("PointsPolygons [1e39] [0 1 2] \"P\" [0 0 0 1 0 0 0 1 0]\n", &handler, &reader));
    BOOST_CHECK(reader.error().contains("expected a count"));

    v3d::render::offline::rib::Reader second(boost::make_shared<v3d::log::Logger>());
    BOOST_CHECK(!read("Illuminate 1e39 1\n", &handler, &second));
    BOOST_CHECK(second.error().contains("expected a light handle"));
}

};  // namespace
