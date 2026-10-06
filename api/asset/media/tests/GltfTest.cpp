/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/Manager.h>
#include <api/asset/Type.h>
#include <api/asset/media/Loaders.h>
#include <api/asset/media/kind/Model.h>
#include <api/asset/media/loader/Gltf.h>
#include <api/image/Compare.h>
#include <api/image/Factory.h>
#include <api/type/animation/Channel.h>
#include <api/type/animation/Clip.h>
#include <api/type/animation/Pose.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace {

// three primitives, the third of them not indexed and all three sharing one material -
// api/asset/tests/data/make_model_fixture.py generates it and says why it is shaped that way
const char* FIXTURE = "three_primitives.glb";

// one triangle whose base colour texture is a bufferView holding pixel.png, which is the
// case the named-texture fixture cannot cover - api/asset/tests/data/make_embedded_fixture.py
const char* EMBEDDED = "embedded_texture.glb";

// two meshes under a turned parent, with three primitives over two materials -
// api/asset/tests/data/make_parts_fixture.py, which also writes the same file with no scene
const char* PARTS = "two_surfaces.glb";
const char* SCENELESS = "two_surfaces_no_scene.glb";

// a strip bound to a chain of three joints, and the same skin listed child first -
// api/asset/tests/data/make_skin_fixture.py
const char* SKINNED = "bending_strip.glb";
const char* SHUFFLED = "bending_strip_shuffled.glb";

// the strip built and exported by Blender - api/asset/tests/data/make_blender_fixture.py
const char* BLENDER = "blender_strip.glb";

/**
 * The strip's skeleton, which both skin fixtures share once read: root, middle and top in a
 * chain a unit apart, with an armature standing the given distance along z above them.
 **/
void checkSkeleton(const v3d::type::Skeleton& skeleton, float armature) {
    BOOST_REQUIRE_EQUAL(skeleton.joints.size(), 3u);
    const char* names[3] = {"root", "middle", "top"};
    for (std::size_t joint = 0; joint < 3; ++joint) {
        const v3d::type::Skeleton::Joint& read = skeleton.joints[joint];
        BOOST_CHECK_EQUAL(read.name, names[joint]);
        BOOST_CHECK_EQUAL(read.parent, static_cast<std::int32_t>(joint) - 1);
        BOOST_CHECK(read.translation == glm::vec3(0.0f, joint == 0 ? 0.0f : 1.0f, 0.0f));
        BOOST_CHECK(read.rotation == glm::identity<glm::quat>());
        BOOST_CHECK(read.scale == glm::vec3(1.0f));
        const glm::vec3 bind(0.0f, static_cast<float>(joint), armature);
        BOOST_CHECK(read.inverseBind == glm::translate(glm::mat4(1.0f), -bind));
    }
}

/**
 * Which joints the strip's rows follow, in the skeleton's order: root below y = 1, half and
 * half at it, middle between, half and half at 2, and top above. Two vertices a row.
 **/
void checkInfluences(const v3d::type::Model& model) {
    const std::vector<v3d::type::Model::Influence>& influences = model.influences();
    BOOST_CHECK_EQUAL(influences[0].joints[0], 0u);
    BOOST_CHECK_EQUAL(influences[4].joints[0], 0u);
    BOOST_CHECK_EQUAL(influences[4].joints[1], 1u);
    BOOST_CHECK_EQUAL(influences[6].joints[0], 1u);
    BOOST_CHECK_EQUAL(influences[8].joints[0], 1u);
    BOOST_CHECK_EQUAL(influences[8].joints[1], 2u);
    BOOST_CHECK_EQUAL(influences[13].joints[0], 2u);
}

/**
 * Both parts, their ranges and their materials, which the two parts fixtures share.
 **/
