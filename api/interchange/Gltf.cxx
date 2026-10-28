/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Gltf.h"

#include <api/asset/File.h>
#include <api/type/Transform.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <numeric>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Placement.h"
#include "Scene.h"

// cgltf.h is a C header, and cpplint sorts it with the C system headers rather than with
// the third party ones. Its implementation half is compiled once, in CgltfImpl.cxx.
#include <cgltf.h>  // NOLINT(build/include_order)
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::interchange {

namespace {

/**
 * cgltf's file reader. The document and every buffer it names are read through
 * asset::readFile, as every other file the api reads is. cgltf has already joined a
 * buffer's uri to the document's directory and decoded its percent-escapes.
 **/
cgltf_result readThroughAssets(const cgltf_memory_options* /* memory */, const cgltf_file_options* /* file */,
    const char* path, cgltf_size* size, void** data) {
    const std::optional<std::string> contents = v3d::asset::readFile(path);
    if (!contents) {
        return cgltf_result_file_not_found;
    }
    // cgltf frees what it is given through releaseRead()
    void* copy = std::malloc(contents->empty() ? 1 : contents->size());
    if (copy == nullptr) {
        return cgltf_result_out_of_memory;
    }
    std::copy(contents->begin(), contents->end(), static_cast<char*>(copy));
    *size = contents->size();
    *data = copy;
    return cgltf_result_success;
}

void releaseRead(const cgltf_memory_options* /* memory */, const cgltf_file_options* /* file */, void* data) {
    std::free(data);  // NOLINT(cppcoreguidelines-no-malloc) - cgltf hands back what readThroughAssets() allocated
}

/**
 * glTF gives a point or spot light in candela and a distant one in lux. Dividing by the
 * luminous efficacy of light at 555 nanometres gives watts per steradian or per square metre.
 **/
constexpr float LUMENS_PER_WATT = 683.0F;

std::string nameOf(const char* name) {
    return name != nullptr ? std::string(name) : std::string();
}

/**
 * A node's placement, from its matrix when it has one.
 **/
v3d::type::Transform placementOf(const cgltf_node& node) {
    if (node.has_matrix != 0) {
        return v3d::interchange::placementOf(glm::make_mat4(node.matrix));
    }
    v3d::type::Transform placement;
    if (node.has_translation != 0) {
        placement.position = glm::make_vec3(node.translation);
    }
    if (node.has_rotation != 0) {
        // glTF writes a quaternion x, y, z, w, and glm's constructor takes w first
        placement.rotation = glm::normalize(glm::quat(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]));
    }
    if (node.has_scale != 0) {
        placement.scale = glm::make_vec3(node.scale);
    }
    return placement;
}

/**
 * The triangles of one primitive as indices into its own points, counter-clockwise as glTF
 * winds them. A strip turns every other triangle round, and a fan shares its first point.
 **/
std::vector<std::uint32_t> triangleIndices(const cgltf_primitive& primitive, cgltf_size count) {
    std::vector<std::uint32_t> order;
    if (primitive.indices != nullptr) {
        order.resize(primitive.indices->count);
        for (cgltf_size index = 0; index < order.size(); index++) {
            order[index] = static_cast<std::uint32_t>(cgltf_accessor_read_index(primitive.indices, index));
        }
    } else {
        order.resize(count);
        std::iota(order.begin(), order.end(), 0U);
    }
    if (primitive.type == cgltf_primitive_type_triangles) {
        order.resize(order.size() - (order.size() % 3));
        return order;
    }
    std::vector<std::uint32_t> triangles;
    const bool strip = primitive.type == cgltf_primitive_type_triangle_strip;
    for (std::size_t first = 0; first + 2 < order.size(); first++) {
        if (strip) {
            const bool swapped = (first % 2) == 1;
            triangles.insert(triangles.end(), { order[swapped ? first + 1 : first], order[swapped ? first : first + 1],
                order[first + 2] });
        } else {
            triangles.insert(triangles.end(), { order[first + 1], order[first + 2], order[0] });
        }
    }
    return triangles;
}

const cgltf_accessor* positionsOf(const cgltf_primitive& primitive) {
    for (cgltf_size index = 0; index < primitive.attributes_count; index++) {
        if (primitive.attributes[index].type == cgltf_attribute_type_position) {
            return primitive.attributes[index].data;
        }
    }
    return nullptr;
}

/**
 * Fills a scene from a file cgltf has parsed, loaded and validated.
 **/
class Reader final {
 public:
    explicit Reader(const cgltf_data& data) : data_(data) {
    }

    Imported read() {
        for (const cgltf_node* root : roots()) {
            addTree(*root);
        }
        for (cgltf_size index = 0; index < data_.animations_count; index++) {
            addAnimation(data_.animations[index]);
        }
        Imported imported;
        imported.scene = std::move(scene_);
        imported.notes = std::move(notes_);
        return imported;
    }

 private:
    /**
     * The nodes the walk starts from: the scene the file names, or its first, or every root
     * when it has none.
     **/
    std::vector<const cgltf_node*> roots() const {
        std::vector<const cgltf_node*> found;
        const cgltf_scene* scene = data_.scene;
        if (scene == nullptr && data_.scenes_count > 0) {
            scene = &data_.scenes[0];
        }
        if (scene != nullptr) {
            found.assign(scene->nodes, scene->nodes + scene->nodes_count);
            return found;
        }
        for (cgltf_size index = 0; index < data_.nodes_count; index++) {
            if (data_.nodes[index].parent == nullptr) {
                found.push_back(&data_.nodes[index]);
            }
        }
        return found;
    }

    /**
     * Add a node and everything beneath it, parents first. A node met a second time, which only
     * a malformed file can do, is not added again.
     **/
    void addTree(const cgltf_node& root) {
        std::vector<std::pair<const cgltf_node*, std::optional<std::size_t>>> pending{ { &root, std::nullopt } };
        while (!pending.empty()) {
            const auto [node, parent] = pending.back();
            pending.pop_back();
            if (nodes_.contains(node)) {
                continue;
            }
            const std::size_t index = addNode(*node, parent);
            // pushed last to first, so that children are added in the file's order
            for (cgltf_size child = node->children_count; child > 0; child--) {
                pending.emplace_back(node->children[child - 1], index);
            }
        }
    }

    std::size_t addNode(const cgltf_node& source, std::optional<std::size_t> parent) {
        Node node;
        node.name = nameOf(source.name);
        node.parent = parent;
        node.placement = placementOf(source);
        if (source.mesh != nullptr) {
            node.mesh = mesh(*source.mesh);
        }
        if (source.camera != nullptr) {
            node.camera = camera(*source.camera);
        }
        if (source.light != nullptr) {
            node.light = light(*source.light);
        }
        if (source.skin != nullptr) {
            notes_.push_back("node " + node.name + " is skinned, and its mesh is read at rest");
        }
        const std::size_t index = scene_.nodes.size();
        nodes_.emplace(&source, index);
        scene_.nodes.push_back(std::move(node));
        return index;
    }

    std::size_t mesh(const cgltf_mesh& source) {
        const auto found = meshes_.find(&source);
        if (found != meshes_.end()) {
            return found->second;
        }
        Mesh mesh;
        mesh.name = nameOf(source.name);
        mesh.triangulated = true;
        for (cgltf_size index = 0; index < source.primitives_count; index++) {
            addPrimitive(source.primitives[index], &mesh);
        }
        const std::size_t index = scene_.meshes.size();
        meshes_.emplace(&source, index);
        scene_.meshes.push_back(std::move(mesh));
        return index;
    }

    void addPrimitive(const cgltf_primitive& primitive, Mesh* mesh) {
        if (primitive.type != cgltf_primitive_type_triangles && primitive.type != cgltf_primitive_type_triangle_strip &&
            primitive.type != cgltf_primitive_type_triangle_fan) {
            notes_.push_back("mesh " + mesh->name + " has a primitive of points or lines, which is left out");
            return;
        }
        const cgltf_accessor* positions = positionsOf(primitive);
        if (positions == nullptr) {
            notes_.push_back("mesh " + mesh->name + " has a primitive with no positions, which is left out");
            return;
        }
        if (primitive.targets_count > 0) {
            notes_.push_back("mesh " + mesh->name + " has morph targets, which are left out");
        }
        const auto base = static_cast<std::uint32_t>(mesh->points.size());
        for (cgltf_size index = 0; index < positions->count; index++) {
            glm::vec3 point(0.0F);
            cgltf_accessor_read_float(positions, index, glm::value_ptr(point), 3);
            mesh->points.push_back(point);
        }
        for (const std::uint32_t corner : triangleIndices(primitive, positions->count)) {
            mesh->indices.push_back(base + corner);
        }
        mesh->counts.resize(mesh->indices.size() / 3, 3);
    }

    std::size_t camera(const cgltf_camera& source) {
        const auto found = cameras_.find(&source);
        if (found != cameras_.end()) {
            return found->second;
        }
        Camera camera;
        camera.name = nameOf(source.name);
        if (source.type == cgltf_camera_type_orthographic) {
            const cgltf_camera_orthographic& lens = source.data.orthographic;
            camera.projection = Camera::Projection::Orthographic;
            camera.magnification = glm::vec2(lens.xmag, lens.ymag);
            camera.clipping = glm::vec2(lens.znear, lens.zfar);
        } else {
            const cgltf_camera_perspective& lens = source.data.perspective;
            camera.yfov = lens.yfov;
            camera.aspect = lens.has_aspect_ratio != 0 ? lens.aspect_ratio : 0.0F;
            camera.clipping = glm::vec2(lens.znear, lens.has_zfar != 0 ? lens.zfar : 0.0F);
        }
        const std::size_t index = scene_.cameras.size();
        cameras_.emplace(&source, index);
        scene_.cameras.push_back(std::move(camera));
        return index;
    }

    std::size_t light(const cgltf_light& source) {
        const auto found = lights_.find(&source);
        if (found != lights_.end()) {
            return found->second;
        }
        Light light;
        light.name = nameOf(source.name);
        light.colour = glm::make_vec3(source.color);
        light.intensity = source.intensity / LUMENS_PER_WATT;
        if (source.type == cgltf_light_type_directional) {
            light.type = Light::Type::Distant;
        } else if (source.type == cgltf_light_type_spot) {
            light.type = Light::Type::Spot;
            light.outerCone = source.spot_outer_cone_angle;
            light.innerCone = source.spot_inner_cone_angle;
        }
        const std::size_t index = scene_.lights.size();
        lights_.emplace(&source, index);
        scene_.lights.push_back(std::move(light));
        return index;
    }

    void addAnimation(const cgltf_animation& animation) {
        for (cgltf_size index = 0; index < animation.channels_count; index++) {
            const cgltf_animation_channel& channel = animation.channels[index];
            const auto node = nodes_.find(channel.target_node);
            if (node == nodes_.end() || channel.sampler == nullptr) {
                continue;
            }
            Channel read;
            read.node = node->second;
            if (channel.target_path == cgltf_animation_path_type_translation) {
                read.path = Channel::Path::Translation;
            } else if (channel.target_path == cgltf_animation_path_type_rotation) {
                read.path = Channel::Path::Rotation;
            } else if (channel.target_path == cgltf_animation_path_type_scale) {
                read.path = Channel::Path::Scale;
            } else {
                notes_.push_back("node " + scene_.nodes[read.node].name + " has keyed morph weights, which are left out");
                continue;
            }
            if (!keyed_.insert({ read.node, read.path }).second) {
                notes_.push_back("node " + scene_.nodes[read.node].name +
                    " has a part keyed by more than one channel, and only the first is read");
                continue;
            }
            readKeys(*channel.sampler, &read);
            scene_.channels.push_back(std::move(read));
        }
    }

    /**
     * A cubic spline sampler stores three values a key: the tangent in, the value and the
     * tangent out.
     **/
    static void readKeys(const cgltf_animation_sampler& sampler, Channel* channel) {
        if (sampler.interpolation == cgltf_interpolation_type_step) {
            channel->interpolation = Channel::Interpolation::Step;
        } else if (sampler.interpolation == cgltf_interpolation_type_cubic_spline) {
            channel->interpolation = Channel::Interpolation::CubicSpline;
        }
        channel->times.resize(sampler.input->count);
        for (cgltf_size key = 0; key < sampler.input->count; key++) {
            cgltf_accessor_read_float(sampler.input, key, &channel->times[key], 1);
        }
        const cgltf_size width = channel->path == Channel::Path::Rotation ? 4 : 3;
        std::vector<glm::vec4> outputs(sampler.output->count, glm::vec4(0.0F));
        for (cgltf_size value = 0; value < outputs.size(); value++) {
            cgltf_accessor_read_float(sampler.output, value, glm::value_ptr(outputs[value]), width);
        }
        if (channel->interpolation != Channel::Interpolation::CubicSpline) {
            channel->values = std::move(outputs);
            return;
        }
        for (std::size_t key = 0; (key * 3) + 2 < outputs.size(); key++) {
            channel->inTangents.push_back(outputs[key * 3]);
            channel->values.push_back(outputs[(key * 3) + 1]);
            channel->outTangents.push_back(outputs[(key * 3) + 2]);
        }
    }

    const cgltf_data& data_;
    Scene scene_;
    std::vector<std::string> notes_;
    std::unordered_map<const cgltf_node*, std::size_t> nodes_;
    std::unordered_map<const cgltf_mesh*, std::size_t> meshes_;
    std::unordered_map<const cgltf_camera*, std::size_t> cameras_;
    std::unordered_map<const cgltf_light*, std::size_t> lights_;
    std::set<std::pair<std::size_t, Channel::Path>> keyed_;
};

};  // namespace

Imported readGltf(const std::string& path) {
    Imported imported;
    cgltf_options options{};
    options.file.read = readThroughAssets;
    options.file.release = releaseRead;
    cgltf_data* data = nullptr;
    if (cgltf_parse_file(&options, path.c_str(), &data) != cgltf_result_success) {
        imported.problem = "could not read " + path + " as glTF";
        return imported;
    }
    // a .gltf names its buffers and a .glb holds them. Either way, no accessor can be read
    // until they are loaded. cgltf reads an accessor without checking its bounds, and
    // cgltf_validate() checks every accessor against its buffer before any is read
    if (cgltf_load_buffers(&options, data, path.c_str()) != cgltf_result_success) {
        imported.problem = "could not load the buffers of " + path;
    } else if (cgltf_validate(data) != cgltf_result_success) {
        imported.problem = path + " is not valid glTF";
    } else {
        imported = Reader(*data).read();
    }
    cgltf_free(data);
    return imported;
}

};  // namespace v3d::interchange
