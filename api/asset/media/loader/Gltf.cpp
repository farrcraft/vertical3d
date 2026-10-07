/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Gltf.h"

#include <api/asset/Type.h>
#include <api/asset/media/kind/Model.h>
#include <api/image/Factory.h>
#include <api/type/animation/Channel.h>
#include <api/type/animation/Clip.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <map>
#include <numeric>
#include <string>
#include <vector>

// cgltf.h is a C header, and cpplint sorts it with the C system headers rather than with
// the third party ones. Its implementation half is compiled once, in CgltfImpl.cpp.
#include <cgltf.h>  // NOLINT(build/include_order)

#include <boost/make_shared.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>

namespace v3d::asset::media::loader {

namespace {

/**
 * The accessors one primitive draws from. Position is the only one a primitive has to have:
 * without it there is no geometry, and the rest default to zero.
 **/
struct Attributes final {
    const cgltf_accessor* position{ nullptr };
    const cgltf_accessor* normal{ nullptr };
    const cgltf_accessor* uv{ nullptr };
    const cgltf_accessor* joints{ nullptr };
    const cgltf_accessor* weights{ nullptr };
    bool moreInfluences{ false };  /**< a second set of joints, which four influences cannot hold **/
};

Attributes attributesOf(const cgltf_primitive& primitive) {
    Attributes found;
    for (cgltf_size index = 0; index < primitive.attributes_count; ++index) {
        const cgltf_attribute& attribute = primitive.attributes[index];
        switch (attribute.type) {
            case cgltf_attribute_type_position:
                found.position = attribute.data;
                break;
            case cgltf_attribute_type_normal:
                found.normal = attribute.data;
                break;
            case cgltf_attribute_type_texcoord:
                // the first set only - a second one is a lightmap or a detail layer, and
                // the vertex layout carries one
                if (attribute.index == 0) {
                    found.uv = attribute.data;
                }
                break;
            case cgltf_attribute_type_joints:
                if (attribute.index == 0) {
                    found.joints = attribute.data;
                } else {
                    found.moreInfluences = true;
                }
                break;
            case cgltf_attribute_type_weights:
                if (attribute.index == 0) {
                    found.weights = attribute.data;
                }
                break;
            default:
                break;
        }
    }
    return found;
}

/**
 * The skin a model keeps, and how to reach its joints in the skeleton's order.
 **/
struct Rig final {
    const cgltf_skin* skin{ nullptr };
    std::vector<std::uint16_t> order;                     /**< a skin joint's index to the skeleton's **/
    std::map<const cgltf_node*, std::uint16_t> joints;    /**< a joint's node to the skeleton's index **/
    std::vector<glm::mat4> bound;                         /**< by skeleton index: the joint at rest, times its inverse bind **/
};

/**
 * What the traversal found that a model cannot hold, for the loader to report.
 **/
struct Dropped final {
    bool skins{ false };       /**< a mesh bound to a skin other than the one kept **/
    bool influences{ false };  /**< a second set of joints and weights **/
    bool unweighted{ false };  /**< a skinned primitive with no joints, or none that weigh anything **/
    bool channels{ false };    /**< a channel animating a node that is not one of the skeleton's joints **/
    bool lines{ false };       /**< a primitive of points or lines, which a model of triangles cannot hold **/
};

/**
 * A primitive the traversal reached, and where its node puts it. In a model with a skeleton, a
 * skinned primitive is placed by its joints and an unskinned one follows one joint rigidly.
 **/
struct Placed final {
    const cgltf_primitive* primitive{ nullptr };
    Attributes attributes;
    glm::mat4 world{ 1.0f };
    bool skinned{ false };
    std::uint16_t joint{ 0 };  /**< the joint an unskinned primitive follows **/
};

/**
 * The primitives drawn with one material, which become one part. The material is null for
 * primitives that name none.
 **/
struct Bucket final {
    const cgltf_material* material{ nullptr };
    std::vector<Placed> primitives;
};

/**
 * The nodes the traversal starts from: the scene the file names, or its first, or every root
 * when it has none.
 **/
std::vector<const cgltf_node*> roots(const cgltf_data& data) {
    std::vector<const cgltf_node*> found;
    const cgltf_scene* scene = data.scene;
    if (scene == nullptr && data.scenes_count > 0) {
        scene = &data.scenes[0];
    }
    if (scene != nullptr) {
        for (cgltf_size index = 0; index < scene->nodes_count; ++index) {
            found.push_back(scene->nodes[index]);
        }
        return found;
    }
    for (cgltf_size index = 0; index < data.nodes_count; ++index) {
        if (data.nodes[index].parent == nullptr) {
            found.push_back(&data.nodes[index]);
        }
    }
    return found;
}

/**
 * The skin of the first skinned mesh a traversal from this node reaches, or null.
 **/
const cgltf_skin* firstSkin(const cgltf_node& node) {
    if (node.mesh != nullptr && node.skin != nullptr) {
        return node.skin;
    }
    for (cgltf_size index = 0; index < node.children_count; ++index) {
        const cgltf_skin* skin = firstSkin(*node.children[index]);
        if (skin != nullptr) {
            return skin;
        }
    }
    return nullptr;
}

glm::mat4 worldOf(const cgltf_node& node) {
    glm::mat4 world(1.0f);
    cgltf_node_transform_world(&node, glm::value_ptr(world));
    return world;
}

/**
 * A joint's rest pose, local to its parent. A node a clip animates carries its transform as a
 * translation, a rotation and a scale, which the specification requires; one given as a matrix
 * is taken apart, which holds for any matrix without shear.
 **/
void restOf(const cgltf_node& node, v3d::type::Skeleton::Joint* joint) {
    if (node.has_matrix != 0) {
        const glm::mat4 local = glm::make_mat4(node.matrix);
        joint->translation = glm::vec3(local[3]);
        joint->scale = glm::vec3(glm::length(glm::vec3(local[0])), glm::length(glm::vec3(local[1])), glm::length(glm::vec3(local[2])));
        const glm::mat3 turn(glm::vec3(local[0]) / joint->scale.x, glm::vec3(local[1]) / joint->scale.y,
            glm::vec3(local[2]) / joint->scale.z);
        joint->rotation = glm::quat_cast(turn);
        return;
    }
    if (node.has_translation != 0) {
        joint->translation = glm::make_vec3(node.translation);
    }
    if (node.has_rotation != 0) {
        // glTF stores x, y, z, w and glm's constructor takes w first
        joint->rotation = glm::quat(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]);
    }
    if (node.has_scale != 0) {
        joint->scale = glm::make_vec3(node.scale);
    }
}

/**
 * The skin's parent of each of its joints: the nearest ancestor that is also one of its
 * joints, by its index in the skin, or -1.
 **/
std::vector<int32_t> skinParents(const cgltf_skin& skin) {
    std::map<const cgltf_node*, int32_t> index;
    for (cgltf_size joint = 0; joint < skin.joints_count; ++joint) {
        index[skin.joints[joint]] = static_cast<int32_t>(joint);
    }
    std::vector<int32_t> parents(skin.joints_count, -1);
    for (cgltf_size joint = 0; joint < skin.joints_count; ++joint) {
        for (const cgltf_node* above = skin.joints[joint]->parent; above != nullptr; above = above->parent) {
            const std::map<const cgltf_node*, int32_t>::const_iterator found = index.find(above);
            if (found != index.end()) {
                parents[joint] = found->second;
                break;
            }
        }
    }
    return parents;
}

/**
 * Read a skin into a skeleton whose parents precede their children, keeping the skin's own
 * order wherever it already does, and record how the rest of the traversal reaches its joints.
 **/
v3d::type::Skeleton readSkeleton(const cgltf_skin& skin, Rig* rig) {
    const std::vector<int32_t> parents = skinParents(skin);
    rig->skin = &skin;
    rig->order.assign(skin.joints_count, 0);

    // repeatedly take every joint whose parent is already placed; a tree places at least one
    // joint a sweep, so this ends
    std::vector<bool> placed(skin.joints_count, false);
    std::vector<cgltf_size> sequence;
    while (sequence.size() < skin.joints_count) {
        for (cgltf_size joint = 0; joint < skin.joints_count; ++joint) {
            if (!placed[joint] && (parents[joint] < 0 || placed[static_cast<std::size_t>(parents[joint])])) {
                rig->order[joint] = static_cast<std::uint16_t>(sequence.size());
                placed[joint] = true;
                sequence.push_back(joint);
            }
        }
    }

    v3d::type::Skeleton skeleton;
    for (const cgltf_size joint : sequence) {
        const cgltf_node& node = *skin.joints[joint];
        v3d::type::Skeleton::Joint read;
        read.name = node.name != nullptr ? node.name : std::string();
        read.parent = parents[joint] < 0 ? -1 : rig->order[static_cast<std::size_t>(parents[joint])];
        restOf(node, &read);
        if (skin.inverse_bind_matrices != nullptr) {
            cgltf_accessor_read_float(skin.inverse_bind_matrices, joint, glm::value_ptr(read.inverseBind), 16);
        }
        rig->joints[&node] = static_cast<std::uint16_t>(skeleton.joints.size());
        rig->bound.push_back(worldOf(node) * read.inverseBind);
        skeleton.joints.push_back(read);
    }

    // whatever stands above the skeleton - an armature's scale, typically - is the root's
    const cgltf_node* above = skin.joints[sequence.front()]->parent;
    if (above != nullptr) {
        skeleton.root = worldOf(*above);
    }
    return skeleton;
}

/**
 * Whether a skin's inverse bind matrices can be read for every one of its joints. They are read
 * by joint index, and cgltf checks neither that index against the accessor's count nor the
 * accessor's type. A skin without them binds every joint by the identity.
 **/
bool bindsEveryJoint(const cgltf_skin& skin) {
    const cgltf_accessor* matrices = skin.inverse_bind_matrices;
    return matrices == nullptr || (matrices->type == cgltf_type_mat4 && matrices->count >= skin.joints_count);
}

/**
 * The joint an unskinned mesh in a skinned model follows: its own node or nearest ancestor that
 * is a joint, or the first root when it is under none.
 **/
std::uint16_t followed(const cgltf_node& node, const Rig& rig) {
    for (const cgltf_node* at = &node; at != nullptr; at = at->parent) {
        const std::map<const cgltf_node*, std::uint16_t>::const_iterator found = rig.joints.find(at);
        if (found != rig.joints.end()) {
            return found->second;
        }
    }
    return 0;
}

/**
 * Where a node's primitives are placed. Without a skeleton that is the node's world matrix. A
 * mesh bound to the kept skin is placed by its joints alone, which the specification requires,
 * so its own node is ignored. Any other mesh follows a joint, and is taken into the space that
 * joint's skinning matrix expects, so that it stands where the file put it while the joint is
 * at rest.
 **/
Placed placement(const cgltf_node& node, const Rig& rig, Dropped* dropped) {
    Placed placed;
    if (rig.skin == nullptr) {
        placed.world = worldOf(node);
        return placed;
    }
    if (node.skin == rig.skin) {
        placed.skinned = true;
        return placed;
    }
    if (node.skin != nullptr) {
        dropped->skins = true;
    }
    placed.joint = followed(node, rig);
    placed.world = glm::inverse(rig.bound[placed.joint]) * worldOf(node);
    return placed;
}

/**
 * Put every primitive of a node's mesh, and of its children's, into the bucket of its
 * material, opening a bucket the first time a material is reached. Meshes are reached through
 * the node hierarchy rather than the file's mesh list, so each is placed by its node.
 **/
void collect(const cgltf_node& node, const Rig& rig, std::vector<Bucket>* buckets, Dropped* dropped) {
    if (node.mesh != nullptr) {
        const Placed where = placement(node, rig, dropped);

        for (cgltf_size index = 0; index < node.mesh->primitives_count; ++index) {
            const cgltf_primitive& primitive = node.mesh->primitives[index];
            const Attributes attributes = attributesOf(primitive);
            if (attributes.position == nullptr) {
                continue;
            }
            // a strip and a fan become a list as they are appended; points and lines are not
            // triangles at all
            if (primitive.type != cgltf_primitive_type_triangles && primitive.type != cgltf_primitive_type_triangle_strip &&
                primitive.type != cgltf_primitive_type_triangle_fan) {
                dropped->lines = true;
                continue;
            }
            std::vector<Bucket>::iterator bucket = buckets->begin();
            while (bucket != buckets->end() && bucket->material != primitive.material) {
                ++bucket;
            }
            if (bucket == buckets->end()) {
                buckets->push_back(Bucket{ primitive.material, {} });
                bucket = buckets->end() - 1;
            }
            Placed placed = where;
            placed.primitive = &primitive;
            placed.attributes = attributes;
            bucket->primitives.push_back(placed);
        }
    }
    for (cgltf_size index = 0; index < node.children_count; ++index) {
        collect(*node.children[index], rig, buckets, dropped);
    }
}

/**
 * A skinned vertex's influence, in the skeleton's joint order and with weights that sum to
 * one. One whose weights sum to nothing follows the first root, as an unweighted vertex has
 * nothing else to follow.
 **/
v3d::type::Model::Influence influenceOf(const Attributes& attributes, cgltf_size index, const Rig& rig, Dropped* dropped) {
    v3d::type::Model::Influence influence;
    influence.weights = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
    if (attributes.joints == nullptr || attributes.weights == nullptr) {
        dropped->unweighted = true;
        return influence;
    }
    cgltf_uint joints[4] = {0, 0, 0, 0};
    glm::vec4 weights(0.0f);
    cgltf_accessor_read_uint(attributes.joints, index, joints, 4);
    // an integer weight reads as its normalised value
    cgltf_accessor_read_float(attributes.weights, index, glm::value_ptr(weights), 4);
    const float sum = weights.x + weights.y + weights.z + weights.w;
    if (sum <= 0.0f) {
        dropped->unweighted = true;
        return influence;
    }
    for (int slot = 0; slot < 4; ++slot) {
        influence.joints[slot] = joints[slot] < rig.order.size() ? rig.order[joints[slot]] : 0;
    }
    influence.weights = weights / sum;
    return influence;
}

/**
 * Append a primitive's triangles to the merged index list. They are appended as a list,
 * whatever mode the primitive was drawn in.
 *
 * A primitive's indices address its own vertices, so every one after the first primitive
 * addresses the wrong geometry unless it is offset into the merge. One drawn straight out of
 * its vertex array still has to be indexed once it is merged, or it is lost.
 *
 * A strip or a fan becomes a list wound as glTF winds them. Every other triangle of a strip
 * swaps its first two corners, so that all of them face the same way. Every triangle of a fan
 * shares the fan's first vertex.
 **/
void appendIndices(const cgltf_primitive& primitive, std::size_t baseVertex, cgltf_size count,
    std::vector<std::uint32_t>* indices) {
    std::vector<std::uint32_t> order;
    if (primitive.indices != nullptr) {
        order.resize(primitive.indices->count);
        for (cgltf_size index = 0; index < order.size(); ++index) {
            order[index] = static_cast<std::uint32_t>(cgltf_accessor_read_index(primitive.indices, index) + baseVertex);
        }
    } else {
        order.resize(count);
        std::iota(order.begin(), order.end(), static_cast<std::uint32_t>(baseVertex));
    }

    if (primitive.type == cgltf_primitive_type_triangles) {
        indices->insert(indices->end(), order.begin(), order.end());
        return;
    }
    const bool strip = primitive.type == cgltf_primitive_type_triangle_strip;
    for (std::size_t first = 0; first + 2 < order.size(); ++first) {
        const bool swapped = strip && (first % 2) == 1;
        if (strip) {
            indices->push_back(order[swapped ? first + 1 : first]);
            indices->push_back(order[swapped ? first : first + 1]);
            indices->push_back(order[first + 2]);
        } else {
            indices->push_back(order[first + 1]);
            indices->push_back(order[first + 2]);
            indices->push_back(order[0]);
        }
    }
}

/**
 * Append one primitive's vertices to the merged array, placed by its node and with its
 * indices rebased onto the array.
 **/
void appendPrimitive(const Placed& placed, const Rig& rig, v3d::type::Model* model, Dropped* dropped) {
    const cgltf_primitive& primitive = *placed.primitive;
    const Attributes& attributes = placed.attributes;
    // a normal is carried by the inverse transpose, so a node scaled unevenly keeps it
    // perpendicular to its surface, and renormalised because a scale changes its length
    const glm::mat3 normals = glm::transpose(glm::inverse(glm::mat3(placed.world)));

    const std::size_t baseVertex = model->vertices().size();
    const cgltf_size count = attributes.position->count;
    model->vertices().resize(baseVertex + count);

    for (cgltf_size index = 0; index < count; ++index) {
        v3d::type::Model::Vertex& vertex = model->vertices()[baseVertex + index];
        cgltf_accessor_read_float(attributes.position, index, &vertex.position.x, 3);
        vertex.position = glm::vec3(placed.world * glm::vec4(vertex.position, 1.0f));
        if (attributes.normal != nullptr) {
            cgltf_accessor_read_float(attributes.normal, index, &vertex.normal.x, 3);
            const glm::vec3 turned = normals * vertex.normal;
            const float length = glm::length(turned);
            vertex.normal = length > 0.0f ? turned / length : turned;
        }
        if (attributes.uv != nullptr) {
            cgltf_accessor_read_float(attributes.uv, index, &vertex.uv.x, 2);
        }
    }

    if (rig.skin != nullptr) {
        dropped->influences = dropped->influences || attributes.moreInfluences;
        for (cgltf_size index = 0; index < count; ++index) {
            v3d::type::Model::Influence influence;
            influence.joints[0] = placed.joint;
            influence.weights = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
            model->influences().push_back(placed.skinned ? influenceOf(attributes, index, rig, dropped) : influence);
        }
    }

    appendIndices(primitive, baseVertex, count, &model->indices());
}

/**
 * One channel of a clip, or nothing when the channel animates a node that is not a joint, or a
 * path a pose has no room for. Morph target weights are dropped without a report, since a
 * model has no morph targets to give them to.
 **/
bool readChannel(const cgltf_animation_channel& source, const Rig& rig, v3d::type::animation::Channel* channel,
    Dropped* dropped) {
    using v3d::type::animation::Channel;
    if (source.sampler == nullptr || source.target_node == nullptr) {
        return false;
    }
    switch (source.target_path) {
        case cgltf_animation_path_type_translation:
            channel->path = Channel::Path::Translation;
            break;
        case cgltf_animation_path_type_rotation:
            channel->path = Channel::Path::Rotation;
            break;
        case cgltf_animation_path_type_scale:
            channel->path = Channel::Path::Scale;
            break;
        default:
            return false;
    }
    const std::map<const cgltf_node*, std::uint16_t>::const_iterator joint = rig.joints.find(source.target_node);
    if (joint == rig.joints.end()) {
        dropped->channels = true;
        return false;
    }
    channel->joint = joint->second;

    const cgltf_animation_sampler& sampler = *source.sampler;
    switch (sampler.interpolation) {
        case cgltf_interpolation_type_step:
            channel->interpolation = Channel::Interpolation::Step;
            break;
        case cgltf_interpolation_type_cubic_spline:
            channel->interpolation = Channel::Interpolation::CubicSpline;
            break;
        default:
            channel->interpolation = Channel::Interpolation::Linear;
            break;
    }

    channel->times.resize(sampler.input->count);
    for (cgltf_size key = 0; key < sampler.input->count; ++key) {
        cgltf_accessor_read_float(sampler.input, key, &channel->times[key], 1);
    }
    // a translation or a scale is three floats and a rotation four; an integer rotation reads
    // as its normalised value
    const cgltf_size width = channel->path == Channel::Path::Rotation ? 4 : 3;
    channel->values.assign(sampler.output->count, glm::vec4(0.0f));
    for (cgltf_size value = 0; value < sampler.output->count; ++value) {
        cgltf_accessor_read_float(sampler.output, value, glm::value_ptr(channel->values[value]), width);
    }
    return true;
}

/**
 * Every animation in the file as a clip of the skeleton's joints, named as the file named it.
 **/
std::vector<v3d::type::animation::Clip> readClips(const cgltf_data& data, const Rig& rig, Dropped* dropped) {
    std::vector<v3d::type::animation::Clip> clips;
    for (cgltf_size index = 0; index < data.animations_count; ++index) {
        const cgltf_animation& animation = data.animations[index];
        v3d::type::animation::Clip clip;
        clip.name = animation.name != nullptr ? animation.name : std::string();
        for (cgltf_size channel = 0; channel < animation.channels_count; ++channel) {
            v3d::type::animation::Channel read;
            if (!readChannel(animation.channels[channel], rig, &read, dropped)) {
                continue;
            }
            if (!read.times.empty()) {
                clip.duration = std::max(clip.duration, read.times.back());
            }
            clip.channels.push_back(read);
        }
        clips.push_back(clip);
    }
    return clips;
}

/**
 * The base colour and the name of the image tinting it.
 *
 * @param embedded set to the image the file carries, where it carries one rather than
 *        naming it - decoded by the caller, which has the readers
 **/
v3d::type::Model::Material readMaterial(const cgltf_material& source, const cgltf_image** embedded) {
    v3d::type::Model::Material material;
    if (source.has_pbr_metallic_roughness == 0) {
        return material;
    }

    const cgltf_pbr_metallic_roughness& pbr = source.pbr_metallic_roughness;
    material.baseColour = glm::vec4(pbr.base_color_factor[0], pbr.base_color_factor[1],
        pbr.base_color_factor[2], pbr.base_color_factor[3]);

    const cgltf_texture* texture = pbr.base_color_texture.texture;
    if (texture == nullptr || texture->image == nullptr) {
        return material;
    }
    if (texture->image->uri != nullptr && std::strncmp(texture->image->uri, "data:", 5) != 0) {
        // a uri is percent-encoded, and the app resolves the file it names
        std::string decoded(texture->image->uri);
        decoded.resize(cgltf_decode_uri(decoded.data()));
        material.baseColourTexture = decoded;
        return material;
    }

    *embedded = texture->image;
    return material;
}

/**
 * Which reader decodes an embedded image, from the mime type the file gave it or, when it gave
 * none, from the type a data uri states.
 *
 * glTF allows only png and jpeg for an embedded image, so an empty answer is a file
 * outside the specification rather than a format worth guessing at from the bytes.
 *
 * @return the key api/image registers its readers under, or nothing
 **/
std::string readerFor(const cgltf_image& image) {
    // the mime type is optional for an image inlined as a data uri, which states its own
    std::string mime = image.mime_type != nullptr ? std::string(image.mime_type) : std::string();
    if (mime.empty() && image.uri != nullptr && std::strncmp(image.uri, "data:", 5) == 0) {
        const char* type = image.uri + 5;
        const char* end = std::strpbrk(type, ";,");
        if (end != nullptr) {
            mime.assign(type, static_cast<std::size_t>(end - type));
        }
    }
    if (mime == "image/png") {
        return "png";
    }
    if (mime == "image/jpeg") {
        return "jpg";
    }
    return std::string();
}

};  // namespace

