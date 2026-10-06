/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/offline/Sampling.h>
#include <api/render/offline/rib/Reader.h>

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
    void declare(const std::string & name, const std::string & declaration) override {
        (void)declaration;
        declared_.push_back(name);
        counts_["Declare"]++;
    }
    void option(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) override {
        (void)name;
        bucket_ = parameters.floats("bucketsize");
        if (parameters.has("texture")) {
            texturePath_ = parameters.string("texture", std::string());
        }
        counts_["Option"]++;
    }
    void hider(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) override {
        (void)parameters;
        hider_ = name;
        counts_["Hider"]++;
    }
    void format(unsigned int width, unsigned int height, float pixelAspect) override {
        width_ = width;
        height_ = height;
        pixelAspect_ = pixelAspect;
        counts_["Format"]++;
    }
    void projection(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) override {
        projection_ = name;
        fov_ = parameters.number("fov", 90.0f);
        counts_["Projection"]++;
    }
    void clipping(float hither, float yon) override {
        hither_ = hither;
        yon_ = yon;
        counts_["Clipping"]++;
    }
    void display(const std::string & name, const std::string & type, const std::string & mode,
        const v3d::render::offline::rib::ParameterList & parameters) override {
        (void)parameters;
        display_ = name + "|" + type + "|" + mode;
        counts_["Display"]++;
    }
    void depthOfField(float fstop, float focalLength, float focalDistance) override {
        lens_ = glm::vec3(fstop, focalLength, focalDistance);
        counts_["DepthOfField"]++;
    }
    void shutter(float open, float close) override {
        shutter_ = glm::vec2(open, close);
        counts_["Shutter"]++;
    }
    void pixelSamples(unsigned int x, unsigned int y) override {
        samples_ = glm::uvec2(x, y);
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
        roughness_ = parameters.number("roughness", -1.0f);
        counts_["Surface"]++;
    }
    void lightSource(const std::string & name, const std::string & handle,
        const v3d::render::offline::rib::ParameterList & parameters) override {
        lights_.push_back(name + "|" + handle + "|" +
            std::to_string(parameters.number("intensity", -1.0f)));
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
        identifier_ = parameters.string("name", "");
        counts_["Attribute"]++;
    }
    void polygon(unsigned int vertices, const v3d::render::offline::rib::ParameterList & parameters) override {
        vertices_ = vertices;
        points_ = parameters.points("P");
        colors_ = parameters.points("Cs");
        counts_["Polygon"]++;
    }
    void pointsPolygons(const std::vector<unsigned int> & perPolygon, const std::vector<unsigned int> & indices,
        const v3d::render::offline::rib::ParameterList & parameters) override {
        perPolygon_ = perPolygon;
        indices_ = indices;
        points_ = parameters.points("P");
        counts_["PointsPolygons"]++;
    }
    void sphere(float radius, float zmin, float zmax, float thetamax,
        const v3d::render::offline::rib::ParameterList & parameters) override {
        (void)zmin;
        (void)zmax;
        (void)thetamax;
        (void)parameters;
        radius_ = radius;
        counts_["Sphere"]++;
    }

    unsigned int count(const std::string & name) const {
        const std::map<std::string, unsigned int>::const_iterator found = counts_.find(name);
        return found == counts_.end() ? 0u : found->second;
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
    glm::mat4x4 transform_ = glm::mat4x4(1.0f);
    glm::vec3 translate_ = glm::vec3(0.0f);
    glm::vec3 color_ = glm::vec3(0.0f);
    glm::vec3 lens_ = glm::vec3(0.0f);
    glm::vec2 shutter_ = glm::vec2(0.0f);
    glm::vec2 filterWidth_ = glm::vec2(0.0f);
    glm::uvec2 samples_ = glm::uvec2(0);
    v3d::render::offline::Filter filter_ = v3d::render::offline::Filter::Box;
    float variance_ = 0.0f;
    std::string projection_;
    std::string display_;
    std::string surface_;
    std::string imager_;
    std::string hider_;
    std::string identifier_;
    float version_ = 0.0f;
    float pixelAspect_ = 0.0f;
    float hither_ = 0.0f;
    float yon_ = 0.0f;
    float fov_ = 0.0f;
    float roughness_ = 0.0f;
    float radius_ = 0.0f;
    unsigned int width_ = 0;
    unsigned int height_ = 0;
    unsigned int vertices_ = 0;
    int frame_ = 0;
};

