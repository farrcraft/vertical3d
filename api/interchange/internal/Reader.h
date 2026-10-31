/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/interchange/Scene.h>
#include <api/type/Transform.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// cgltf.h is a C header, and cpplint sorts it with the C system headers rather than with
// the third party ones.
#include <cgltf.h>  // NOLINT(build/include_order)

namespace v3d::interchange::internal {

/**
 * Fills a scene from a file cgltf has parsed, loaded and validated.
 **/
class Reader final {
 public:
    explicit Reader(const cgltf_data& data);

    Imported read();

 private:
    /**
     * glTF gives a point or spot light in candela and a distant one in lux. Dividing by the
     * luminous efficacy of light at 555 nanometres gives watts per steradian or per square metre.
     **/
    static constexpr float LUMENS_PER_WATT = 683.0F;

    static const cgltf_accessor* positionsOf(const cgltf_primitive& primitive);

    static std::string nameOf(const char* name);

    /**
     * A node's placement, from its matrix when it has one.
     **/
    static v3d::type::Transform placementOf(const cgltf_node& node);

    /**
     * The triangles of one primitive as indices into its own points, counter-clockwise as glTF
     * winds them. A strip turns every other triangle round, and a fan shares its first point.
     **/
    static std::vector<std::uint32_t> triangleIndices(const cgltf_primitive& primitive, cgltf_size count);

    /**
     * The nodes the walk starts from: the scene the file names, or its first, or every root
     * when it has none.
     **/
    std::vector<const cgltf_node*> roots() const;

    /**
     * Add a node and everything beneath it, parents first. A node met a second time, which only
     * a malformed file can do, is not added again.
     **/
    void addTree(const cgltf_node& root);

    std::size_t addNode(const cgltf_node& source, std::optional<std::size_t> parent);

    std::size_t mesh(const cgltf_mesh& source);

    void addPrimitive(const cgltf_primitive& primitive, Mesh* mesh);

    std::size_t camera(const cgltf_camera& source);

    std::size_t light(const cgltf_light& source);

    void addAnimation(const cgltf_animation& animation);

    /**
     * A cubic spline sampler stores three values a key: the tangent in, the value and the
     * tangent out.
     **/
    static void readKeys(const cgltf_animation_sampler& sampler, Channel* channel);

    const cgltf_data& data_;
    Scene scene_;
    std::vector<std::string> notes_;
    std::unordered_map<const cgltf_node*, std::size_t> nodes_;
    std::unordered_map<const cgltf_mesh*, std::size_t> meshes_;
    std::unordered_map<const cgltf_camera*, std::size_t> cameras_;
    std::unordered_map<const cgltf_light*, std::size_t> lights_;
    std::set<std::pair<std::size_t, Channel::Path>> keyed_;
};

};  // namespace v3d::interchange::internal