/**
 **/
Gltf::Gltf(const boost::shared_ptr<v3d::log::Logger>& logger) :
    Loader(Type::ModelGltf, logger) {
}

/**
 **/
boost::shared_ptr<Asset> Gltf::load(std::string_view name) {
    logger_->get()->info("Looking for gltf asset at: {}", name);

    const std::string path(name);
    cgltf_options options{};
    cgltf_data* data = nullptr;

    if (cgltf_parse_file(&options, path.c_str(), &data) != cgltf_result_success) {
        logger_->get()->error("Could not parse gltf asset: {}", name);
        return boost::shared_ptr<Asset>();
    }
    // a .gltf points at its buffers and a .glb carries them; either way nothing can be read
    // out of an accessor until they are resolved
    if (cgltf_load_buffers(&options, data, path.c_str()) != cgltf_result_success) {
        logger_->get()->error("Could not load the buffers of gltf asset: {}", name);
        cgltf_free(data);
        return boost::shared_ptr<Asset>();
    }
    // cgltf reads an accessor without checking its bounds, so every accessor is checked
    // against its buffer view, and every view against its buffer, before any is read. It also
    // requires a primitive's attributes to share one count, and a sampler's output to match its
    // input. It does not compare a skin's inverse bind matrices with its joints.
    if (cgltf_validate(data) != cgltf_result_success) {
        logger_->get()->error("Gltf asset failed validation: {}", name);
        cgltf_free(data);
        return boost::shared_ptr<Asset>();
    }

    const std::vector<const cgltf_node*> starts = roots(*data);
    boost::shared_ptr<v3d::type::Model> model = boost::make_shared<v3d::type::Model>();

    // the first skin a mesh is bound to is the model's; a model has one skeleton
    Rig rig;
    for (const cgltf_node* start : starts) {
        const cgltf_skin* skin = firstSkin(*start);
        if (skin != nullptr && skin->joints_count > 0) {
            if (!bindsEveryJoint(*skin)) {
                logger_->get()->error("The skin of gltf asset {} does not give each of its joints an inverse bind matrix",
                    name);
                cgltf_free(data);
                return boost::shared_ptr<Asset>();
            }
            model->skeleton() = readSkeleton(*skin, &rig);
            break;
        }
    }

    Dropped dropped;
    if (rig.skin != nullptr) {
        model->clips() = readClips(*data, rig, &dropped);
    }

    std::vector<Bucket> buckets;
    for (const cgltf_node* start : starts) {
        collect(*start, rig, &buckets, &dropped);
    }

    std::vector<boost::shared_ptr<v3d::image::Image>> baseColours;

    for (const Bucket& bucket : buckets) {
        v3d::type::Model::Part part;
        part.firstIndex = static_cast<std::uint32_t>(model->indices().size());
        part.material = static_cast<std::uint32_t>(model->materials().size());
        for (const Placed& placed : bucket.primitives) {
            appendPrimitive(placed, rig, model.get(), &dropped);
        }
        part.indexCount = static_cast<std::uint32_t>(model->indices().size()) - part.firstIndex;
        model->parts().push_back(part);

        const cgltf_image* embedded = nullptr;
        model->materials().push_back(bucket.material != nullptr ? readMaterial(*bucket.material, &embedded)
                                                                : v3d::type::Model::Material());
        // decoded before the file is freed, because the bytes are the file's
        baseColours.push_back(embedded != nullptr ? decodeEmbedded(*embedded, name) : boost::shared_ptr<v3d::image::Image>());
    }

    cgltf_free(data);

    if (dropped.skins) {
        logger_->get()->warn("{} binds meshes to more than one skin. The first is kept, and a mesh bound to another "
            "follows a joint of it rigidly", name);
    }
    if (dropped.influences) {
        logger_->get()->warn("{} gives some vertices more than four influences, and only the first four are kept", name);
    }
    if (dropped.channels) {
        logger_->get()->warn("{} animates nodes that are not joints of its skeleton, which no clip keeps", name);
    }
    if (dropped.unweighted) {
        logger_->get()->warn("{} has skinned vertices with no weight, which follow the skeleton's first root", name);
    }
    if (dropped.lines) {
        logger_->get()->warn("{} has primitives of points or lines, which a model of triangles leaves out", name);
    }

    if (model->empty()) {
        logger_->get()->error("No geometry in gltf asset: {}", name);
        return boost::shared_ptr<Asset>();
    }

    return boost::make_shared<kind::Model>(std::string(name), Type::ModelGltf, model, baseColours);
}