bool read(const std::string & source, CountingHandler * handler, v3d::render::offline::rib::Reader * reader) {
    std::istringstream stream(source);
    return reader->read(stream, handler);
}

};  // namespace

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

    BOOST_CHECK_CLOSE(handler.version_, 3.03f, 0.01f);
    BOOST_CHECK_EQUAL(handler.width_, 640u);
    BOOST_CHECK_EQUAL(handler.height_, 480u);
    BOOST_CHECK_EQUAL(handler.pixelAspect_, 1.0f);
    BOOST_CHECK_EQUAL(handler.projection_, "perspective");
    BOOST_CHECK_EQUAL(handler.fov_, 45.0f);
    BOOST_CHECK_EQUAL(handler.hither_, 1.0f);
    BOOST_CHECK_EQUAL(handler.yon_, 100.0f);
    BOOST_CHECK_EQUAL(handler.display_, "out.png|file|rgb");
    BOOST_CHECK_EQUAL(handler.frame_, 1);
    BOOST_CHECK_EQUAL(handler.count("WorldBegin"), 1u);
    BOOST_CHECK_EQUAL(handler.count("WorldEnd"), 1u);
}

/**
 * RIB carries no vertex count - it is the length of the position array.
 **/
BOOST_AUTO_TEST_CASE(ribreader_polygon_test) {
    CountingHandler handler;
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(read(
        "Polygon \"P\" [-1 -1 5  1 -1 5  1 1 5  -1 1 5]\n", &handler, &reader));

    BOOST_CHECK_EQUAL(handler.count("Polygon"), 1u);
    BOOST_CHECK_EQUAL(handler.vertices_, 4u);
    BOOST_REQUIRE_EQUAL(handler.points_.size(), 4u);
    BOOST_CHECK_EQUAL(handler.points_[2].x, 1.0f);
    BOOST_CHECK_EQUAL(handler.points_[2].y, 1.0f);
    BOOST_CHECK_EQUAL(handler.points_[2].z, 5.0f);
}

/**
 * A varying parameter carries one element per vertex, so a colour per corner reads back as
 * four colours without the reader being told how many there were.
 **/
BOOST_AUTO_TEST_CASE(ribreader_varying_parameter_test) {
    CountingHandler handler;
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(read(
        "Polygon \"P\" [0 0 1  1 0 1  1 1 1]\n"
        "        \"Cs\" [1 0 0  0 1 0  0 0 1]\n", &handler, &reader));

    BOOST_CHECK_EQUAL(handler.vertices_, 3u);
    BOOST_REQUIRE_EQUAL(handler.colors_.size(), 3u);
    BOOST_CHECK_EQUAL(handler.colors_[2].z, 1.0f);
}

BOOST_AUTO_TEST_CASE(ribreader_points_polygons_test) {
    CountingHandler handler;
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(read(
        "PointsPolygons [3 3] [0 1 2  0 2 3] \"P\" [0 0 0  1 0 0  1 1 0  0 1 0]\n", &handler, &reader));

    BOOST_REQUIRE_EQUAL(handler.perPolygon_.size(), 2u);
    BOOST_CHECK_EQUAL(handler.perPolygon_[0], 3u);
    BOOST_REQUIRE_EQUAL(handler.indices_.size(), 6u);
    BOOST_CHECK_EQUAL(handler.indices_[4], 2u);
    BOOST_CHECK_EQUAL(handler.points_.size(), 4u);
}

/**
 * An unbracketed value is legal for a uniform parameter; only the declaration table says
 * how many values it takes.
 **/
BOOST_AUTO_TEST_CASE(ribreader_unbracketed_parameter_test) {
    CountingHandler handler;
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(read("Surface \"plastic\" \"roughness\" .3\n", &handler, &reader));

    BOOST_CHECK_EQUAL(handler.surface_, "plastic");
    BOOST_CHECK_CLOSE(handler.roughness_, 0.3f, 0.01f);
}