void checkParts(const v3d::type::Model& model) {
    // where the vertices land: each primitive's triangle moved by its node and then turned a
    // quarter about +z by the parent, in the order the parts hold them - A and C in material
    // 0's part, then B in material 1's. Whole numbers throughout, so exact
    const glm::vec3 placed[9] = {
        {0, 2, 0}, {0, 3, 0}, {-1, 2, 0},    // A, under node 1's (2, 0, 0)
        {-3, 0, 2}, {-3, 1, 2}, {-4, 0, 2},  // C, under node 2's (0, 3, 0)
        {-3, 0, 1}, {-3, 1, 1}, {-4, 0, 1},  // B, under node 2 as well
    };

    BOOST_REQUIRE_EQUAL(model.vertices().size(), 9u);
    BOOST_REQUIRE_EQUAL(model.indices().size(), 9u);
    BOOST_REQUIRE_EQUAL(model.parts().size(), 2u);

    BOOST_CHECK_EQUAL(model.parts()[0].firstIndex, 0u);
    BOOST_CHECK_EQUAL(model.parts()[0].indexCount, 6u);
    BOOST_CHECK_EQUAL(model.parts()[0].material, 0u);
    BOOST_CHECK_EQUAL(model.parts()[1].firstIndex, 6u);
    BOOST_CHECK_EQUAL(model.parts()[1].indexCount, 3u);
    BOOST_CHECK_EQUAL(model.parts()[1].material, 1u);

    for (std::size_t index = 0; index < 9; ++index) {
        BOOST_CHECK_MESSAGE(model.vertices()[index].position == placed[index],
            "vertex " << index << " is at " << model.vertices()[index].position.x << ", "
                      << model.vertices()[index].position.y << ", " << model.vertices()[index].position.z);
    }
}

boost::shared_ptr<v3d::log::Logger> logger() {
    return boost::make_shared<v3d::log::Logger>();
}

boost::shared_ptr<v3d::asset::media::kind::Model> loadAsset(const char* name) {
    v3d::asset::Manager manager("data", logger());
    v3d::asset::media::registerLoaders(manager, logger());
    boost::shared_ptr<v3d::asset::Asset> asset = manager.load(name, v3d::asset::Type::ModelGltf);
    return boost::dynamic_pointer_cast<v3d::asset::media::kind::Model>(asset);
}

boost::shared_ptr<v3d::type::Model> load(const char* name) {
    v3d::asset::Manager manager("data", logger());
    v3d::asset::media::registerLoaders(manager, logger());
    boost::shared_ptr<v3d::asset::Asset> asset = manager.load(name, v3d::asset::Type::ModelGltf);
    if (!asset) {
        return boost::shared_ptr<v3d::type::Model>();
    }
    boost::shared_ptr<v3d::asset::media::kind::Model> model = boost::dynamic_pointer_cast<v3d::asset::media::kind::Model>(asset);
    return model ? model->model() : boost::shared_ptr<v3d::type::Model>();
}

};  // namespace

BOOST_AUTO_TEST_CASE(gltf_merges_every_primitive_test) {
    boost::shared_ptr<v3d::type::Model> model = load(FIXTURE);

    BOOST_REQUIRE(model);
    // three triangles of three vertices, in one array and one index run
    BOOST_CHECK_EQUAL(model->vertices().size(), 9u);
    BOOST_CHECK_EQUAL(model->indices().size(), 9u);
}

BOOST_AUTO_TEST_CASE(gltf_rebases_the_indices_of_later_primitives_test) {
    boost::shared_ptr<v3d::type::Model> model = load(FIXTURE);
    BOOST_REQUIRE(model);

    // the first two primitives both index 0, 1, 2 in the file. Without the rebase the
    // second triangle would redraw the first one's vertices and the highest index would
    // be 2 rather than 8
    const std::vector<std::uint32_t> expected{ 0, 1, 2, 3, 4, 5, 6, 7, 8 };
    BOOST_CHECK_EQUAL(model->indices() == expected, true);
}

