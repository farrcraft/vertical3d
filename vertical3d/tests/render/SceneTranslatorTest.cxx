/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <api/brep/Primitives.h>
#include <api/curve/Curve.h>
#include <api/curve/Curves.h>
#include <api/log/Logger.h>
#include <api/type/camera/Profile.h>
#include <moya/libmoya/ri/SceneClient.h>
#include <moya/libmoya/scene/Description.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Scene.h>
#include <vertical3d/src/render/MoyaDelegate.h>
#include <vertical3d/src/render/RIBExportVisitor.h>
#include <vertical3d/src/render/SceneTranslator.h>
#include <vertical3d/src/render/Shot.h>
#include <vertical3d/src/scene/item/Curve.h>
#include <vertical3d/src/scene/item/Group.h>
#include <vertical3d/src/scene/item/Light.h>
#include <vertical3d/src/scene/item/Mesh.h>
#include <vertical3d/src/scene/RenderSettings.h>
#include <vertical3d/src/scene/Scene.h>

#include <cstdint>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace {

namespace moya = v3d::moya::scene;
namespace scene = v3d::editor::scene;
typedef v3d::editor::render::RIBExportVisitor::Output Output;

/** The camera every case renders from. **/
v3d::type::camera::Profile view() {
    v3d::type::camera::Profile profile("shot");
    profile.fov(50.0F);
    profile.clipping(glm::vec2(0.5F, 250.0F));
    profile.eye(glm::vec3(0.0F, 0.0F, -8.0F));
    return profile;
}

/** A small picture, at two samples along each axis. **/
Output output() {
    Output made;
    made.settings.size = glm::uvec2(64, 48);
    made.settings.samples = 2;
    return made;
}

/** The scene the translator makes of an editor's scene. **/
std::string translated(const scene::Scene& editor) {
    moya::Scene made(moya::kinds());
    v3d::editor::render::SceneTranslator translator(&made);
    translator.update(editor, editor.evaluate(), view(), output());
    made.apply();
    return moya::describe(made);
}

/** The scene the RIB export makes of an editor's scene through moya's RIB client. **/
std::string exported(const scene::Scene& editor) {
    moya::Scene made(moya::kinds());
    v3d::moya::ri::SceneClient client(made, boost::make_shared<v3d::log::Logger>());
    client.frame([&made]() { made.apply(); });
    v3d::editor::render::shoot(&client, editor, editor.evaluate(), view(), output());
    return moya::describe(made);
}

/** What happened to each object after a version, as kind, name and what changed. **/
std::vector<std::string> changes(const moya::Scene& made, std::uint64_t since) {
    std::vector<std::string> found;
    for (const moya::Change& change : made.changes(since)) {
        std::string line = made.schema().kind(change.handle.kind).name;
        switch (change.what) {
            case moya::Change::What::Created: line += " created"; break;
            case moya::Change::What::Modified: line += " modified"; break;
            case moya::Change::What::Removed: line += " removed"; break;
        }
        for (const std::string& parameter : change.parameters) {
            line += " " + parameter;
        }
        found.push_back(line);
    }
    return found;
}