BOOST_AUTO_TEST_CASE(ribreader_declare_then_use_test) {
    CountingHandler handler;
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(read(
        "Declare \"squish\" \"uniform float\"\n"
        "Surface \"marble\" \"squish\" 5\n", &handler, &reader));

    BOOST_REQUIRE_EQUAL(handler.declared_.size(), 1u);
    BOOST_CHECK_EQUAL(handler.declared_[0], "squish");
    BOOST_CHECK_EQUAL(handler.count("Surface"), 1u);
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
    BOOST_CHECK_EQUAL(bracketed.count("Surface"), 1u);
    BOOST_CHECK_EQUAL(bracketed.count("WorldBegin"), 1u);

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
    BOOST_CHECK_EQUAL(handler.samples_.x, 4u);
    BOOST_CHECK_EQUAL(handler.samples_.y, 3u);
    BOOST_CHECK(handler.filter_ == v3d::render::offline::Filter::CatmullRom);
    BOOST_CHECK_EQUAL(handler.filterWidth_.x, 3.0f);
    BOOST_CHECK_EQUAL(handler.filterWidth_.y, 4.0f);
    BOOST_CHECK_CLOSE(handler.variance_, 0.05f, 1.0e-4f);
    BOOST_CHECK_EQUAL(handler.shutter_.x, 0.0f);
    BOOST_CHECK_EQUAL(handler.shutter_.y, 0.5f);
    BOOST_CHECK_EQUAL(handler.lens_.x, 8.0f);
    BOOST_CHECK_CLOSE(handler.lens_.y, 0.1f, 1.0e-4f);
    BOOST_CHECK_EQUAL(handler.lens_.z, 3.0f);
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

    BOOST_CHECK_EQUAL(handler.count("PixelFilter"), 0u);
    BOOST_CHECK_EQUAL(handler.count("PixelSamples"), 1u);
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

    BOOST_CHECK_EQUAL(handler.count("DepthOfField"), 1u);
    BOOST_CHECK(std::isinf(handler.lens_.x));
    BOOST_CHECK_EQUAL(handler.samples_.x, 1u);
    BOOST_CHECK_EQUAL(handler.samples_.y, 1u);

    // a rate too large for a pixel's grid of samples, or for the conversion, is capped
    BOOST_REQUIRE(read("PixelSamples 65536 1e39\n", &handler, &reader));
    BOOST_CHECK_EQUAL(handler.samples_.x, v3d::render::offline::maximumSamples);
    BOOST_CHECK_EQUAL(handler.samples_.y, v3d::render::offline::maximumSamples);

    v3d::render::offline::Sampling sampling;
    sampling.fstop = handler.lens_.x;
    sampling.focalLength = 0.1f;
    sampling.focalDistance = 3.0f;
    BOOST_CHECK(sampling.pinhole());
    sampling.fstop = 8.0f;
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
        "Displacement \"MyShader\" \"squish\" 5\n"
        "Sides 1\n"
        "WorldBegin\n"
        "Polygon \"P\" [0 0 1  1 0 1  1 1 1]\n"
        "WorldEnd\n", &handler, &reader));

    BOOST_REQUIRE_EQUAL(reader.unrecognised().size(), 2u);
    BOOST_CHECK_EQUAL(reader.unrecognised()[0], "Sides");
    BOOST_CHECK_EQUAL(reader.unrecognised()[1], "Displacement");
    // everything after them still arrived
    BOOST_CHECK_EQUAL(handler.count("WorldBegin"), 1u);
    BOOST_CHECK_EQUAL(handler.vertices_, 3u);
}