BOOST_AUTO_TEST_CASE(gltf_synthesises_indices_for_a_primitive_without_them_test) {
    boost::shared_ptr<v3d::type::Model> model = load(FIXTURE);
    BOOST_REQUIRE(model);

    // the third primitive is drawn straight out of its vertex array in the file. Its three
    // vertices are in the merge, and so is an index run reaching them - dropping either
    // would leave six of each
    BOOST_CHECK_EQUAL(model->vertices().size(), 9u);
    BOOST_CHECK_EQUAL(model->indices()[6], 6u);
    BOOST_CHECK_EQUAL(model->indices()[8], 8u);
}

BOOST_AUTO_TEST_CASE(gltf_every_index_addresses_a_real_vertex_test) {
    boost::shared_ptr<v3d::type::Model> model = load(FIXTURE);
    BOOST_REQUIRE(model);

    for (const std::uint32_t index : model->indices()) {
        BOOST_REQUIRE(index < model->vertices().size());
    }
}

BOOST_AUTO_TEST_CASE(gltf_reads_the_vertex_attributes_test) {
    boost::shared_ptr<v3d::type::Model> model = load(FIXTURE);
    BOOST_REQUIRE(model);

    // the three triangles are stepped along x, so the position says which primitive a
    // vertex came from and in what order they were appended
    BOOST_CHECK_CLOSE(model->vertices()[0].position.x, 0.0f, 0.01f);
    BOOST_CHECK_CLOSE(model->vertices()[3].position.x, 2.0f, 0.01f);
    BOOST_CHECK_CLOSE(model->vertices()[6].position.x, 4.0f, 0.01f);

    // every fixture normal points down +z, and the uvs are the same three per triangle
    BOOST_CHECK_CLOSE(model->vertices()[0].normal.z, 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(model->vertices()[8].normal.z, 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(model->vertices()[1].uv.x, 1.0f, 0.01f);
    BOOST_CHECK_CLOSE(model->vertices()[2].uv.y, 1.0f, 0.01f);
}

BOOST_AUTO_TEST_CASE(gltf_reads_the_base_colour_test) {
    boost::shared_ptr<v3d::type::Model> model = load(FIXTURE);
    BOOST_REQUIRE(model);
    BOOST_REQUIRE_EQUAL(model->materials().size(), 1u);

    BOOST_CHECK_CLOSE(model->materials()[0].baseColour.r, 0.25f, 0.01f);
    BOOST_CHECK_CLOSE(model->materials()[0].baseColour.g, 0.5f, 0.01f);
    BOOST_CHECK_CLOSE(model->materials()[0].baseColour.b, 0.75f, 0.01f);
    BOOST_CHECK_CLOSE(model->materials()[0].baseColour.a, 1.0f, 0.01f);
}

BOOST_AUTO_TEST_CASE(gltf_names_the_texture_rather_than_decoding_it_test) {
    boost::shared_ptr<v3d::type::Model> model = load(FIXTURE);
    BOOST_REQUIRE(model);

    // the material names its image and the app resolves it. The fixture ships no
    // albedo.png at all, and loading still succeeds
    BOOST_CHECK_EQUAL(model->materials()[0].baseColourTexture, "albedo.png");
}

/**
 * An image the file carries has no name to hand over, so it arrives decoded - and decoded
 * to exactly what the png reader makes of the same bytes on disk.
 **/
BOOST_AUTO_TEST_CASE(gltf_decodes_a_texture_the_file_carries_test) {
    boost::shared_ptr<v3d::asset::media::kind::Model> asset = loadAsset(EMBEDDED);
    BOOST_REQUIRE(asset);
    BOOST_REQUIRE(asset->model());

    // there is no name, because there is no file to name
    BOOST_CHECK(asset->model()->materials()[0].baseColourTexture.empty());

    boost::shared_ptr<v3d::image::Image> embedded = asset->baseColourImage(0);
    BOOST_REQUIRE(embedded);

    v3d::image::Factory factory(logger());
    boost::shared_ptr<v3d::image::Image> onDisk = factory.read("data/pixel.png");
    BOOST_REQUIRE(onDisk);

    const v3d::image::Difference difference = v3d::image::compare(*embedded, *onDisk, 0);
    BOOST_CHECK_MESSAGE(difference.match, difference.description());
}

/**
 * A model whose texture was named carries no pixels, so an app can tell the two apart by
 * checking rather than by knowing which packaging it loaded.
 **/
BOOST_AUTO_TEST_CASE(gltf_a_named_texture_carries_no_pixels_test) {
    boost::shared_ptr<v3d::asset::media::kind::Model> asset = loadAsset(FIXTURE);
    BOOST_REQUIRE(asset);

    BOOST_CHECK(!asset->baseColourImage(0));
}

BOOST_AUTO_TEST_CASE(gltf_a_missing_file_is_no_asset_test) {
    // a loader that failed hands back nothing rather than an asset holding nothing, which
    // is indistinguishable from a loaded one until a consumer dereferences it
    BOOST_CHECK_EQUAL(static_cast<bool>(load("does_not_exist.glb")), false);
}

BOOST_AUTO_TEST_CASE(gltf_a_file_that_is_not_gltf_is_no_asset_test) {
    BOOST_CHECK_EQUAL(static_cast<bool>(load("plain.txt")), false);
}

BOOST_AUTO_TEST_CASE(gltf_resolves_by_extension_test) {
    v3d::asset::Manager manager("data", logger());
    v3d::asset::media::registerLoaders(manager, logger());

    // .glb and .gltf both reach the model loader, so an app naming a file gets one without
    // naming the type
    boost::shared_ptr<v3d::asset::Asset> asset = manager.loadTypeFromExt(FIXTURE);
    BOOST_CHECK_EQUAL(static_cast<bool>(boost::dynamic_pointer_cast<v3d::asset::media::kind::Model>(asset)), true);
}

BOOST_AUTO_TEST_CASE(gltf_vertex_bytes_is_the_array_size_test) {
    boost::shared_ptr<v3d::type::Model> model = load(FIXTURE);
    BOOST_REQUIRE(model);

    // what a device buffer is made from: eight floats a vertex, and the loader fills them
    // all whether or not the file carried normals and uvs
    BOOST_CHECK_EQUAL(model->vertexBytes(), model->vertices().size() * 8u * sizeof(float));
    BOOST_CHECK_EQUAL(model->empty(), false);
}

/**
 * A file with several surfaces is one model in parts, one part per material, holding every
 * primitive drawn with it.
 **/
BOOST_AUTO_TEST_CASE(gltf_a_part_per_material_test) {
    boost::shared_ptr<v3d::type::Model> model = load(PARTS);
    BOOST_REQUIRE(model);
    checkParts(*model);

    BOOST_REQUIRE_EQUAL(model->materials().size(), 2u);
    BOOST_CHECK(model->materials()[0].baseColour == glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
    BOOST_CHECK_EQUAL(model->materials()[0].baseColourTexture, "red.png");
    BOOST_CHECK(model->materials()[1].baseColour == glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
    BOOST_CHECK(model->materials()[1].baseColourTexture.empty());
}

/**
 * Each index addresses its own part's vertices: C's run was synthesised, and B's file indices
 * are 0, 1, 2 like A's.
 **/
BOOST_AUTO_TEST_CASE(gltf_a_part_indexes_its_own_vertices_test) {
    boost::shared_ptr<v3d::type::Model> model = load(PARTS);
    BOOST_REQUIRE(model);

    const std::vector<std::uint32_t> expected{ 0, 1, 2, 3, 4, 5, 6, 7, 8 };
    BOOST_CHECK_EQUAL(model->indices() == expected, true);
}

/**
 * Normals are turned with the node that turns the positions. Every normal in the file is +x,
 * and a quarter turn about +z makes it +y.
 **/
BOOST_AUTO_TEST_CASE(gltf_a_node_turns_the_normals_test) {
    boost::shared_ptr<v3d::type::Model> model = load(PARTS);
    BOOST_REQUIRE(model);

    for (const v3d::type::Model::Vertex& vertex : model->vertices()) {
        BOOST_CHECK_SMALL(vertex.normal.x, 1e-6f);
        BOOST_CHECK_CLOSE(vertex.normal.y, 1.0f, 1e-4f);
        BOOST_CHECK_SMALL(vertex.normal.z, 1e-6f);
    }
}

/**
 * A file that names no scene is read from every root node, and comes out the same.
 **/
BOOST_AUTO_TEST_CASE(gltf_a_file_with_no_scene_reads_its_roots_test) {
    boost::shared_ptr<v3d::type::Model> model = load(SCENELESS);
    BOOST_REQUIRE(model);
    checkParts(*model);
}

/**
 * A file of one material is one part covering every index.
 **/
BOOST_AUTO_TEST_CASE(gltf_one_material_is_one_part_test) {
    boost::shared_ptr<v3d::type::Model> model = load(FIXTURE);
    BOOST_REQUIRE(model);

    BOOST_REQUIRE_EQUAL(model->parts().size(), 1u);
    BOOST_CHECK_EQUAL(model->parts()[0].firstIndex, 0u);
    BOOST_CHECK_EQUAL(model->parts()[0].indexCount, 9u);
    BOOST_CHECK_EQUAL(model->parts()[0].material, 0u);
}

/**
 * A model with no skin has no skeleton and no influences.
 **/
BOOST_AUTO_TEST_CASE(gltf_a_static_model_has_no_skeleton_test) {
    boost::shared_ptr<v3d::type::Model> model = load(FIXTURE);
    BOOST_REQUIRE(model);

    BOOST_CHECK(model->skeleton().empty());
    BOOST_CHECK(model->influences().empty());
}

/**
 * The skin's joints, their parents, rest poses and inverse bind matrices - every one a whole
 * number, so compared exactly.
 **/
BOOST_AUTO_TEST_CASE(gltf_reads_a_skeleton_test) {
    boost::shared_ptr<v3d::type::Model> model = load(SKINNED);
    BOOST_REQUIRE(model);
    checkSkeleton(model->skeleton(), 0.0f);
    BOOST_CHECK(model->skeleton().root == glm::mat4(1.0f));
}

/**
 * A vertex's joints and weights: wholly one joint's below and above the joins, half and half
 * on them.
 **/
BOOST_AUTO_TEST_CASE(gltf_reads_the_influences_test) {
    boost::shared_ptr<v3d::type::Model> model = load(SKINNED);
    BOOST_REQUIRE(model);
    BOOST_REQUIRE_EQUAL(model->influences().size(), model->vertices().size());
    checkInfluences(*model);

    const std::vector<v3d::type::Model::Influence>& influences = model->influences();
    BOOST_CHECK(influences[0].weights == glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));
    BOOST_CHECK(influences[4].weights == glm::vec4(0.5f, 0.5f, 0.0f, 0.0f));
    BOOST_CHECK(influences[13].weights == glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));
}

/**
 * A skinned mesh is placed by its joints, not its node: the strip's node is moved seven along
 * z, and the strip is read where its vertices say.
 **/
BOOST_AUTO_TEST_CASE(gltf_a_skinned_mesh_is_not_placed_by_its_node_test) {
    boost::shared_ptr<v3d::type::Model> model = load(SKINNED);
    BOOST_REQUIRE(model);
    BOOST_REQUIRE(!model->parts().empty());

    BOOST_CHECK(model->vertices()[0].position == glm::vec3(-1.0f, 0.0f, 0.0f));
    BOOST_CHECK(model->vertices()[13].position == glm::vec3(1.0f, 3.0f, 0.0f));
}

/**
 * An unskinned mesh in a skinned model follows the joint above it, wholly, and stands where
 * its node puts it while that joint is at rest - half a unit above top, which is at y = 2.
 **/
BOOST_AUTO_TEST_CASE(gltf_an_unskinned_mesh_follows_a_joint_test) {
    boost::shared_ptr<v3d::type::Model> model = load(SKINNED);
    BOOST_REQUIRE(model);
    BOOST_REQUIRE_EQUAL(model->parts().size(), 2u);

    const v3d::type::Model::Part& tip = model->parts()[1];
    const std::uint32_t first = model->indices()[tip.firstIndex];
    BOOST_CHECK(model->vertices()[first].position == glm::vec3(0.0f, 2.5f, 0.0f));
    BOOST_CHECK_EQUAL(model->influences()[first].joints[0], 2u);
    BOOST_CHECK(model->influences()[first].weights == glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));
}

/**
 * A skin listed child first is read parent first, with every vertex's joints remapped into
 * that order, and whatever stands above the skeleton kept as its root. Its weights are bytes,
 * which read normalised and sum to one.
 **/
BOOST_AUTO_TEST_CASE(gltf_a_skeleton_listed_child_first_is_reordered_test) {
    boost::shared_ptr<v3d::type::Model> model = load(SHUFFLED);
    BOOST_REQUIRE(model);
    checkSkeleton(model->skeleton(), 4.0f);
    BOOST_CHECK(model->skeleton().root == glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 4.0f)));

    BOOST_REQUIRE_EQUAL(model->influences().size(), model->vertices().size());
    checkInfluences(*model);
    for (const v3d::type::Model::Influence& influence : model->influences()) {
        BOOST_CHECK_CLOSE(influence.weights.x + influence.weights.y + influence.weights.z + influence.weights.w, 1.0f, 1e-4f);
    }
    BOOST_CHECK_CLOSE(model->influences()[4].weights.x, 0.5f, 0.5f);
    BOOST_CHECK_CLOSE(model->influences()[4].weights.y, 0.5f, 0.5f);
}