BOOST_AUTO_TEST_CASE(translator_a_mesh_alone_describes_as_the_rib_export_does_test) {
    scene::Scene editor;
    editor.add(boost::make_shared<scene::item::Mesh>(v3d::brep::cube()));
    const std::string direct = translated(editor);
    BOOST_TEST(direct == exported(editor));
    BOOST_TEST(direct.find("distantLight \"camera light\"") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(translator_a_scene_of_every_kind_describes_as_the_rib_export_does_test) {
    scene::Scene editor;
    const boost::shared_ptr<scene::item::Group> outer = boost::make_shared<scene::item::Group>();
    const boost::shared_ptr<scene::item::Mesh> held = boost::make_shared<scene::item::Mesh>(v3d::brep::plane());
    const boost::shared_ptr<scene::item::Mesh> inner = boost::make_shared<scene::item::Mesh>(v3d::brep::cube());
    outer->translation(glm::vec3(0.0F, 10.0F, 0.0F));
    held->translation(glm::vec3(2.0F, 0.0F, 0.0F));
    inner->translation(glm::vec3(0.0F, 0.0F, 3.0F));
    editor.add(outer);
    editor.add(held);
    editor.add(inner);
    editor.parent(held->id(), outer->id());
    editor.parent(inner->id(), held->id());

    for (const scene::item::Light::Type type : { scene::item::Light::Type::Point, scene::item::Light::Type::Distant, scene::item::Light::Type::Spot }) {
        const boost::shared_ptr<scene::item::Light> light = boost::make_shared<scene::item::Light>();
        scene::item::Light::Emission emission;
        emission.type = type;
        emission.intensity = 2.0F;
        emission.colour = glm::vec3(1.0F, 0.5F, 0.25F);
        light->emission(emission);
        light->translation(glm::vec3(1.0F, 2.0F, 3.0F));
        editor.add(light);
        editor.parent(light->id(), outer->id());
    }

    // two Catmull-Rom curves make one run, and a closed poly curve after them another
    const boost::shared_ptr<v3d::curve::Curves> curves = boost::make_shared<v3d::curve::Curves>();
    const std::vector<glm::vec3> points{ { 0, 0, 0 }, { 1, 1, 0 }, { 2, 0, 0 } };
    BOOST_REQUIRE(curves->add({ .shape = v3d::curve::CatmullRom{ points } }).empty());
    BOOST_REQUIRE(curves->add({ .shape = v3d::curve::CatmullRom{ points } }).empty());
    BOOST_REQUIRE(curves->add({ .shape = v3d::curve::Poly{ points }, .closed = true }).empty());
    const boost::shared_ptr<scene::item::Curve> hair = boost::make_shared<scene::item::Curve>(curves);
    hair->style({ .renders = true, .width = 0.25F });
    editor.add(hair);

    const std::string direct = translated(editor);
    BOOST_TEST(direct == exported(editor));
    BOOST_TEST(direct.find("camera light") == std::string::npos);
    BOOST_TEST(direct.find("curves \"" + hair->name() + " 2\"") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(translator_moving_one_item_sets_its_instance_alone_test) {
    scene::Scene editor;
    const boost::shared_ptr<scene::item::Mesh> moved = boost::make_shared<scene::item::Mesh>(v3d::brep::cube());
    editor.add(moved);
    editor.add(boost::make_shared<scene::item::Mesh>(v3d::brep::plane()));
    moya::Scene made(moya::kinds());
    v3d::editor::render::SceneTranslator translator(&made);
    BOOST_TEST(translator.update(editor, editor.evaluate(), view(), output()));
    made.apply();
    const std::uint64_t built = made.version();

    // nothing moved, so nothing is committed
    BOOST_TEST(!translator.update(editor, editor.evaluate(), view(), output()));
    BOOST_TEST(made.apply() == built);

    moved->translation(glm::vec3(1.0F, 0.0F, 0.0F));
    BOOST_TEST(translator.update(editor, editor.evaluate(), view(), output()));
    made.apply();
    BOOST_TEST(made.version() == built + 1U);
    BOOST_TEST(changes(made, built) == (std::vector<std::string> { "instance modified transform" }), boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(translator_removing_an_item_removes_its_objects_test) {
    scene::Scene editor;
    const boost::shared_ptr<scene::item::Mesh> gone = boost::make_shared<scene::item::Mesh>(v3d::brep::cube());
    editor.add(gone);
    editor.add(boost::make_shared<scene::item::Mesh>(v3d::brep::plane()));
    moya::Scene made(moya::kinds());
    v3d::editor::render::SceneTranslator translator(&made);
    translator.update(editor, editor.evaluate(), view(), output());
    made.apply();
    const std::uint64_t built = made.version();

    BOOST_REQUIRE(editor.remove(static_cast<unsigned int>(gone->id())));
    BOOST_TEST(translator.update(editor, editor.evaluate(), view(), output()));
    made.apply();
    BOOST_TEST(changes(made, built) == (std::vector<std::string> { "mesh removed", "instance removed" }), boost::test_tools::per_element());
    BOOST_TEST(!made.find("mesh", gone->name()).has_value());
    BOOST_TEST(made.objects("mesh").size() == 1U);
}

BOOST_AUTO_TEST_CASE(translator_the_moya_delegate_keeps_its_scene_in_step_test) {
    scene::Scene editor;
    const boost::shared_ptr<scene::item::Mesh> cube = boost::make_shared<scene::item::Mesh>(v3d::brep::cube());
    editor.add(cube);
    v3d::editor::render::MoyaDelegate delegate(boost::make_shared<v3d::log::Logger>());
    delegate.render(editor, view(), output().settings);
    BOOST_TEST(delegate.retained().find("instance", cube->name()).has_value());
    BOOST_TEST(delegate.retained().find("options", "options").has_value());
    delegate.cancel();
}

};  // namespace