/**
 * MakeTexture is understood and makes nothing, because the image a scene names is the
 * texture; and the texture search path is a string the renderer receives.
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
    BOOST_CHECK_EQUAL(handler.vertices_, 3u);
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

    const glm::vec4 moved = handler.transform_ * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    BOOST_CHECK_EQUAL(moved.x, 2.0f);
    BOOST_CHECK_EQUAL(moved.y, 3.0f);
    BOOST_CHECK_EQUAL(moved.z, 4.0f);
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

    BOOST_CHECK_EQUAL(handler.count("AttributeBegin"), 1u);
    BOOST_CHECK_EQUAL(handler.count("AttributeEnd"), 1u);
    BOOST_CHECK_CLOSE(handler.color_.r, 0.9f, 0.01f);
    BOOST_CHECK_EQUAL(handler.translate_.z, 3.0f);
    BOOST_CHECK_EQUAL(handler.identifier_, "myball");
    BOOST_CHECK_EQUAL(handler.radius_, 0.5f);
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

    BOOST_CHECK_EQUAL(handler.count("FrameBegin"), 2u);
    BOOST_CHECK_EQUAL(handler.count("FrameEnd"), 2u);
    BOOST_CHECK_EQUAL(handler.count("WorldBegin"), 2u);
    BOOST_CHECK_EQUAL(handler.count("Transform"), 2u);
    BOOST_CHECK_EQUAL(handler.count("Sphere"), 4u);
    BOOST_CHECK_EQUAL(handler.count("Polygon"), 1u);
    BOOST_CHECK_EQUAL(handler.count("Option"), 2u);
    BOOST_CHECK_EQUAL(handler.count("Declare"), 2u);
    BOOST_CHECK_EQUAL(handler.count("LightSource"), 3u);
    BOOST_CHECK_EQUAL(handler.count("Illuminate"), 2u);
    BOOST_CHECK_EQUAL(handler.count("Imager"), 1u);
    // the floor polygon is the only geometry with vertices in the file
    BOOST_CHECK_EQUAL(handler.vertices_, 4u);

    // Displacement and ShadingRate are recognised by the standard and not by this reader
    BOOST_CHECK(!reader.unrecognised().empty());
}

/**
 * A light carries a handle, and a later Illuminate names it by that handle. RIB 3.03
 * writes it as a sequence number and later RIB writes a string; both reach the handler as a
 * string.
 **/
BOOST_AUTO_TEST_CASE(ribreader_light_handles_test) {
    CountingHandler handler;
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(read(
        "LightSource \"distantlight\" 1 \"intensity\" [2]\n"
        "LightSource \"pointlight\" \"fill\" \"intensity\" [0.5]\n"
        "Illuminate 1 0\n"
        "Illuminate \"fill\" 1\n", &handler, &reader));

    BOOST_REQUIRE_EQUAL(handler.lights_.size(), 2u);
    BOOST_CHECK_EQUAL(handler.lights_[0], "distantlight|1|2.000000");
    BOOST_CHECK_EQUAL(handler.lights_[1], "pointlight|fill|0.500000");
    BOOST_REQUIRE_EQUAL(handler.illuminated_.size(), 2u);
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
    BOOST_REQUIRE_EQUAL(handler.lights_.size(), 1u);
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
    BOOST_REQUIRE_EQUAL(handler.background_.size(), 1u);
    BOOST_CHECK_CLOSE(handler.background_[0].b, 0.3f, 0.01f);
}

/**
 * Hider names how the renderer decides what the camera sees, and reaches the handler by name.
 **/
BOOST_AUTO_TEST_CASE(ribreader_hider_test) {
    CountingHandler handler;
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(read("Hider \"raytrace\"\n", &handler, &reader));
    BOOST_CHECK_EQUAL(handler.count("Hider"), 1u);
    BOOST_CHECK_EQUAL(handler.hider_, "raytrace");
    BOOST_CHECK(reader.unrecognised().empty());
}

/**
 * A motion block's times reach the handler, and every transform request inside it does too,
 * since each is the transformation at one of the times.
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

    BOOST_CHECK_EQUAL(handler.count("MotionBegin"), 1u);
    BOOST_CHECK_EQUAL(handler.count("MotionEnd"), 1u);
    BOOST_CHECK_EQUAL(handler.count("Translate"), 2u);
    BOOST_REQUIRE_EQUAL(handler.motionTimes_.size(), 2u);
    BOOST_CHECK_EQUAL(handler.motionTimes_[1], 0.5f);
    BOOST_CHECK_EQUAL(handler.count("Polygon"), 1u);
    BOOST_CHECK(reader.unsupported().empty());
}

/**
 * A primitive repeated inside a motion block deforms, which the default handler does not
 * build: the first reaches
 * the handler, the second is read and reported, and the request after the block still reads.
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

    BOOST_CHECK_EQUAL(handler.count("Polygon"), 1u);
    BOOST_CHECK_EQUAL(handler.vertices_, 3u);
    BOOST_REQUIRE_EQUAL(reader.unsupported().size(), 1u);
    BOOST_CHECK_EQUAL(reader.unsupported()[0], "deforming Polygon");
    BOOST_CHECK_EQUAL(handler.count("Color"), 1u);
}

namespace {

/**
 * A handler that implements what the default bodies leave out: sample an area light, make a
 * texture, and build a primitive that deforms.
 **/
