/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/material/internal/Emitter.h>

#include <moya/libmoya/kernel/program/Instruction.h>
#include <moya/libmoya/kernel/program/Op.h>
#include <moya/libmoya/kernel/program/Pattern.h>
#include <moya/libmoya/kernel/program/Program.h>
#include <moya/libmoya/kernel/program/Run.h>
#include <moya/libmoya/kernel/texture/Filtering.h>
#include <moya/libmoya/kernel/texture/Texture.h>
#include <moya/libmoya/material/internal/Network.h>
#include <moya/libmoya/material/internal/Operation.h>
#include <moya/libmoya/texture/Request.h>
#include <MaterialXCore/Definition.h>
#include <MaterialXCore/Document.h>
#include <MaterialXCore/Geom.h>
#include <MaterialXCore/Interface.h>
#include <MaterialXCore/Node.h>
#include <MaterialXCore/Value.h>

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace v3d::moya::material::internal {

namespace {

namespace mx = MaterialX;
using kernel::program::Instruction;
using kernel::program::Op;

/** How many registers a value of a MaterialX type takes, or zero for one no program holds. **/
std::uint32_t widthOf(const std::string & type) {
    if (type == "float" || type == "integer" || type == "boolean") {
        return 1;
    }
    if (type == "vector2") {
        return 2;
    }
    if (type == "color3" || type == "vector3") {
        return 3;
    }
    if (type == "color4" || type == "vector4") {
        return 4;
    }
    if (type == "matrix33") {
        return kernel::program::kMatrix3;
    }
    if (type == "matrix44") {
        return kernel::program::kMatrix4;
    }
    return 0;
}

/** A value's components as a program holds them, a boolean as one or zero. **/
std::vector<float> componentsOf(const mx::ConstValuePtr & value) {
    std::vector<float> out;
    if (!value) {
        return out;
    }
    const std::string written = value->getValueString();
    std::size_t at = 0;
    while (at <= written.size()) {
        const std::size_t comma = written.find(',', at);
        const std::size_t end = comma == std::string::npos ? written.size() : comma;
        std::string piece = written.substr(at, end - at);
        piece.erase(0, piece.find_first_not_of(' '));
        if (piece == "true" || piece == "false") {
            out.push_back(piece == "true" ? 1.0F : 0.0F);
        } else if (!piece.empty()) {
            out.push_back(std::stof(piece));
        }
        at = end + 1U;
    }
    return out;
}

/** Whether a value of a number of components is a matrix. **/
bool isMatrix(std::uint32_t width) {
    return width == kernel::program::kMatrix3 || width == kernel::program::kMatrix4;
}

/** The space a transform names: the world, or the geometry's own for object, model and none. **/
std::uint32_t spaceOf(const std::string & name) {
    return name == "world" ? kernel::program::kWorld : kernel::program::kObject;
}

/** Whether a matrix operation's operands have the shapes it reads. **/
bool matrixShaped(const Instruction & instruction) {
    const std::uint32_t width = instruction.width;
    const std::uint32_t first = instruction.widths[0];
    const std::uint32_t second = instruction.widths[1];
    switch (instruction.op) {
        case Op::Product:
        case Op::Quotient:
            return isMatrix(width) && first == width && second == width;
        case Op::Transpose:
        case Op::Inverse:
            return isMatrix(width) && first == width;
        case Op::Determinant:
            return width == 1U && isMatrix(first);
        case Op::Transform: {
            // a vector2 or vector3 before a matrix33, and a vector3 or vector4 before a matrix44
            const bool smaller = second == kernel::program::kMatrix3 && (first == 2U || first == 3U);
            const bool larger = second == kernel::program::kMatrix4 && (first == 3U || first == 4U);
            return (smaller || larger) && width == first;
        }
        case Op::Rows:
            return width == kernel::program::kMatrix4 && first == 3U && second == 3U && instruction.widths[2] == 3U && instruction.widths[3] == 3U;
        default:
            return false;
    }
}

/** Whether a pattern's operands have the shapes it reads. **/
bool patternShaped(const Instruction & instruction) {
    const std::uint32_t width = instruction.width;
    const std::uint32_t * widths = instruction.widths;
    // a position of two components or of three, and a value of one to four
    const auto placed = [](std::uint32_t components) {
        return components == 2U || components == 3U;
    };
    const bool value = width >= 1U && width <= 4U;
    switch (instruction.op) {
        case Op::Noise:
            return value && (widths[0] == width || widths[0] == 1U) && widths[1] == 1U && placed(widths[2]);
        case Op::Fractal:
            return value && (widths[0] == width || widths[0] == 1U) && widths[1] == 1U && widths[2] == 1U && widths[3] == 1U && placed(widths[4]);
        case Op::CellNoise:
            return width == 1U && placed(widths[0]);
        case Op::Worley:
            return width <= 3U && placed(widths[0]) && widths[1] == 1U && widths[2] == 1U;
        case Op::RampLeftRight:
        case Op::RampTopBottom:
            return value && widths[0] == width && widths[1] == width && widths[2] == 2U;
        case Op::SplitLeftRight:
        case Op::SplitTopBottom:
            return value && widths[0] == width && widths[1] == width && widths[2] == 1U && widths[3] == 2U;
        case Op::Rotate2d:
            return width == 2U && widths[0] == 2U && widths[1] == 1U;
        case Op::Rotate3d:
            return width == 3U && widths[0] == 3U && widths[1] == 1U && widths[2] == 3U;
        default:
            return false;
    }
}

/** Whether a normalmap's operands are a vector3, a scale of one or two, then three vector3. **/
bool normalMapShaped(const Instruction & instruction) {
    const std::uint32_t * widths = instruction.widths;
    const bool scale = widths[1] == 1U || widths[1] == 2U;
    return instruction.width == 3U && widths[0] == 3U && scale && widths[2] == 3U && widths[3] == 3U && widths[4] == 3U;
}

/** Whether an instruction's operands have the shapes its operation reads. **/
bool shaped(const Instruction & instruction, std::size_t operands) {
    const std::uint32_t width = instruction.width;
    const std::uint32_t first = instruction.widths[0];
    if (kernel::program::patterned(instruction.op)) {
        return patternShaped(instruction);
    }
    if (kernel::program::composited(instruction.op)) {
        // colours with alpha, and a mix of one component after two of them
        const bool single = instruction.op == Op::Premult || instruction.op == Op::Unpremult;
        return width == 4U && first == 4U && (single || (instruction.widths[1] == 4U && instruction.widths[2] == 1U));
    }
    if (kernel::program::componentwise(instruction.op)) {
        for (std::size_t i = 0; i < operands; ++i) {
            if (instruction.widths[i] != width && instruction.widths[i] != 1U) {
                return false;
            }
        }
        return true;
    }
    switch (instruction.op) {
        case Op::Normalize:
        case Op::RgbToHsv:
        case Op::HsvToRgb:
            return first == width && width >= 2U;
        case Op::Magnitude:
            return width == 1U && first >= 2U;
        case Op::Dot:
            return width == 1U && first == instruction.widths[1];
        case Op::Cross:
            return width == 3U && first == 3U && instruction.widths[1] == 3U;
        case Op::Luminance:
            return first == width && width >= 3U && instruction.widths[1] == 3U;
        case Op::Extract:
            return instruction.widths[1] == 1U && (width == 1U || (first == kernel::program::kMatrix3 && width == 3U) || (first == kernel::program::kMatrix4 && width == 4U));
        case Op::Product:
        case Op::Quotient:
        case Op::Transpose:
        case Op::Inverse:
        case Op::Determinant:
        case Op::Transform:
        case Op::Rows:
            return matrixShaped(instruction);
        case Op::NormalMap:
            return normalMapShaped(instruction);
        case Op::Combine: {
            std::uint32_t sum = 0;
            for (std::size_t i = 0; i < operands; ++i) {
                sum += instruction.widths[i];
            }
            return sum == width;
        }
        default:
            return false;
    }
}

/** A node's output of a name, or its first when the name is empty. **/
mx::OutputPtr outputOf(const mx::NodeDefPtr & def, const std::string & name) {
    if (!def) {
        return nullptr;
    }
    if (name.empty()) {
        const std::vector<mx::OutputPtr> outputs = def->getActiveOutputs();
        return outputs.empty() ? nullptr : outputs.front();
    }
    return def->getActiveOutput(name);
}

};  // namespace

Emitter::Emitter(Network & network, std::vector<std::string> & primvars, std::uint32_t parameters) :
    network_(network), primvars_(primvars), values_(parameters, 0.0F) {
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the type, then the space
void Emitter::constant(std::uint32_t at, const mx::ConstValuePtr & value, const std::string & type, const std::string & space) {
    const std::uint32_t width = widthOf(type);
    const std::vector<float> components = componentsOf(value);
    for (std::uint32_t i = 0; i < width && at + i < values_.size(); ++i) {
        values_[at + i] = i < components.size() ? components[i] : 0.0F;
    }
    if (type == "color3" || type == "color4") {
        colours_.push_back(at);
        spaces_.push_back(space);
    }
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - from, to, then the components
void Emitter::copyValue(std::uint32_t from, std::uint32_t to, std::uint32_t width) {
    Instruction out;
    out.op = Op::Copy;
    out.out = to;
    out.width = width;
    out.in[0] = from;
    out.widths[0] = width;
    instructions_.push_back(out);
}

std::string Emitter::keyed(const std::string & name) const {
    return std::to_string(shift_) + (measuring_ ? " measured " : " ") + name;
}

std::uint32_t Emitter::output(const mx::NodePtr & node, const std::string & name) {
    const std::string key = keyed(node->getName() + "." + name);
    const auto found = outputs_.find(key);
    if (found != outputs_.end()) {
        return found->second;
    }
    if (visiting_.contains(node->getName())) {
        network_.report("node " + node->getName() + " reads its own output, and gives its output's default");
        return uncompiled(node, name);
    }
    visiting_.insert(node->getName());
    const std::optional<Instruction> compiled = instruction(node);
    visiting_.erase(node->getName());
    std::uint32_t out = 0;
    if (compiled) {
        Instruction placed = *compiled;
        placed.out = allocate(placed.width);
        instructions_.push_back(placed);
        out = placed.out;
    } else {
        out = uncompiled(node, name);
    }
    outputs_.insert_or_assign(key, out);
    return out;
}

std::uint32_t Emitter::uncompiled(const mx::NodePtr & node, const std::string & name) {
    const mx::NodeDefPtr def = node->getNodeDef();
    const mx::OutputPtr output = outputOf(def, name);
    const std::string type = output ? output->getType() : node->getType();
    network_.report("node " + node->getName() + " of " + (def ? def->getName() : node->getCategory()) + " is not compiled, and gives its output's default");
    const std::uint32_t width = widthOf(type) > 0U ? widthOf(type) : 1U;
    const std::uint32_t at = allocate(width);
    const std::string written = output ? output->getAttribute("default") : std::string();
    constant(at, written.empty() ? nullptr : mx::Value::createValueFromStrings(written, type), type, std::string());
    return at;
}

std::uint32_t Emitter::allocate(std::uint32_t width) {
    const auto at = static_cast<std::uint32_t>(values_.size());
    if (at + width > kernel::program::kRegisters) {
        overflowed_ = true;
    }
    values_.resize(at + width, 0.0F);
    return at;
}

std::uint32_t Emitter::operand(const mx::NodePtr & node, const mx::InputPtr & declared) {
    const std::string & name = declared->getName();
    const mx::InputPtr input = node->getInput(name);
    const mx::NodePtr from = input ? input->getConnectedNode() : nullptr;
    if (from) {
        return output(from, input->getOutputString());
    }
    if (!node->getInputValue(name) && declared->hasDefaultGeomPropString()) {
        const std::optional<std::uint32_t> read = geomprop(declared);
        if (read) {
            return *read;
        }
    }
    const std::string & type = declared->getType();
    const std::uint32_t at = allocate(widthOf(type));
    constant(at, node->getInputValue(name), type, (input ? input : declared)->getActiveColorSpace());
    return at;
}

std::uint32_t Emitter::primvar(const std::string & name) {
    for (std::size_t i = 0; i < primvars_.size(); ++i) {
        if (primvars_[i] == name) {
            return static_cast<std::uint32_t>(i);
        }
    }
    primvars_.push_back(name);
    return static_cast<std::uint32_t>(primvars_.size() - 1U);
}

std::optional<std::uint32_t> Emitter::geomprop(const mx::InputPtr & declared) {
    const std::string & name = declared->getDefaultGeomPropString();
    const std::string key = keyed("geomprop " + name);
    const auto found = outputs_.find(key);
    if (found != outputs_.end()) {
        return found->second;
    }
    const mx::GeomPropDefPtr def = declared->getDocument()->getGeomPropDef(name);
    const std::uint32_t width = widthOf(declared->getType());
    if (!def) {
        return std::nullopt;
    }
    const std::string & property = def->getGeomProp();
    Instruction out;
    out.width = width;
    const std::map<std::string, Op> directions { { "position", Op::Position }, { "normal", Op::Normal }, { "tangent", Op::Tangent }, { "bitangent", Op::Bitangent } };
    const auto direction = directions.find(property);
    if (property == "texcoord" && width == 2U) {
        const std::string & index = def->getIndex();
        out.op = Op::Primvar;
        out.in[0] = primvar(index.empty() || index == "0" ? "st" : "st" + index);
        out.in[1] = allocate(width);
        out.widths[1] = width;
    } else if (direction != directions.end() && width == 3U) {
        out.op = direction->second;
        out.in[0] = spaceOf(def->getSpace());
    } else {
        return std::nullopt;
    }
    out.out = allocate(width);
    instructions_.push_back(out);
    outputs_.insert_or_assign(key, out.out);
    return out.out;
}

std::optional<Instruction> Emitter::geometric(const mx::NodePtr & node, std::uint32_t width) {
    const std::string & category = node->getCategory();
    const auto text = [&](const std::string & name) {
        const mx::ValuePtr value = node->getInputValue(name);
        return value ? value->getValueString() : std::string();
    };
    Instruction out;
    out.width = width;
    if (category == "geompropvalue" || category == "texcoord" || category == "geomcolor") {
        std::string name = text("geomprop");
        if (category == "texcoord") {
            const std::string index = text("index");
            name = index.empty() || index == "0" ? "st" : "st" + index;
        } else if (category == "geomcolor") {
            name = "Cs";
        }
        if (name.empty() || (category == "texcoord" && width != 2U) || (category == "geomcolor" && width != 3U)) {
            return std::nullopt;
        }
        out.op = Op::Primvar;
        out.in[0] = primvar(name);
        const mx::InputPtr fallback = node->getNodeDef()->getActiveInput("default");
        out.in[1] = fallback ? operand(node, fallback) : allocate(width);
        out.widths[1] = width;
        return out;
    }
    const std::map<std::string, Op> transforms { { "transformpoint", Op::TransformPoint }, { "transformvector", Op::TransformVector },
        { "transformnormal", Op::TransformNormal } };
    const auto transform = transforms.find(category);
    if (transform != transforms.end()) {
        const mx::InputPtr given = node->getNodeDef()->getActiveInput("in");
        if (width != 3U || !given || widthOf(given->getType()) != 3U) {
            return std::nullopt;
        }
        out.op = transform->second;
        out.in[0] = operand(node, given);
        out.widths[0] = 3U;
        out.in[1] = spaceOf(text("fromspace"));
        out.in[2] = spaceOf(text("tospace"));
        return out;
    }
    const std::map<std::string, Op> directions { { "position", Op::Position }, { "normal", Op::Normal }, { "tangent", Op::Tangent }, { "bitangent", Op::Bitangent } };
    const auto found = directions.find(category);
    if (found == directions.end() || width != 3U) {
        return std::nullopt;
    }
    out.op = found->second;
    out.in[0] = text("space") == "world" ? kernel::program::kWorld : kernel::program::kObject;
    return out;
}

std::string Emitter::textOf(const mx::NodePtr & node, const std::string & name) {
    const mx::InputPtr given = node->getInput(name);
    if (given && given->hasValue()) {
        return given->getValueString();
    }
    const mx::InputPtr declared = node->getNodeDef()->getActiveInput(name);
    return declared ? declared->getValueString() : std::string();
}

std::optional<Instruction> Emitter::image(const mx::NodePtr & node, std::uint32_t width) {
    const mx::NodeDefPtr def = node->getNodeDef();
    const mx::InputPtr coordinate = def->getActiveInput("texcoord");
    const mx::InputPtr fallback = def->getActiveInput("default");
    if (width < 1U || width > 4U || !coordinate || widthOf(coordinate->getType()) != 2U || !fallback || widthOf(fallback->getType()) != width) {
        return std::nullopt;
    }
    Instruction out;
    out.op = Op::Image;
    out.width = width;
    out.widths[0] = 2U;
    out.widths[1] = width;
    const auto mode = [](const std::string & name) -> std::uint32_t {
        const std::vector<std::string> modes { "constant", "clamp", "periodic", "mirror" };
        for (std::size_t i = 0; i < modes.size(); ++i) {
            if (modes[i] == name) {
                return static_cast<std::uint32_t>(i);
            }
        }
        return 2U;
    };
    out.in[3] = mode(textOf(node, "uaddressmode")) + (kernel::program::kAddressModes * mode(textOf(node, "vaddressmode")));
    const std::string filter = textOf(node, "filtertype");
    out.in[4] = static_cast<std::uint32_t>(kernel::texture::Filtering::Linear);
    if (filter == "closest") {
        out.in[4] = static_cast<std::uint32_t>(kernel::texture::Filtering::Closest);
    } else if (filter == "cubic") {
        out.in[4] = static_cast<std::uint32_t>(kernel::texture::Filtering::Cubic);
    }

    const mx::InputPtr file = node->getInput("file");
    const std::string path = file ? file->getResolvedValueString() : std::string();
    out.in[2] = kernel::texture::kNoTexture;
    if (!path.empty()) {
        const std::string type = node->getType();
        const texture::Request request { .file = path,
            .colourSpace = (file ? file : def->getActiveInput("file"))->getActiveColorSpace(),
            .colour = type == "color3" || type == "color4" };
        std::size_t at = 0;
        while (at < images_.size() && !(images_[at] == request)) {
            ++at;
        }
        if (at == images_.size()) {
            images_.push_back(request);
        }
        out.in[2] = static_cast<std::uint32_t>(at);
    }
    out.in[0] = operand(node, coordinate);
    out.in[1] = operand(node, fallback);
    return out;
}

void Emitter::shift(std::uint32_t axis) {
    Instruction out;
    out.op = Op::Shift;
    out.width = 0;
    out.in[0] = axis;
    instructions_.push_back(out);
    shift_ = axis;
}

std::uint32_t Emitter::combined(const std::uint32_t (&from)[3], std::uint32_t width) {
    Instruction out;
    out.op = Op::Combine;
    out.width = 3U * width;
    for (std::size_t i = 0; i < 3U; ++i) {
        out.in[i] = from[i];
        out.widths[i] = width;
    }
    out.out = allocate(out.width);
    instructions_.push_back(out);
    return out.out;
}

std::optional<Instruction> Emitter::heightToNormal(const mx::NodePtr & node) {
    const mx::NodeDefPtr def = node->getNodeDef();
    const mx::InputPtr height = def->getActiveInput("in");
    const mx::InputPtr scale = def->getActiveInput("scale");
    const mx::InputPtr coordinate = def->getActiveInput("texcoord");
    if (measuring_ || !height || !scale || !coordinate || widthOf(height->getType()) != 1U || widthOf(scale->getType()) != 1U ||
        widthOf(coordinate->getType()) != 2U) {
        return std::nullopt;
    }
    std::uint32_t heights[3] {};
    std::uint32_t coordinates[3] {};
    // each axis in turn, then back at the point, where the program carries on
    measuring_ = true;
    for (const std::uint32_t axis : { 1U, 2U, 0U }) {
        shift(axis);
        heights[axis] = operand(node, height);
        coordinates[axis] = operand(node, coordinate);
    }
    measuring_ = false;
    Instruction out;
    out.op = Op::HeightToNormal;
    out.width = 3;
    out.in[0] = combined(heights, 1U);
    out.widths[0] = 3;
    out.in[1] = operand(node, scale);
    out.widths[1] = 1;
    out.in[2] = combined(coordinates, 2U);
    out.widths[2] = 3U * 2U;  // three coordinates of two components
    return out;
}

std::optional<Instruction> Emitter::instruction(const mx::NodePtr & node) {
    const mx::NodeDefPtr def = node->getNodeDef();
    const mx::OutputPtr output = outputOf(def, std::string());
    if (!def || !output || widthOf(output->getType()) == 0U) {
        return std::nullopt;
    }
    const std::optional<Instruction> reading = geometric(node, widthOf(output->getType()));
    if (reading) {
        return reading;
    }
    if (node->getCategory() == "image") {
        return image(node, widthOf(output->getType()));
    }
    if (node->getCategory() == "heighttonormal") {
        return heightToNormal(node);
    }
    std::optional<Operation> operation = Operation::of(node->getCategory());
    if (node->getCategory() == "creatematrix") {
        // rows of a matrix: three vector3, four vector4, or three vector3 and a translation
        std::vector<std::string> rows { "in1", "in2", "in3" };
        if (def->getActiveInput("in4")) {
            rows.emplace_back("in4");
        }
        const mx::InputPtr first = def->getActiveInput("in1");
        const bool translated = rows.size() == 4U && first && first->getType() == "vector3";
        operation = Operation { .op = translated ? Op::Rows : Op::Combine, .inputs = rows };
    }
    if (!operation || operation->inputs.size() > kernel::program::kMostOperands) {
        return std::nullopt;
    }
    Instruction out;
    out.op = operation->op;
    out.width = widthOf(output->getType());
    std::vector<mx::InputPtr> declared;
    for (const std::string & name : operation->inputs) {
        const mx::InputPtr input = def->getActiveInput(name);
        if (!input || widthOf(input->getType()) == 0U) {
            return std::nullopt;
        }
        declared.push_back(input);
    }
    for (std::size_t i = 0; i < declared.size(); ++i) {
        out.widths[i] = widthOf(declared[i]->getType());
    }
    // a product or quotient of matrices is the matrix product, not one taken by components
    if ((out.op == Op::Multiply || out.op == Op::Divide) && isMatrix(out.width) && out.widths[0] == out.width && out.widths[1] == out.width) {
        out.op = out.op == Op::Multiply ? Op::Product : Op::Quotient;
    }
    if (!shaped(out, declared.size())) {
        return std::nullopt;
    }
    for (std::size_t i = 0; i < declared.size(); ++i) {
        out.in[i] = operand(node, declared[i]);
    }
    return out;
}

};  // namespace v3d::moya::material::internal
