/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/offline/rib/Reader.h>

#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

namespace {

/**
 * Counts what it was handed, which is what proves the parser without either renderer.
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
        counts_["Option"]++;
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
    std::vector<float> bucket_;
    std::vector<glm::vec3> points_;
    std::vector<glm::vec3> colors_;
    std::vector<unsigned int> perPolygon_;
    std::vector<unsigned int> indices_;
    glm::mat4x4 transform_ = glm::mat4x4(1.0f);
    glm::vec3 translate_ = glm::vec3(0.0f);
    glm::vec3 color_ = glm::vec3(0.0f);
    std::string projection_;
    std::string display_;
    std::string surface_;
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
 * An unbracketed value is legal for a uniform parameter, and is why the declaration table
 * exists at all.
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
 * without an array is not, and saying so is better than reading the next request as a value.
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
 * An unrecognised request is reported once per name and its arguments are skipped, so the
 * request after it is still read. A scene that rendered nothing and a scene that was not
 * understood look identical from outside without this.
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
    // and everything after them still arrived
    BOOST_CHECK_EQUAL(handler.count("WorldBegin"), 1u);
    BOOST_CHECK_EQUAL(handler.vertices_, 3u);
}

/**
 * RIB writes a matrix in row major order under RI's row vector convention; glm stores column
 * major under a column vector one, so reading the floats in order is the conversion. A
 * transpose would undo it, which is why this applies the result rather than comparing storage.
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
 * A parse error says what and where rather than leaving a scene that quietly rendered nothing.
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
    // the floor polygon is the only geometry with vertices in the file
    BOOST_CHECK_EQUAL(handler.vertices_, 4u);

    // Displacement and ShadingRate are recognised by the standard and not by this reader
    BOOST_CHECK(!reader.unrecognised().empty());
}