/**
 **/
boost::shared_ptr<v3d::image::Image> Gltf::decodeEmbedded(const cgltf_image& image, std::string_view model) {
    boost::shared_ptr<v3d::image::Image> empty;

    const std::string kind = readerFor(image);
    if (kind.empty()) {
        logger_->get()->warn("The base colour texture of {} is embedded as {}, which is not a format glTF "
            "allows embedded, so the model has no texture", model,
            image.mime_type == nullptr ? "nothing in particular" : image.mime_type);
        return empty;
    }

    v3d::image::Factory factory(logger_);

    // a .glb keeps its images in its own buffer, which cgltf_load_buffers has already
    // resolved by the time this runs
    if (image.buffer_view != nullptr) {
        const unsigned char* bytes = cgltf_buffer_view_data(image.buffer_view);
        if (bytes == nullptr) {
            logger_->get()->error("The embedded base colour texture of {} has no bytes behind it", model);
            return empty;
        }
        return factory.read(bytes, static_cast<std::size_t>(image.buffer_view->size), kind);
    }

    // and a .gltf may inline one as a data uri instead, which nothing has decoded yet -
    // cgltf_load_buffers resolves the file's buffers and an image is not one of them
    if (image.uri == nullptr) {
        return empty;
    }
    const char* comma = std::strchr(image.uri, ',');
    if (comma == nullptr || comma - image.uri < 7 || std::strncmp(comma - 7, ";base64", 7) != 0) {
        logger_->get()->error("The base colour texture of {} is a data uri that is not base64", model);
        return empty;
    }
    // four base64 characters carry three bytes, less however many the padding stands in for
    const std::size_t encoded = std::strlen(comma + 1);
    if (encoded == 0 || encoded % 4 != 0) {
        logger_->get()->error("The base colour texture of {} is a data uri of the wrong length", model);
        return empty;
    }
    std::size_t decoded = encoded / 4 * 3;
    if (comma[encoded] == '=') {
        decoded--;
    }
    if (comma[encoded - 1] == '=') {
        decoded--;
    }

    cgltf_options options{};
    void* bytes = nullptr;
    if (cgltf_load_buffer_base64(&options, decoded, comma + 1, &bytes) != cgltf_result_success || bytes == nullptr) {
        logger_->get()->error("The base colour texture of {} could not be decoded out of its data uri", model);
        return empty;
    }
    boost::shared_ptr<v3d::image::Image> result =
        factory.read(static_cast<const unsigned char*>(bytes), decoded, kind);
    // allocated by cgltf's default allocator, which is malloc
    free(bytes);
    return result;
}

};  // namespace v3d::asset::media::loader