/**
 * Every animation is a clip, named as the file named it, keeping the channels on joints - the
 * bend also moves the tip, which is not a joint, and that channel is dropped.
 **/
BOOST_AUTO_TEST_CASE(gltf_reads_the_clips_test) {
    using v3d::type::animation::Channel;
    boost::shared_ptr<v3d::type::Model> model = load(SKINNED);
    BOOST_REQUIRE(model);
    BOOST_REQUIRE_EQUAL(model->clips().size(), 3u);

    const v3d::type::animation::Clip& bend = model->clips()[0];
    BOOST_CHECK_EQUAL(bend.name, "bend");
    BOOST_CHECK_EQUAL(bend.duration, 1.0f);
    BOOST_REQUIRE_EQUAL(bend.channels.size(), 1u);
    BOOST_CHECK_EQUAL(bend.channels[0].joint, 1u);
    BOOST_CHECK(bend.channels[0].path == Channel::Path::Rotation);
    BOOST_CHECK(bend.channels[0].interpolation == Channel::Interpolation::Linear);

    BOOST_CHECK_EQUAL(model->clips()[1].name, "step");
    BOOST_REQUIRE_EQUAL(model->clips()[1].channels.size(), 1u);
    BOOST_CHECK(model->clips()[1].channels[0].interpolation == Channel::Interpolation::Step);

    const v3d::type::animation::Clip& cubic = model->clips()[2];
    BOOST_CHECK_EQUAL(cubic.name, "cubic");
    BOOST_REQUIRE_EQUAL(cubic.channels.size(), 1u);
    BOOST_CHECK_EQUAL(cubic.channels[0].joint, 2u);
    BOOST_CHECK(cubic.channels[0].path == Channel::Path::Translation);
    BOOST_CHECK(cubic.channels[0].interpolation == Channel::Interpolation::CubicSpline);
    // an in tangent, a value and an out tangent for each of two keys
    BOOST_CHECK_EQUAL(cubic.channels[0].values.size(), 6u);
}

