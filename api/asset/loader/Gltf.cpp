/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Gltf.h"

#include <cstring>
#include <numeric>
#include <string>

#include "../Model.h"
#include "../Type.h"

// cgltf.h is a C header, and cpplint sorts it with the C system headers rather than with
// the third party ones. Its implementation half is compiled once, in CgltfImpl.cpp.
#include <cgltf.h>  // NOLINT(build/include_order)

#include <boost/make_shared.hpp>

namespace v3d::asset::loader {

namespace {

/**
 * The accessors one primitive draws from. Position is the only one a primitive has to have:
 * without it there is no geometry, and the other two default to zero.
 **/
struct Attributes final {
    const cgltf_accessor* position{ nullptr };
    const cgltf_accessor* normal{ nullptr };
    const cgltf_accessor* uv{ nullptr };
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
            default:
                break;
        }
    }
    return found;
}

/**
 * Append one primitive's vertices to the merged array, rebasing its indices onto it.
 **/
void appendPrimitive(const cgltf_primitive& primitive, const Attributes& attributes, v3d::type::Model* model) {
    const std::size_t baseVertex = model->vertices().size();
    const cgltf_size count = attributes.position->count;
    model->vertices().resize(baseVertex + count);

    for (cgltf_size index = 0; index < count; ++index) {
        v3d::type::Model::Vertex& vertex = model->vertices()[baseVertex + index];
        cgltf_accessor_read_float(attributes.position, index, &vertex.position.x, 3);
        if (attributes.normal != nullptr) {
            cgltf_accessor_read_float(attributes.normal, index, &vertex.normal.x, 3);
        }
        if (attributes.uv != nullptr) {
            cgltf_accessor_read_float(attributes.uv, index, &vertex.uv.x, 2);
        }
    }

    const std::size_t baseIndex = model->indices().size();
    if (primitive.indices != nullptr) {
        const cgltf_size indices = primitive.indices->count;
        model->indices().resize(baseIndex + indices);
        for (cgltf_size index = 0; index < indices; ++index) {
            // a primitive's indices address its own vertices, so every one after the first
            // primitive addresses the wrong geometry unless it is offset into the merge
            model->indices()[baseIndex + index] =
                static_cast<std::uint32_t>(cgltf_accessor_read_index(primitive.indices, index) + baseVertex);
        }
        return;
    }

    // a primitive drawn straight out of its vertex array still has to be indexed once it
    // is merged, or its geometry is lost rather than drawn
    model->indices().resize(baseIndex + count);
    std::iota(model->indices().begin() + static_cast<std::ptrdiff_t>(baseIndex), model->indices().end(),
        static_cast<std::uint32_t>(baseVertex));
}

/**
 * The base colour and the name of the image tinting it.
 *
 * @param embedded set when the material names a texture the file carries rather than one it
 *        names, which is reported by the caller rather than dropped
 **/
v3d::type::Model::Material readMaterial(const cgltf_material& source, bool* embedded) {
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
        material.baseColourTexture = texture->image->uri;
        return material;
    }

    *embedded = true;
    return material;
}

};  // namespace

/**
 **/
Gltf::Gltf(Manager* manager, const boost::shared_ptr<v3d::log::Logger>& logger) :
    Loader(manager, Type::ModelGltf, logger) {
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

    boost::shared_ptr<v3d::type::Model> model = boost::make_shared<v3d::type::Model>();
    bool haveMaterial = false;
    bool embedded = false;

    for (cgltf_size meshIndex = 0; meshIndex < data->meshes_count; ++meshIndex) {
        const cgltf_mesh& mesh = data->meshes[meshIndex];
        for (cgltf_size primitiveIndex = 0; primitiveIndex < mesh.primitives_count; ++primitiveIndex) {
            const cgltf_primitive& primitive = mesh.primitives[primitiveIndex];
            const Attributes attributes = attributesOf(primitive);
            if (attributes.position == nullptr) {
                continue;
            }

            appendPrimitive(primitive, attributes, model.get());

            if (!haveMaterial && primitive.material != nullptr) {
                model->material() = readMaterial(*primitive.material, &embedded);
                haveMaterial = true;
            }
        }
    }

    cgltf_free(data);

    if (embedded) {
        logger_->get()->warn("The base colour texture of {} is embedded in the file and was not read - "
            "an image reader that takes a buffer is what that needs", name);
    }
    if (model->empty()) {
        logger_->get()->error("No geometry in gltf asset: {}", name);
        return boost::shared_ptr<Asset>();
    }

    return boost::make_shared<Model>(std::string(name), Type::ModelGltf, model);
}

};  // namespace v3d::asset::loader
