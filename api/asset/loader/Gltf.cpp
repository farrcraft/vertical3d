/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Gltf.h"

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <numeric>
#include <string>

#include "../Model.h"
#include "../Type.h"
#include "../../image/Factory.h"

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
        material.baseColourTexture = texture->image->uri;
        return material;
    }

    *embedded = texture->image;
    return material;
}

/**
 * Which reader decodes an embedded image, from the mime type the file gave it.
 *
 * glTF allows only png and jpeg for an embedded image, so an empty answer is a file
 * outside the specification rather than a format worth guessing at from the bytes.
 *
 * @return the key api/image registers its readers under, or nothing
 **/
std::string readerFor(const cgltf_image& image) {
    if (image.mime_type == nullptr) {
        return std::string();
    }
    const std::string mime(image.mime_type);
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
    const cgltf_image* embedded = nullptr;

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

    // decoded before the file is freed, because the bytes are the file's
    boost::shared_ptr<v3d::image::Image> baseColour;
    if (embedded != nullptr) {
        baseColour = decodeEmbedded(*embedded, name);
    }

    cgltf_free(data);

    if (model->empty()) {
        logger_->get()->error("No geometry in gltf asset: {}", name);
        return boost::shared_ptr<Asset>();
    }

    return boost::make_shared<Model>(std::string(name), Type::ModelGltf, model, baseColour);
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

};  // namespace v3d::asset::loader