class CapableHandler final : public v3d::render::offline::rib::Handler {
 public:
    void areaLightSource(const std::string & name, const std::string & handle,
        const v3d::render::offline::rib::ParameterList & parameters) override {
        (void)parameters;
        area_ = name + "|" + handle;
    }
    void lightSource(const std::string & name, const std::string & handle,
        const v3d::render::offline::rib::ParameterList & parameters) override {
        (void)name;
        (void)handle;
        (void)parameters;
        lights_++;
    }
    void makeTexture(const std::string & picture, const std::string & texture, const std::string & swrap,
        const std::string & twrap, const std::string & filter, float swidth, float twidth,
        const v3d::render::offline::rib::ParameterList & parameters) override {
        (void)parameters;
        made_ = picture + "|" + texture + "|" + swrap + "|" + twrap + "|" + filter + "|" +
            std::to_string(static_cast<int>(swidth)) + "|" + std::to_string(static_cast<int>(twidth));
    }
    void polygon(unsigned int vertices, const v3d::render::offline::rib::ParameterList & parameters) override {
        (void)parameters;
        vertices_.push_back(vertices);
    }
    Handler* deformation() override {
        return &later_;
    }

    /** The later poses, which this renderer keeps apart from the first. **/
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
    BOOST_CHECK_EQUAL(handler.lights_, 0u);
    BOOST_CHECK((handler.vertices_ == std::vector<unsigned int>{ 3u }));
    BOOST_CHECK((handler.later_.vertices_ == std::vector<unsigned int>{ 4u }));
    BOOST_CHECK(reader.unsupported().empty());
}

/**
 * A Format, a FrameBegin or a PixelFilter width that is not a size is skipped with a warning,
 * rather than converted to an unsigned count, which is undefined for a negative number. The
 * file still reads, and the requests that are sizes still arrive.
 **/
BOOST_AUTO_TEST_CASE(ribreader_sizes_that_are_not_sizes_are_skipped_test) {
    CountingHandler handler;
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(read(
        "Format -1 480 1\n"
        "PixelFilter \"box\" 0 1\n"
        "FrameBegin 1e30\n"
        "FrameEnd\n", &handler, &reader));
    BOOST_CHECK_EQUAL(handler.width_, 0u);
    BOOST_CHECK_EQUAL(handler.filterWidth_.x, 0.0f);
    BOOST_CHECK_EQUAL(handler.count("PixelFilter"), 0u);
    BOOST_CHECK_EQUAL(handler.count("FrameBegin"), 0u);
    BOOST_CHECK_EQUAL(handler.count("FrameEnd"), 1u);

    BOOST_REQUIRE(read("Format 64 48 1\nPixelFilter \"box\" 2 2\n", &handler, &reader));
    BOOST_CHECK_EQUAL(handler.width_, 64u);
    BOOST_CHECK_EQUAL(handler.filterWidth_.x, 2.0f);
}

/**
 * A pixel aspect of zero or less asks for the device's own. The size still arrives, with
 * square pixels.
 **/
BOOST_AUTO_TEST_CASE(ribreader_default_pixel_aspect_test) {
    CountingHandler handler;
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(read("Format 640 480 -1\n", &handler, &reader));
    BOOST_CHECK_EQUAL(handler.width_, 640u);
    BOOST_CHECK_EQUAL(handler.height_, 480u);
    BOOST_CHECK_EQUAL(handler.pixelAspect_, 1.0f);

    BOOST_REQUIRE(read("Format 320 240 1e39\n", &handler, &reader));
    BOOST_CHECK_EQUAL(handler.width_, 320u);
    BOOST_CHECK_EQUAL(handler.pixelAspect_, 1.0f);
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