/**
 * The bend read from the file, sampled at its end, swings the top joint to (-1, 1, 0) - the
 * loader and the sampler agreeing about the file end to end.
 **/
BOOST_AUTO_TEST_CASE(gltf_a_clip_read_bends_the_skeleton_test) {
    boost::shared_ptr<v3d::type::Model> model = load(SKINNED);
    BOOST_REQUIRE(model);
    BOOST_REQUIRE(!model->clips().empty());

    v3d::type::animation::Pose pose = v3d::type::animation::rest(model->skeleton());
    v3d::type::animation::sample(model->clips()[0], 1.0f, &pose);
    std::vector<glm::mat4> palette;
    v3d::type::animation::palette(model->skeleton(), pose, &palette);
    BOOST_REQUIRE_EQUAL(palette.size(), 3u);

    // the top joint's bind position, carried by its skinning matrix to where it stands now
    const glm::vec4 top = palette[2] * glm::vec4(0.0f, 2.0f, 0.0f, 1.0f);
    BOOST_CHECK_SMALL(top.x + 1.0f, 1e-5f);
    BOOST_CHECK_SMALL(top.y - 1.0f, 1e-5f);
    BOOST_CHECK_SMALL(top.z, 1e-5f);
}

/**
 * A model with no skeleton has no clips, and neither has a skin with no animations.
 **/
