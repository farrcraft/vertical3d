/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/material/internal/RiSurfaces.h>

#include <moya/libmoya/material/internal/RiFeed.h>
#include <moya/libmoya/material/internal/RiParameter.h>

#include <string>
#include <vector>

namespace v3d::moya::material::internal {

namespace {

std::string inputOf(const Parameter & parameter) {
    return "    <input name=\"" + parameter.name + "\" type=\"" + parameter.type + "\" value=\"" + parameter.value + "\" />\n";
}

/** A node that passes an input of the graph's on. **/
std::string passed(const std::string & name, const std::string & type, const std::string & interface) {
    return "    <dot name=\"" + name + "\" type=\"" + type + "\">\n      <input name=\"in\" type=\"" + type + "\" interfacename=\"" + interface +
        "\" />\n    </dot>\n";
}

/** A node that gives a value. **/
std::string fixed(const std::string & name, const std::string & type, const std::string & value) {
    return "    <dot name=\"" + name + "\" type=\"" + type + "\">\n      <input name=\"in\" type=\"" + type + "\" value=\"" + value + "\" />\n    </dot>\n";
}

/** A node of two float inputs, each an input of the graph's, a node or a value. **/
std::string binary(const std::string & category, const std::string & name, const std::string & first, const std::string & second) {
    const auto operand = [](const std::string & input, const std::string & given) {
        if (given.starts_with("@")) {
            return "      <input name=\"" + input + "\" type=\"float\" interfacename=\"" + given.substr(1) + "\" />\n";
        }
        if (given.starts_with("#")) {
            return "      <input name=\"" + input + "\" type=\"float\" nodename=\"" + given.substr(1) + "\" />\n";
        }
        return "      <input name=\"" + input + "\" type=\"float\" value=\"" + given + "\" />\n";
    };
    return "    <" + category + " name=\"" + name + "\" type=\"float\">\n" + operand("in1", first) + operand("in2", second) + "    </" + category + ">\n";
}

/** colour: the geometry's Cs primvar, or the node's Cs. **/
std::string colour() {
    return "    <geompropvalue name=\"colour\" type=\"color3\">\n      <input name=\"geomprop\" type=\"string\" value=\"Cs\" />\n"
           "      <input name=\"default\" type=\"color3\" interfacename=\"Cs\" />\n    </geompropvalue>\n";
}

/**
 * opacity: the mean of the node's Os. It is the node's alone, so that it is one value over the
 * surface, and a surface of no opacity below one is not cut out.
 **/
std::string opacity() {
    std::string out;
    const char * const channels[] = { "opacityRed", "opacityGreen", "opacityBlue" };
    for (int i = 0; i < 3; ++i) {
        out += "    <extract name=\"" + std::string(channels[i]) +
            "\" type=\"float\">\n      <input name=\"in\" type=\"color3\" interfacename=\"Os\" />\n      <input name=\"index\" type=\"integer\" value=\"" +
            std::to_string(i) + "\" />\n    </extract>\n";
    }
    out += binary("add", "opacityRedGreen", "#opacityRed", "#opacityGreen");
    out += binary("add", "opacitySum", "#opacityRedGreen", "#opacityBlue");
    out += binary("multiply", "opacity", "#opacitySum", "0.333333");
    return out;
}

/**
 * specularRoughness: RI's roughness as OpenPBR's. A Phong exponent of 1 / roughness is a Beckmann
 * alpha of sqrt(2 roughness / (1 + 2 roughness)), and OpenPBR's roughness is that alpha's root.
 **/
std::string roughness() {
    return binary("multiply", "roughnessTwice", "@roughness", "2") + binary("add", "roughnessPlus", "#roughnessTwice", "1") +
        binary("divide", "roughnessRatio", "#roughnessTwice", "#roughnessPlus") + binary("power", "specularRoughness", "#roughnessRatio", "0.25");
}

/** The two definitions of a built-in and the graph of its translation, of the nodes given. **/
std::string builtIn(const std::string & shader, std::vector<Parameter> parameters, const std::string & nodes, const std::vector<Feed> & feeds) {
    parameters.push_back(Parameter { .name = "Cs", .type = "color3", .value = "1, 1, 1" });
    parameters.push_back(Parameter { .name = "Os", .type = "color3", .value = "1, 1, 1" });
    std::string inputs;
    for (const Parameter & parameter : parameters) {
        inputs += inputOf(parameter);
    }
    std::string out = "  <nodedef name=\"ND_ri_" + shader + "_surfaceshader\" node=\"ri_" + shader + "\" nodegroup=\"pbr\">\n" + inputs +
        "    <output name=\"out\" type=\"surfaceshader\" />\n  </nodedef>\n";
    out += "  <nodedef name=\"ND_ri_" + shader + "_to_open_pbr_surface\" node=\"ri_" + shader + "_to_open_pbr_surface\" nodegroup=\"translation\">\n" + inputs;
    std::string outputs;
    for (const Feed & feed : feeds) {
        out += "    <output name=\"" + feed.target + "_out\" type=\"" + feed.type + "\" />\n";
        outputs += "    <output name=\"" + feed.target + "_out\" type=\"" + feed.type + "\" nodename=\"" + feed.node + "\" />\n";
    }
    out += "  </nodedef>\n";
    out += "  <nodegraph name=\"NG_ri_" + shader + "_to_open_pbr_surface\" nodedef=\"ND_ri_" + shader + "_to_open_pbr_surface\">\n" + nodes + outputs +
        "  </nodegraph>\n";
    return out;
}

/** RI's ambient weight, which every built-in but glass takes at one. **/
Parameter ka() {
    return Parameter { .name = "Ka", .type = "float", .value = "1" };
}

/** RI's roughness at its default for a highlight. **/
Parameter roughnessParameter() {
    return Parameter { .name = "roughness", .type = "float", .value = "0.1" };
}

Feed baseColour() {
    return Feed { .target = "base_color", .type = "color3", .node = "colour" };
}

Feed meanOpacity() {
    return Feed { .target = "geometry_opacity", .type = "float", .node = "opacity" };
}

std::string constant() {
    const std::string nodes = fixed("none", "float", "0") + fixed("one", "float", "1") + passed("emitted", "color3", "Cs") + opacity();
    return builtIn("constant", {}, nodes,
        { Feed { .target = "base_weight", .type = "float", .node = "none" }, Feed { .target = "specular_weight", .type = "float", .node = "none" },
            Feed { .target = "emission_luminance", .type = "float", .node = "one" },
            Feed { .target = "emission_color", .type = "color3", .node = "emitted" }, meanOpacity() });
}

std::string matte() {
    const std::string nodes = passed("baseWeight", "float", "Kd") + fixed("none", "float", "0") + colour() + opacity();
    return builtIn("matte", { ka(), Parameter { .name = "Kd", .type = "float", .value = "1" } }, nodes,
        { Feed { .target = "base_weight", .type = "float", .node = "baseWeight" }, baseColour(),
            Feed { .target = "specular_weight", .type = "float", .node = "none" }, meanOpacity() });
}

/** plastic's parameters, which paintedplastic's begin with. **/
std::vector<Parameter> plasticParameters() {
    return { ka(), Parameter { .name = "Kd", .type = "float", .value = "0.5" }, Parameter { .name = "Ks", .type = "float", .value = "0.5" }, roughnessParameter(),
        Parameter { .name = "specularcolor", .type = "color3", .value = "1, 1, 1" } };
}

/** plastic's feeds, with its base colour from the node named. **/
std::vector<Feed> plasticFeeds(const std::string & base) {
    return { Feed { .target = "base_weight", .type = "float", .node = "baseWeight" }, Feed { .target = "base_color", .type = "color3", .node = base },
        Feed { .target = "specular_weight", .type = "float", .node = "specularWeight" },
        Feed { .target = "specular_color", .type = "color3", .node = "specularColour" },
        Feed { .target = "specular_roughness", .type = "float", .node = "specularRoughness" }, meanOpacity() };
}

std::string plasticNodes() {
    return passed("baseWeight", "float", "Kd") + passed("specularWeight", "float", "Ks") + passed("specularColour", "color3", "specularcolor") + colour() +
        roughness() + opacity();
}

std::string plastic() {
    return builtIn("plastic", plasticParameters(), plasticNodes(), plasticFeeds("colour"));
}

std::string paintedplastic() {
    std::vector<Parameter> parameters = plasticParameters();
    parameters.push_back(Parameter { .name = "texturename", .type = "filename", .value = "" });
    const std::string texture =
        "    <texcoord name=\"coordinate\" type=\"vector2\" />\n"
        "    <extract name=\"s\" type=\"float\">\n      <input name=\"in\" type=\"vector2\" nodename=\"coordinate\" />\n"
        "      <input name=\"index\" type=\"integer\" value=\"0\" />\n    </extract>\n"
        "    <extract name=\"t\" type=\"float\">\n      <input name=\"in\" type=\"vector2\" nodename=\"coordinate\" />\n"
        "      <input name=\"index\" type=\"integer\" value=\"1\" />\n    </extract>\n" +
        binary("subtract", "up", "1", "#t") +
        "    <combine2 name=\"placed\" type=\"vector2\">\n      <input name=\"in1\" type=\"float\" nodename=\"s\" />\n"
        "      <input name=\"in2\" type=\"float\" nodename=\"up\" />\n    </combine2>\n"
        "    <image name=\"picture\" type=\"color3\">\n      <input name=\"file\" type=\"filename\" interfacename=\"texturename\" />\n"
        "      <input name=\"default\" type=\"color3\" value=\"1, 1, 1\" />\n      <input name=\"texcoord\" type=\"vector2\" nodename=\"placed\" />\n"
        "    </image>\n"
        "    <multiply name=\"painted\" type=\"color3\">\n      <input name=\"in1\" type=\"color3\" nodename=\"colour\" />\n"
        "      <input name=\"in2\" type=\"color3\" nodename=\"picture\" />\n    </multiply>\n";
    return builtIn("paintedplastic", parameters, plasticNodes() + texture, plasticFeeds("painted"));
}

std::string metal() {
    const std::string nodes = fixed("metallic", "float", "1") + passed("baseWeight", "float", "Ks") + colour() + roughness() + opacity();
    return builtIn("metal", { ka(), Parameter { .name = "Ks", .type = "float", .value = "1" }, roughnessParameter() }, nodes,
        { Feed { .target = "base_weight", .type = "float", .node = "baseWeight" }, baseColour(),
            Feed { .target = "base_metalness", .type = "float", .node = "metallic" },
            Feed { .target = "specular_roughness", .type = "float", .node = "specularRoughness" }, meanOpacity() });
}

std::string shinymetal() {
    const std::string nodes = fixed("metallic", "float", "1") + colour() + roughness() + opacity() + binary("add", "lobes", "@Ks", "@Kr") +
        binary("min", "baseWeight", "#lobes", "1") + binary("max", "bounded", "#lobes", "0.000001") + binary("divide", "glossyShare", "@Ks", "#bounded") +
        binary("multiply", "sharedRoughness", "#specularRoughness", "#glossyShare");
    return builtIn("shinymetal",
        { ka(), Parameter { .name = "Ks", .type = "float", .value = "1" }, Parameter { .name = "Kr", .type = "float", .value = "1" }, roughnessParameter() }, nodes,
        { Feed { .target = "base_weight", .type = "float", .node = "baseWeight" }, baseColour(),
            Feed { .target = "base_metalness", .type = "float", .node = "metallic" },
            Feed { .target = "specular_roughness", .type = "float", .node = "sharedRoughness" }, meanOpacity() });
}

std::string glass() {
    const std::string nodes = fixed("sharp", "float", "0") + passed("specularWeight", "float", "Kr") + passed("transmissionWeight", "float", "Kt") +
        passed("index", "float", "eta") + colour();
    return builtIn("glass",
        { Parameter { .name = "Ka", .type = "float", .value = "0" }, Parameter { .name = "Ks", .type = "float", .value = "0.5" },
            Parameter { .name = "Kr", .type = "float", .value = "1" }, Parameter { .name = "Kt", .type = "float", .value = "1" },
            Parameter { .name = "roughness", .type = "float", .value = "0.05" }, Parameter { .name = "eta", .type = "float", .value = "1.5" } },
        nodes,
        { Feed { .target = "specular_weight", .type = "float", .node = "specularWeight" },
            Feed { .target = "specular_roughness", .type = "float", .node = "sharp" }, Feed { .target = "specular_ior", .type = "float", .node = "index" },
            Feed { .target = "transmission_weight", .type = "float", .node = "transmissionWeight" },
            Feed { .target = "transmission_color", .type = "color3", .node = "colour" } });
}

};  // namespace

std::string riSurfaces() {
    return constant() + matte() + plastic() + paintedplastic() + metal() + shinymetal() + glass();
}

};  // namespace v3d::moya::material::internal
