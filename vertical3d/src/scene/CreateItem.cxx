/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "CreateItem.h"

#include <api/type/camera/Profile.h>

#include "Camera.h"
#include "Evaluation.h"
#include "Light.h"

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace v3d::editor {

/**
 **/
glm::quat facing(const v3d::type::camera::Profile& view) {
    // a profile's orientation turns its own axes into the world's, as an item's rotation does.
    // It is always a rotation: a mirrored hand is applied only as the view is built
    return glm::normalize(glm::quat_cast(glm::mat3(view.orientation())));
}

/**
 **/
boost::shared_ptr<Camera> create_camera(const v3d::type::camera::Profile& view, Camera::Projection projection) {
    const boost::shared_ptr<Camera> camera = boost::make_shared<Camera>();
    camera->translation(view.eye());
    camera->rotation(facing(view));

    Camera::Lens lens;
    lens.projection = projection;
    lens.fov = view.fov();
    lens.extent = view.orthoZoom();
    lens.clipping = view.clipping();
    // a lens a document could not read back is replaced by the default lens
    if (!lens.valid()) {
        lens = Camera::Lens();
        lens.projection = projection;
    }
    camera->lens(lens);
    return camera;
}

/**
 **/
v3d::type::camera::Profile profile_of(const Camera& camera, const glm::mat4& world,
    const v3d::type::camera::Profile& picture) {
    v3d::type::camera::Profile profile(picture);
    profile.name("scene camera");
    const glm::quat rotation = orientation(world);
    profile.eye(glm::vec3(world[3]));
    profile.right(rotation * glm::vec3(1.0F, 0.0F, 0.0F));
    profile.up(rotation * glm::vec3(0.0F, 1.0F, 0.0F));
    profile.direction(rotation * glm::vec3(0.0F, 0.0F, 1.0F));
    profile.rotation(rotation);

    const Camera::Lens& lens = camera.lens();
    profile.orthographic(lens.projection == Camera::Projection::Orthographic);
    profile.fov(lens.fov);
    profile.orthoZoom(lens.extent);
    profile.clipping(lens.clipping);
    return profile;
}

/**
 **/
boost::shared_ptr<Light> create_light(const v3d::type::camera::Profile& view, Light::Type type) {
    const boost::shared_ptr<Light> light = boost::make_shared<Light>();
    light->translation(view.eye());
    light->rotation(facing(view));
    Light::Emission emission;
    emission.type = type;
    light->emission(emission);
    return light;
}

};  // namespace v3d::editor