BOOST_AUTO_TEST_CASE(gltf_no_animations_are_no_clips_test) {
    BOOST_CHECK(load(FIXTURE)->clips().empty());
    BOOST_CHECK(load(SHUFFLED)->clips().empty());
}

/**
 * The strip again, built and exported by Blender 5.2 by make_blender_fixture.py, so that the
 * loader meets what a real exporter writes. That is an armature node above the root joint
 * holding a Mixamo-style hundredth scale, joints a hundred units apart, and actions sampled a
 * key a frame with a channel on every joint.
 **/
BOOST_AUTO_TEST_CASE(gltf_reads_a_blender_rig_test) {
    boost::shared_ptr<v3d::type::Model> model = load(BLENDER);
    BOOST_REQUIRE(model);

    const v3d::type::Skeleton& skeleton = model->skeleton();
    BOOST_REQUIRE_EQUAL(skeleton.joints.size(), 3u);
    const char* names[3] = {"root", "middle", "top"};
    for (std::size_t joint = 0; joint < 3; ++joint) {
        BOOST_CHECK_EQUAL(skeleton.joints[joint].name, names[joint]);
        BOOST_CHECK_EQUAL(skeleton.joints[joint].parent, static_cast<std::int32_t>(joint) - 1);
    }
    // the armature's scale is above the skeleton, and the joints are in its unscaled units
    BOOST_CHECK_CLOSE(skeleton.root[0][0], 0.01f, 1e-3f);
    BOOST_CHECK_CLOSE(skeleton.joints[1].translation.y, 100.0f, 1e-3f);

    // the exporter's inverse bind matrices agree with its rest pose and its armature: at rest,
    // every joint's skinning matrix is the identity, to rounding
    std::vector<glm::mat4> palette;
    v3d::type::animation::palette(skeleton, v3d::type::animation::rest(skeleton), &palette);
    for (const glm::mat4& matrix : palette) {
        for (int column = 0; column < 4; ++column) {
            for (int row = 0; row < 4; ++row) {
                BOOST_CHECK_SMALL(matrix[column][row] - (column == row ? 1.0f : 0.0f), 1e-4f);
            }
        }
    }

    BOOST_REQUIRE_EQUAL(model->influences().size(), model->vertices().size());
    BOOST_REQUIRE_EQUAL(model->clips().size(), 2u);
    BOOST_CHECK_EQUAL(model->clips()[0].name, "bend");
    BOOST_CHECK_EQUAL(model->clips()[1].name, "sway");
    // Blender keys from frame 1, so twenty four frames at 24 a second end at 25/24
    BOOST_CHECK_CLOSE(model->clips()[0].duration, 25.0f / 24.0f, 1e-3f);
}

/**
 * Blender's bend at its end has turned middle a quarter about its own x, which the exporter
 * makes glTF's x: top, a unit above middle in the finished skeleton, swings from (0, 2, 0) to
 * (0, 1, 1).
 **/
BOOST_AUTO_TEST_CASE(gltf_a_blender_clip_bends_the_rig_test) {
    boost::shared_ptr<v3d::type::Model> model = load(BLENDER);
    BOOST_REQUIRE(model);
    BOOST_REQUIRE(!model->clips().empty());

    v3d::type::animation::Pose pose = v3d::type::animation::rest(model->skeleton());
    v3d::type::animation::sample(model->clips()[0], model->clips()[0].duration, &pose);
    std::vector<glm::mat4> palette;
    v3d::type::animation::palette(model->skeleton(), pose, &palette);
    BOOST_REQUIRE_EQUAL(palette.size(), 3u);

    const glm::vec4 top = palette[2] * glm::vec4(0.0f, 2.0f, 0.0f, 1.0f);
    BOOST_TEST_MESSAGE("top is at " << top.x << ", " << top.y << ", " << top.z);
    BOOST_CHECK_SMALL(top.x, 1e-3f);
    BOOST_CHECK_SMALL(top.y - 1.0f, 1e-3f);
    BOOST_CHECK_SMALL(top.z - 1.0f, 1e-3f);
}

/**
 * A triangle strip and a triangle fan become triangle lists, wound as glTF winds them, and a
 * primitive of lines or points is left out. Every triangle of the fixture faces +z, so a strip
 * that did not swap every other triangle shows up as one facing the other way.
 * api/asset/tests/data/make_modes_fixture.py generates it.
 **/
BOOST_AUTO_TEST_CASE(gltf_strips_and_fans_become_lists_test) {
    boost::shared_ptr<v3d::type::Model> model = load("primitive_modes.gltf");
    BOOST_REQUIRE(model);

    BOOST_REQUIRE_EQUAL(model->indices().size(), 12u);
    BOOST_CHECK_EQUAL(model->vertices().size(), 8u);
    for (std::size_t first = 0; first < model->indices().size(); first += 3) {
        const glm::vec3 a = model->vertices()[model->indices()[first]].position;
        const glm::vec3 b = model->vertices()[model->indices()[first + 1]].position;
        const glm::vec3 c = model->vertices()[model->indices()[first + 2]].position;
        BOOST_TEST_CONTEXT("triangle " << first / 3) {
            BOOST_CHECK_GT(glm::cross(b - a, c - a).z, 0.0f);
        }
    }

    // a uri is percent-encoded, and the name handed over is the file's own
    BOOST_CHECK_EQUAL(model->materials()[0].baseColourTexture, "my texture.png");
}

/**
 * An image inlined as a data uri needs no mimeType, because the uri states its type.
 **/
BOOST_AUTO_TEST_CASE(gltf_a_data_uri_states_its_own_type_test) {
    boost::shared_ptr<v3d::asset::media::kind::Model> asset = loadAsset("data_uri_texture.gltf");
    BOOST_REQUIRE(asset);
    boost::shared_ptr<v3d::image::Image> embedded = asset->baseColourImage(0);
    BOOST_REQUIRE(embedded);

    v3d::image::Factory factory(logger());
    boost::shared_ptr<v3d::image::Image> onDisk = factory.read("data/pixel.png");
    BOOST_REQUIRE(onDisk);
    const v3d::image::Difference difference = v3d::image::compare(*embedded, *onDisk, 0);
    BOOST_CHECK_MESSAGE(difference.match, difference.description());
}
