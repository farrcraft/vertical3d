/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/geometry/internal/Walk.h>

#include <api/log/Logger.h>
#include <moya/libmoya/camera/Camera.h>
#include <moya/libmoya/geometry/Cache.h>
#include <moya/libmoya/geometry/Curves.h>
#include <moya/libmoya/geometry/subdivision/Dicing.h>
#include <moya/libmoya/geometry/Geometry.h>
#include <moya/libmoya/geometry/Group.h>
#include <moya/libmoya/geometry/Instance.h>
#include <moya/libmoya/geometry/internal/Displace.h>
#include <moya/libmoya/geometry/internal/LightShapes.h>
#include <moya/libmoya/geometry/internal/Piece.h>
#include <moya/libmoya/geometry/internal/Primvars.h>
#include <moya/libmoya/geometry/Motion.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/geometry/Quadrics.h>
#include <moya/libmoya/geometry/Segments.h>
#include <moya/libmoya/geometry/ShapedLight.h>
#include <moya/libmoya/geometry/subdivision/Surface.h>
#include <moya/libmoya/geometry/subdivision/Tessellation.h>
#include <moya/libmoya/geometry/subdivision/Topology.h>
#include <moya/libmoya/geometry/Triangulate.h>
#include <moya/libmoya/kernel/camera/Camera.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/primitive/Shapes.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/geometry/Sampled.h>
#include <moya/libmoya/kernel/surface/Material.h>
#include <moya/libmoya/kernel/surface/Primvar.h>
#include <moya/libmoya/material/Compile.h>
#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Samples.h>
#include <moya/libmoya/scene/Schema.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>
#include <moya/libmoya/texture/Options.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::moya::geometry::internal {

namespace {

/** A parameter of the scene's options, or its fallback while the scene has none. **/
const scene::Value & option(const scene::Scene & scene, const std::string & name) {
    const std::vector<scene::Handle> options = scene.objects("options");
    if (!options.empty()) {
        return scene.value(options.front(), name);
    }
    for (const scene::Declaration & declaration : scene.schema().kind(*scene.schema().find("options")).parameters) {
        if (declaration.name == name) {
            return declaration.fallback;
        }
    }
    throw std::invalid_argument("the options have no parameter " + name);
}

/** The floats of a sample of points, three to a point. **/
std::vector<float> floatsOf(const scene::Value & sample) {
    std::vector<float> out;
    out.reserve(sample.size() * 3);
    for (std::size_t point = 0; point < sample.size(); ++point) {
        const glm::vec3 at = sample.triple(point);
        out.insert(out.end(), { at.x, at.y, at.z });
    }
    return out;
}

/** How far a point is from a box of a lowest and highest corner that a placement moves. **/
float distanceTo(const glm::vec3 & point, const std::pair<glm::vec3, glm::vec3> & box, const glm::mat4 & placement) {
    const glm::vec3 & low = box.first;
    const glm::vec3 & high = box.second;
    glm::vec3 placedLow { std::numeric_limits<float>::infinity() };
    glm::vec3 placedHigh { -std::numeric_limits<float>::infinity() };
    for (int corner = 0; corner < 8; ++corner) {
        const glm::vec3 at { (corner & 1) != 0 ? high.x : low.x, (corner & 2) != 0 ? high.y : low.y, (corner & 4) != 0 ? high.z : low.z };
        const glm::vec3 placed { placement * glm::vec4(at, 1.0F) };
        placedLow = glm::min(placedLow, placed);
        placedHigh = glm::max(placedHigh, placed);
    }
    return glm::length(glm::max(glm::max(placedLow - point, point - placedHigh), glm::vec3 { 0.0F }));
}

/** The topology of one of the scene's subdivision surfaces, with RI's tags. **/
subdivision::Topology topologyOf(const scene::Scene & scene, const scene::Handle & handle, std::uint32_t points) {
    const auto integers = [&](const char * name) {
        return scene.value(handle, name).integerArray();
    };
    const auto floats = [&](const char * name) {
        return scene.value(handle, name).floatArray();
    };
    subdivision::Topology out;
    out.vertices = integers("vertices");
    out.indices = integers("indices");
    out.points = points;
    const std::string & scheme = scene.value(handle, "scheme").stringArray().front();
    if (scheme == "loop") {
        out.scheme = subdivision::Scheme::Loop;
    } else if (scheme == "bilinear") {
        out.scheme = subdivision::Scheme::Bilinear;
    }
    out.creases = integers("creases");
    out.creaseLengths = integers("creaseLengths");
    out.creaseSharpness = floats("creaseSharpness");
    out.corners = integers("corners");
    out.cornerSharpness = floats("cornerSharpness");
    out.holes = integers("holes");
    const std::string & boundary = scene.value(handle, "boundary").stringArray().front();
    if (boundary == "edges") {
        out.boundary = subdivision::Boundary::Edges;
    } else if (boundary == "edges-and-corners") {
        out.boundary = subdivision::Boundary::EdgesAndCorners;
    }
    return out;
}

/**
 * Everything a subdivision surface's cut is measured from: the dicing camera, the edge length and
 * the surface's placement. Two cuts measured from the same are the same cut.
 **/
std::vector<float> dicingOf(const kernel::camera::Camera & camera, float edgeLength, const glm::mat4 & placement) {
    std::vector<float> out;
    for (const glm::mat4 * matrix : { &camera.toWorld, &placement }) {
        for (int column = 0; column < 4; ++column) {
            for (int row = 0; row < 4; ++row) {
                out.push_back((*matrix)[column][row]);
            }
        }
    }
    out.insert(out.end(), { camera.perspective ? 1.0F : 0.0F, camera.tanHalfFov, camera.screen.x, camera.screen.y, camera.screen.z, camera.screen.w,
        camera.resolution.x, camera.resolution.y, edgeLength });
    return out;
}

/**
 * How many floats shading reads a primvar of a number as. `N`, `Cs` and `tangent` are of three,
 * and `st` of two. Zero is for a primvar a program reads by its name, which holds one to four.
 **/
std::size_t widthOf(std::uint32_t number) {
    if (number == kernel::surface::kTexture) {
        return 2;
    }
    return number < kernel::surface::kFirstNamed ? 3U : 0U;
}

};  // namespace

Walk::Walk(const scene::Scene & scene, const boost::shared_ptr<log::Logger> & logger, Cache * cache) :
    scene_(scene), logger_(logger), groups_(byGroup(scene)), cache_(cache), programs_(scene) {
    prototypes_.primvarNames = programs_.names();
    if (cache_ != nullptr) {
        cache_->made_.clear();
        // a cached geometry holds only the primvars that were read when it was made
        if (cache_->primvarNames_ != programs_.names()) {
            cache_->pieces_.clear();
            cache_->primvarNames_ = programs_.names();
        }
    }
}

Prototypes Walk::take() {
    const kernel::camera::Camera camera = camera::fromScene(scene_);
    prototypes_.open = camera.open;
    prototypes_.close = camera.close;
    prototypes_.world = members(scene::Handle {});
    lights();
    deepen(prototypes_.world, 0);
    cutSurfaces();
    placeVolumes();
    if (cache_ != nullptr) {
        std::erase_if(cache_->pieces_, [&](const auto & held) { return !placed_.contains(held.first); });
        std::erase_if(cache_->grids_, [&](const auto & held) { return !gridIndex_.contains(held.first); });
    }
    return std::move(prototypes_);
}

std::map<std::uint32_t, std::vector<scene::Handle>> Walk::byGroup(const scene::Scene & scene) {
    std::map<std::uint32_t, std::vector<scene::Handle>> out;
    for (const scene::Handle & instance : scene.objects("instance")) {
        const scene::Handle group = scene.value(instance, "group").handleArray().front();
        if (!group.valid()) {
            out[Group::kWorld].push_back(instance);
        } else if (scene.contains(group)) {
            out[group.index].push_back(instance);
        }
    }
    return out;
}

void Walk::report(const std::string & message) {
    if (!reported_.insert(message).second) {
        return;
    }
    prototypes_.reports.push_back(message);
    if (logger_) {
        logger_->get()->warn("{}", message);
    }
}

Group Walk::members(const scene::Handle & group) {
    Group out;
    const auto found = groups_.find(group.valid() ? group.index : Group::kWorld);
    if (found == groups_.end()) {
        return out;
    }
    for (const scene::Handle & handle : found->second) {
        const std::optional<std::uint32_t> at = instance(handle, group);
        if (at) {
            out.instances.push_back(*at);
        }
    }
    return out;
}

std::optional<std::uint32_t> Walk::instance(const scene::Handle & handle, const scene::Handle & holder) {
    const scene::Handle prototype = scene_.value(handle, "prototype").handleArray().front();
    if (!prototype.valid() || !scene_.contains(prototype)) {
        return std::nullopt;
    }
    Instance out;
    out.id = handle.index;
    const scene::Samples & transform = scene_.samples(handle, "transform");
    out.transform = transform.values.front().matrixAt(0);
    if (transform.values.size() > 1) {
        out.keys = keys(transform);
    }
    const std::pair<const char *, std::uint32_t> kinds[] = { { "camera", kernel::geometry::kCameraRay },
        { "shadow", kernel::geometry::kShadowRay }, { "indirect", kernel::geometry::kIndirectRay } };
    for (const auto & [name, bit] : kinds) {
        if (scene_.value(handle, name).integerArray().front() != 0) {
            out.visibility |= bit;
        }
    }
    out.doubleSided = scene_.value(handle, "doubleSided").integerArray().front() != 0;
    out.priority = scene_.value(handle, "priority").integerArray().front();
    const scene::Handle named = scene_.value(handle, "material").handleArray().front();
    if (named.valid() && scene_.contains(named)) {
        out.material = material(named);
    }
    const std::string & kind = scene_.schema().kind(prototype.kind).name;
    if (kind == "group") {
        // the world is held by nothing, so only a group's member can close a loop
        if (holder.valid() && held(prototype.index).contains(holder.index)) {
            report("group " + scene_.name(prototype) + " holds itself, and is left out where it does");
            return std::nullopt;
        }
        out.kind = Prototype::Group;
        out.prototype = group(prototype);
    } else if (kind == "volume") {
        const std::optional<std::uint32_t> fields = volume(prototype);
        if (!fields) {
            return std::nullopt;
        }
        out.kind = Prototype::Volume;
        out.prototype = *fields;
        volumeInstances_.emplace_back(holder.valid() ? holder.index : Group::kWorld, static_cast<std::uint32_t>(prototypes_.instances.size()));
        prototypes_.instances.push_back(out);
        handles_.push_back(handle);
        // a ray is tracked through a volume, so no hierarchy is built over it
        return std::nullopt;
    } else {
        const std::optional<std::uint32_t> placed = geometry(prototype, kind);
        if (!placed) {
            return std::nullopt;
        }
        if (kind != "curves" && out.material != kernel::surface::kNoMaterial && prototypes_.materials[out.material].haired != 0U) {
            report("material " + scene_.name(named) + " is a hair's, and shades what is not a curve by its tangent, met at the middle of a fibre");
        }
        out.kind = Prototype::Geometry;
        out.prototype = *placed;
    }
    const auto index = static_cast<std::uint32_t>(prototypes_.instances.size());
    prototypes_.instances.push_back(out);
    handles_.push_back(handle);
    return index;
}

void Walk::lights() {
    for (const scene::Handle & handle : shapedLights(scene_)) {
        Instance out = lightInstance(scene_, handle);
        out.prototype = lightShape(handle);
        const auto index = static_cast<std::uint32_t>(prototypes_.instances.size());
        prototypes_.instances.push_back(out);
        handles_.push_back(handle);
        prototypes_.world.instances.push_back(index);
        prototypes_.lights.push_back(ShapedLight { .handle = handle, .instance = index });
    }
}

std::uint32_t Walk::lightShape(const scene::Handle & handle) {
    placed_.insert(handle.index);
    if (cache_ != nullptr) {
        const auto held = cache_->pieces_.find(handle.index);
        if (held != cache_->pieces_.end()) {
            const auto index = static_cast<std::uint32_t>(prototypes_.geometries.size());
            prototypes_.geometries.push_back(place(held->second));
            return index;
        }
    }
    const Mark mark = marked();
    const auto index = static_cast<std::uint32_t>(prototypes_.geometries.size());
    prototypes_.geometries.push_back(internal::lightShape(scene_, handle, prototypes_));
    if (cache_ != nullptr) {
        cache_->pieces_.insert_or_assign(handle.index, pieceOf(index, mark));
        cache_->made_.insert(handle.index);
    }
    return index;
}

std::uint32_t Walk::material(const scene::Handle & handle) {
    const auto found = materialIndex_.find(handle.index);
    if (found != materialIndex_.end()) {
        return found->second;
    }
    kernel::surface::Material out;
    std::string space;
    const scene::Handle surface = scene_.value(handle, "surface").handleArray().front();
    // a displacement that displaces nothing says why wherever its material is used
    const material::Compiled * displacement = programs_.displacement(handle.index);
    if (displacement != nullptr && displacement->values.empty()) {
        for (const std::string & message : displacement->reports) {
            report(message);
        }
    }
    if (programs_.compiles(handle.index)) {
        std::vector<std::string> said;
        out = programs_.place(handle.index, prototypes_, said);
        for (const std::string & message : said) {
            report(message);
        }
    } else if (surface.valid() && scene_.contains(surface)) {
        for (const scene::Declaration & declaration : scene_.primvars(surface)) {
            if (declaration.name != "Cs" || scene::floatsOf(declaration.type) != 3U) {
                continue;
            }
            const scene::Value & value = scene_.value(surface, "Cs");
            const std::vector<float> & floats = value.floatArray();
            if (floats.size() >= 3U) {
                out.colour = glm::vec3 { floats[0], floats[1], floats[2] };
                out.coloured = 1;
                space = value.space();
            }
        }
    }
    const auto index = static_cast<std::uint32_t>(prototypes_.materials.size());
    prototypes_.materials.push_back(out);
    prototypes_.materialSpaces.push_back(space);
    prototypes_.materialHandles.push_back(handle);
    materialIndex_.emplace(handle.index, index);
    materialHandles_.push_back(handle);
    return index;
}

std::uint32_t Walk::group(const scene::Handle & handle) {
    const auto found = groupIndex_.find(handle.index);
    if (found != groupIndex_.end()) {
        return found->second;
    }
    // its members first, so that a group comes after every group it holds. No member closes a
    // loop back to this group, so this ends
    Group gathered = members(handle);
    gathered.id = handle.index;
    const auto index = static_cast<std::uint32_t>(prototypes_.groups.size());
    prototypes_.groups.push_back(std::move(gathered));
    groupIndex_.emplace(handle.index, index);
    return index;
}

std::optional<std::uint32_t> Walk::geometry(const scene::Handle & handle, const std::string & kind) {
    const auto found = geometryIndex_.find(handle.index);
    if (found != geometryIndex_.end()) {
        return found->second;
    }
    // a subdivision surface is placed from the cache, or cut, once every instance is gathered
    if (kind == "subdivision") {
        return subdivision(handle);
    }
    if (cache_ != nullptr) {
        const auto held = cache_->pieces_.find(handle.index);
        if (held != cache_->pieces_.end()) {
            const auto index = static_cast<std::uint32_t>(prototypes_.geometries.size());
            prototypes_.geometries.push_back(place(held->second));
            geometryIndex_.emplace(handle.index, index);
            placed_.insert(handle.index);
            return index;
        }
    }
    const Mark mark = marked();
    std::optional<std::uint32_t> out;
    if (kind == "mesh") {
        out = mesh(handle);
    } else if (kind == "points") {
        out = cloud(handle);
    } else if (kind == "sphere" || kind == "cone" || kind == "cylinder" || kind == "hyperboloid" || kind == "paraboloid" || kind == "disk" ||
        kind == "torus") {
        out = quadric(handle);
    } else if (kind == "curves") {
        out = curves(handle);
    } else {
        report("a " + kind + " is not traced, so " + scene_.name(handle) + " is left out");
        return std::nullopt;
    }
    if (!out) {
        return std::nullopt;
    }
    prototypes_.geometries[*out].id = handle.index;
    placed_.insert(handle.index);
    if (cache_ != nullptr) {
        cache_->pieces_.insert_or_assign(handle.index, pieceOf(*out, mark));
        cache_->made_.insert(handle.index);
    }
    return out;
}

Walk::Mark Walk::marked() const {
    return Mark { .points = prototypes_.points.size(),
        .widths = prototypes_.widths.size(),
        .quadrics = prototypes_.quadrics.size(),
        .controls = prototypes_.controls.size(),
        .ends = prototypes_.ends.size(),
        .times = prototypes_.times.size(),
        .faces = prototypes_.faces.size(),
        .primvars = prototypes_.primvars.size(),
        .primvarValues = prototypes_.primvarValues.size(),
        .reports = prototypes_.reports.size() };
}

Geometry Walk::shifted(Geometry geometry, const Mark & from, const Mark & to) {
    const auto move = [](std::uint32_t & offset, std::size_t was, std::size_t is) {
        offset = static_cast<std::uint32_t>((static_cast<std::size_t>(offset) - was) + is);
    };
    // only what a geometry holds is moved, so an offset it does not read is never taken below zero
    const auto moveSampled = [&](kernel::geometry::Sampled & sampled, std::size_t was, std::size_t is) {
        if (sampled.count > 0) {
            move(sampled.first, was, is);
        }
        if (sampled.samples > 1) {
            move(sampled.times, from.times, to.times);
        }
    };
    if (geometry.primvarCount > 0) {
        move(geometry.primvars, from.primvars, to.primvars);
    }
    using Form = kernel::geometry::primitive::Form;
    switch (geometry.form) {
        case Form::Triangles:
            move(geometry.first, from.faces, to.faces);
            moveSampled(geometry.points, from.points, to.points);
            break;
        case Form::RoundPoints:
        case Form::FlatPoints:
            moveSampled(geometry.points, from.points, to.points);
            moveSampled(geometry.widths, from.widths, to.widths);
            break;
        case Form::Quadric:
            moveSampled(geometry.quadric, from.quadrics, to.quadrics);
            break;
        case Form::RoundLinearCurves:
        case Form::FlatLinearCurves:
        case Form::RoundCubicCurves:
        case Form::FlatCubicCurves:
            move(geometry.first, from.faces, to.faces);
            move(geometry.ends, from.ends, to.ends);
            moveSampled(geometry.controls, from.controls, to.controls);
            break;
    }
    return geometry;
}

Piece Walk::pieceOf(std::uint32_t index, const Mark & mark) const {
    const auto since = [](const auto & values, std::size_t from) {
        return std::vector(values.begin() + static_cast<std::ptrdiff_t>(from), values.end());
    };
    Piece out;
    out.geometry = shifted(prototypes_.geometries[index], mark, Mark {});
    out.points = since(prototypes_.points, mark.points);
    out.widths = since(prototypes_.widths, mark.widths);
    out.quadrics = since(prototypes_.quadrics, mark.quadrics);
    out.controls = since(prototypes_.controls, mark.controls);
    out.ends = since(prototypes_.ends, mark.ends);
    out.times = since(prototypes_.times, mark.times);
    out.faces = since(prototypes_.faces, mark.faces);
    out.primvars = since(prototypes_.primvars, mark.primvars);
    for (kernel::surface::Primvar & primvar : out.primvars) {
        primvar.first -= static_cast<std::uint32_t>(mark.primvarValues);
    }
    out.primvarValues = since(prototypes_.primvarValues, mark.primvarValues);
    out.primvarSpaces = since(prototypes_.primvarSpaces, mark.primvars);
    out.reports = since(prototypes_.reports, mark.reports);
    out.made = true;
    return out;
}

Geometry Walk::place(const Piece & piece) {
    const Mark at = marked();
    const auto append = [](auto & values, const auto & added) {
        values.insert(values.end(), added.begin(), added.end());
    };
    append(prototypes_.points, piece.points);
    append(prototypes_.widths, piece.widths);
    append(prototypes_.quadrics, piece.quadrics);
    append(prototypes_.controls, piece.controls);
    append(prototypes_.ends, piece.ends);
    append(prototypes_.times, piece.times);
    append(prototypes_.faces, piece.faces);
    for (kernel::surface::Primvar primvar : piece.primvars) {
        primvar.first += static_cast<std::uint32_t>(at.primvarValues);
        prototypes_.primvars.push_back(primvar);
    }
    append(prototypes_.primvarValues, piece.primvarValues);
    append(prototypes_.primvarSpaces, piece.primvarSpaces);
    for (const std::string & message : piece.reports) {
        report(message);
    }
    return shifted(piece.geometry, Mark {}, at);
}

std::optional<std::uint32_t> Walk::mesh(const scene::Handle & handle) {
    // the faces are cut where the first sample's points lie, and every sample shares the cut
    const scene::Samples & samples = scene_.samples(handle, "P");
    const scene::Value & firstSample = samples.values.front();
    std::vector<glm::vec3> points;
    points.reserve(firstSample.size());
    for (std::size_t i = 0; i < firstSample.size(); ++i) {
        points.push_back(firstSample.triple(i));
    }
    const std::vector<std::int32_t> & indices = scene_.value(handle, "indices").integerArray();
    Triangles triangles;
    try {
        triangles = triangulate(points, scene_.value(handle, "vertices").integerArray(), indices);
    } catch (const std::invalid_argument & refused) {
        report("mesh " + scene_.name(handle) + " is left out, because " + refused.what());
        return std::nullopt;
    }
    if (triangles.dropped > 0) {
        report("mesh " + scene_.name(handle) + " has " + std::to_string(triangles.dropped) + " faces that cover no area");
    }
    Geometry mesh;
    mesh.form = kernel::geometry::primitive::Form::Triangles;
    mesh.first = static_cast<std::uint32_t>(prototypes_.faces.size());
    mesh.count = static_cast<std::uint32_t>(triangles.faces.size());
    mesh.points = sampled(prototypes_.points.size(), triangles.corners.size(), samples.times);
    for (const scene::Value & sample : samples.values) {
        for (const std::uint32_t corner : triangles.corners) {
            prototypes_.points.push_back(sample.triple(static_cast<std::size_t>(indices[corner])));
        }
    }
    prototypes_.faces.insert(prototypes_.faces.end(), triangles.faces.begin(), triangles.faces.end());
    const std::size_t faceCount = scene_.value(handle, "vertices").integerArray().size();
    addPrimvars(handle, mesh, [&](scene::Interpolation interpolation, const std::vector<float> & values, std::size_t width) {
        return spreadOverMesh(interpolation, values, width, triangles, indices, points.size(), faceCount);
    });
    const auto index = static_cast<std::uint32_t>(prototypes_.geometries.size());
    prototypes_.geometries.push_back(mesh);
    geometryIndex_.emplace(handle.index, index);
    return index;
}

std::uint32_t Walk::cloud(const scene::Handle & handle) {
    const scene::Samples & centres = scene_.samples(handle, "P");
    const std::size_t count = centres.values.front().size();
    Geometry cloud;
    cloud.form = scene_.value(handle, "profile").stringArray().front() == "flat" ? kernel::geometry::primitive::Form::FlatPoints :
                                                                                 kernel::geometry::primitive::Form::RoundPoints;
    cloud.count = static_cast<std::uint32_t>(count);
    cloud.points = sampled(prototypes_.points.size(), count, centres.times);
    for (const scene::Value & sample : centres.values) {
        for (std::size_t point = 0; point < count; ++point) {
            prototypes_.points.push_back(sample.triple(point));
        }
    }
    // a width for each point at each sample, or the one constant width for every point
    const scene::Samples & widths = scene_.samples(handle, "width");
    if (widths.values.front().size() == count && count > 0) {
        cloud.widths = sampled(prototypes_.widths.size(), count, widths.times);
        for (const scene::Value & sample : widths.values) {
            const std::vector<float> & values = sample.floatArray();
            prototypes_.widths.insert(prototypes_.widths.end(), values.begin(), values.end());
        }
    } else {
        if (!widths.values.front().floatArray().empty()) {
            report("points " + scene_.name(handle) + " has a width for some of its points but not all, and takes its constant width");
        }
        cloud.widths = sampled(prototypes_.widths.size(), count, {});
        prototypes_.widths.insert(prototypes_.widths.end(), count, scene_.value(handle, "constantWidth").floatArray().front());
    }
    addPrimvars(handle, cloud, [&](scene::Interpolation interpolation, const std::vector<float> & values, std::size_t width) {
        return spreadOverPoints(interpolation, values, width, count);
    });
    const auto index = static_cast<std::uint32_t>(prototypes_.geometries.size());
    prototypes_.geometries.push_back(cloud);
    geometryIndex_.emplace(handle.index, index);
    return index;
}

std::uint32_t Walk::quadric(const scene::Handle & handle) {
    // every time any dimension is sampled at, since each moves linearly between its own samples
    std::set<float> times;
    for (const scene::Declaration & declaration : scene_.schema().kind(handle.kind).parameters) {
        if (declaration.motion) {
            const std::vector<float> & own = scene_.samples(handle, declaration.name).times;
            times.insert(own.begin(), own.end());
        }
    }
    const std::vector<float> at = times.empty() ? std::vector<float> { 0.0F } : std::vector<float>(times.begin(), times.end());
    Geometry shape;
    shape.form = kernel::geometry::primitive::Form::Quadric;
    shape.count = 1;
    shape.quadric = sampled(prototypes_.quadrics.size(), 1, times.empty() ? std::vector<float> {} : at);
    for (const float time : at) {
        prototypes_.quadrics.push_back(quadricAt(scene_, handle, time));
    }
    addPrimvars(handle, shape, &spreadOverQuadric);
    const auto index = static_cast<std::uint32_t>(prototypes_.geometries.size());
    prototypes_.geometries.push_back(shape);
    geometryIndex_.emplace(handle.index, index);
    return index;
}

std::optional<std::uint32_t> Walk::subdivision(const scene::Handle & handle) {
    const std::size_t count = scene_.samples(handle, "P").values.front().size();
    std::optional<subdivision::Surface> surface;
    if (cache_ != nullptr) {
        const auto held = cache_->pieces_.find(handle.index);
        if (held != cache_->pieces_.end()) {
            surface = held->second.surface;
        }
    }
    try {
        if (!surface) {
            surface.emplace(topologyOf(scene_, handle, static_cast<std::uint32_t>(count)));
        }
    } catch (const std::invalid_argument & refused) {
        report("subdivision " + scene_.name(handle) + " is left out, because " + refused.what());
        return std::nullopt;
    }
    const auto index = static_cast<std::uint32_t>(prototypes_.geometries.size());
    prototypes_.geometries.emplace_back().id = handle.index;
    geometryIndex_.emplace(handle.index, index);
    placed_.insert(handle.index);
    uncut_.push_back(Uncut { .handle = handle, .index = index, .surface = std::move(*surface) });
    return index;
}

void Walk::cutSurfaces() {
    if (uncut_.empty()) {
        return;
    }
    const kernel::camera::Camera camera = camera::dicingFromScene(scene_);
    const float edgeLength = option(scene_, "edgeLength").floatArray().front();
    const std::map<std::uint32_t, Placement> placements = nearest(camera);
    for (const Uncut & uncut : uncut_) {
        const auto placed = placements.find(uncut.index);
        const Placement placement = placed == placements.end() ? Placement {} : placed->second;
        const material::Compiled * displacement = displacementOf(placement.material);
        const std::vector<float> measure = dicingOf(camera, edgeLength, placement.transform);
        const std::string print = fingerprintOf(displacement);
        if (cache_ != nullptr) {
            const auto held = cache_->pieces_.find(uncut.handle.index);
            if (held != cache_->pieces_.end() && held->second.made && held->second.dicing == measure && held->second.displaced == print) {
                prototypes_.geometries[uncut.index] = place(held->second);
                continue;
            }
        }
        const Mark mark = marked();
        const subdivision::Dicing dicing(camera, edgeLength, placement.transform);
        const scene::Samples & samples = scene_.samples(uncut.handle, "P");
        // the surface is measured at its first sample's points, and every sample shares the cut
        const subdivision::Tessellation tessellation = uncut.surface.cut(floatsOf(samples.values.front()), dicing);
        Geometry & mesh = prototypes_.geometries[uncut.index];
        mesh.form = kernel::geometry::primitive::Form::Triangles;
        mesh.first = static_cast<std::uint32_t>(prototypes_.faces.size());
        mesh.count = static_cast<std::uint32_t>(tessellation.faces.size());
        mesh.points = sampled(prototypes_.points.size(), tessellation.corners.size(), samples.times);
        // every sample's points are evaluated on the one surface, whose patches its topology fixes
        for (const scene::Value & sample : samples.values) {
            const std::vector<float> limit = uncut.surface.at(floatsOf(sample), 3, scene::Interpolation::Vertex, tessellation);
            for (const std::uint32_t corner : tessellation.corners) {
                const std::size_t at = static_cast<std::size_t>(corner) * 3U;
                prototypes_.points.emplace_back(limit[at], limit[at + 1U], limit[at + 2U]);
            }
        }
        prototypes_.faces.insert(prototypes_.faces.end(), tessellation.faces.begin(), tessellation.faces.end());
        // an undisplaced surface shades with its limit normals, a displaced one with its own
        if (displacement == nullptr) {
            const std::vector<float> normals = uncut.surface.normals(floatsOf(samples.values.front()), tessellation);
            Spread spread { .perPrimitive = 3, .values = {} };
            spread.values.reserve(tessellation.corners.size() * 3U);
            for (const std::uint32_t corner : tessellation.corners) {
                const auto from = normals.begin() + static_cast<std::ptrdiff_t>(static_cast<std::size_t>(corner) * 3U);
                spread.values.insert(spread.values.end(), from, from + 3);
            }
            addPrimvar(mesh, kernel::surface::kNormal, 3, spread, std::nullopt);
        }
        addPrimvars(uncut.handle, mesh, [&](scene::Interpolation interpolation, const std::vector<float> & values, std::size_t width) {
            return spreadOverSurface(interpolation, values, width, uncut.surface, tessellation);
        });
        if (displacement != nullptr) {
            displace(uncut, tessellation, *displacement, placement.transform, mark);
        }
        if (cache_ != nullptr) {
            Piece piece = pieceOf(uncut.index, mark);
            piece.surface = uncut.surface;
            piece.dicing = measure;
            piece.displaced = print;
            cache_->pieces_.insert_or_assign(uncut.handle.index, std::move(piece));
            cache_->made_.insert(uncut.handle.index);
        }
    }
}

std::map<std::uint32_t, Walk::Placement> Walk::nearest(const kernel::camera::Camera & camera) const {
    // each surface's box, grown by its bound, by its index among the geometries
    std::map<std::uint32_t, std::pair<glm::vec3, glm::vec3>> boxes;
    for (const Uncut & uncut : uncut_) {
        const scene::Value & first = scene_.samples(uncut.handle, "P").values.front();
        glm::vec3 low { std::numeric_limits<float>::infinity() };
        glm::vec3 high { -std::numeric_limits<float>::infinity() };
        for (std::size_t point = 0; point < first.size(); ++point) {
            low = glm::min(low, first.triple(point));
            high = glm::max(high, first.triple(point));
        }
        const float bound = std::max(scene_.value(uncut.handle, "displacementBound").floatArray().front(), 0.0F);
        boxes.emplace(uncut.index, std::make_pair(low - bound, high + bound));
    }
    const glm::vec3 eye { camera.toWorld * glm::vec4(0.0F, 0.0F, 0.0F, 1.0F) };
    std::map<std::uint32_t, std::pair<float, Placement>> best;
    // every placement of every surface, down the chains of instances a traversal follows. The
    // innermost instance of a chain that names a material gives it
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the chain's material, then its depth
    const auto place = [&](const auto & self, const Group & group, const glm::mat4 & above, std::uint32_t named, std::uint32_t depth) -> void {
        if (depth == kernel::geometry::kInstanceDepth) {
            return;
        }
        for (const std::uint32_t index : group.instances) {
            const Instance & instance = prototypes_.instances[index];
            const glm::mat4 placement = above * instance.transform;
            const std::uint32_t material = instance.material != kernel::surface::kNoMaterial ? instance.material : named;
            if (instance.kind == Prototype::Group) {
                self(self, prototypes_.groups[instance.prototype], placement, material, depth + 1);
                continue;
            }
            const auto box = boxes.find(instance.prototype);
            if (box == boxes.end()) {
                continue;
            }
            const float distance = distanceTo(eye, box->second, placement);
            const auto held = best.find(instance.prototype);
            if (held == best.end() || distance < held->second.first) {
                best[instance.prototype] = std::make_pair(distance, Placement { .transform = placement, .material = material });
            }
        }
    };
    place(place, prototypes_.world, glm::mat4(1.0F), kernel::surface::kNoMaterial, 0);
    std::map<std::uint32_t, Placement> out;
    for (const auto & [index, held] : best) {
        out.emplace(index, held.second);
    }
    return out;
}

const material::Compiled * Walk::displacementOf(std::uint32_t material) {
    if (material >= materialHandles_.size()) {
        return nullptr;
    }
    const material::Compiled * out = programs_.displacement(materialHandles_[material].index);
    if (out == nullptr) {
        return nullptr;
    }
    for (const std::string & message : out->reports) {
        report(message);
    }
    return out->values.empty() ? nullptr : out;
}

void Walk::displace(const Uncut & uncut, const subdivision::Tessellation & tessellation, const material::Compiled & displacement, const glm::mat4 & placement,
    const Mark & mark) {
    const scene::Samples & samples = scene_.samples(uncut.handle, "P");
    const std::vector<float> normals = uncut.surface.normals(floatsOf(samples.values.front()), tessellation);
    std::vector<Lift> lifts = liftsOf(pieceOf(uncut.index, mark), displacement, tessellation.corners, normals, placement, texture::optionsOf(scene_), logger_);
    const float bound = std::max(scene_.value(uncut.handle, "displacementBound").floatArray().front(), 0.0F);
    if (heldWithin(lifts, bound)) {
        std::ostringstream message;
        message << "subdivision " << scene_.name(uncut.handle) << " is displaced beyond its bound of " << bound << ", and is held to it";
        report(message.str());
    }
    const Geometry & mesh = prototypes_.geometries[uncut.index];
    const std::size_t count = tessellation.corners.size();
    for (std::size_t sample = 0; sample < samples.values.size(); ++sample) {
        const std::vector<glm::vec3> moved = raised(uncut.surface, tessellation, floatsOf(samples.values[sample]), lifts);
        std::copy(moved.begin(), moved.end(), prototypes_.points.begin() + static_cast<std::ptrdiff_t>(mesh.points.first + (sample * count)));
    }
}

std::optional<std::uint32_t> Walk::curves(const scene::Handle & handle) {
    const scene::Samples & positions = scene_.samples(handle, "P");
    const scene::Samples & widths = scene_.samples(handle, "width");
    const std::vector<std::int32_t> & vertices = scene_.value(handle, "vertices").integerArray();
    const bool cubic = scene_.value(handle, "type").stringArray().front() == "cubic";
    const std::string & basis = scene_.value(handle, "basis").stringArray().front();
    const bool periodic = scene_.value(handle, "wrap").stringArray().front() == "periodic";
    const bool flat = scene_.value(handle, "profile").stringArray().front() == "flat";
    const std::size_t count = positions.values.front().size();
    std::size_t varying = 0;
    try {
        varying = varyingCount(vertices, cubic, basis, periodic);
    } catch (const std::invalid_argument & refused) {
        report("curves " + scene_.name(handle) + " is left out, because " + refused.what());
        return std::nullopt;
    }
    // a width for each varying value or each vertex at each sample, or the one constant width
    const std::size_t given = widths.values.front().floatArray().size();
    const bool constant = given != varying && given != count;
    if (constant && given > 0) {
        report("curves " + scene_.name(handle) + " has a width for some of its vertices but not all, and takes its constant width");
    }
    // every time the points or the widths are sampled at, since each moves linearly between its
    // own samples and the segments are linear in both
    std::set<float> times(positions.times.begin(), positions.times.end());
    if (!constant) {
        times.insert(widths.times.begin(), widths.times.end());
    }
    const std::vector<float> at = times.size() > 1 ? std::vector<float>(times.begin(), times.end()) : std::vector<float> { 0.0F };
    const float constantWidth = scene_.value(handle, "constantWidth").floatArray().front();
    std::vector<Segments> cut;
    try {
        for (const float time : at) {
            std::vector<glm::vec3> points;
            points.reserve(count);
            for (std::size_t point = 0; point < count; ++point) {
                points.push_back(pointOf(positions, point, time));
            }
            std::vector<float> diameters(varying, constantWidth);
            if (!constant) {
                diameters.resize(given);
                for (std::size_t width = 0; width < given; ++width) {
                    diameters[width] = numberAt(widths, width, time);
                }
            }
            cut.push_back(geometry::cut(points, diameters, vertices, cubic, basis, periodic));
        }
    } catch (const std::invalid_argument & refused) {
        report("curves " + scene_.name(handle) + " is left out, because " + refused.what());
        return std::nullopt;
    }
    Geometry shape;
    if (cubic) {
        shape.form = flat ? kernel::geometry::primitive::Form::FlatCubicCurves : kernel::geometry::primitive::Form::RoundCubicCurves;
    } else {
        shape.form = flat ? kernel::geometry::primitive::Form::FlatLinearCurves : kernel::geometry::primitive::Form::RoundLinearCurves;
    }
    shape.first = static_cast<std::uint32_t>(prototypes_.faces.size());
    shape.count = static_cast<std::uint32_t>(cut.front().curves.size());
    shape.controls = sampled(prototypes_.controls.size(), cut.front().controls.size(), at.size() > 1 ? at : std::vector<float> {});
    for (const Segments & segments : cut) {
        prototypes_.controls.insert(prototypes_.controls.end(), segments.controls.begin(), segments.controls.end());
    }
    prototypes_.faces.insert(prototypes_.faces.end(), cut.front().curves.begin(), cut.front().curves.end());
    shape.ends = static_cast<std::uint32_t>(prototypes_.ends.size());
    prototypes_.ends.insert(prototypes_.ends.end(), cut.front().ends.begin(), cut.front().ends.end());
    addPrimvars(handle, shape, [&](scene::Interpolation interpolation, const std::vector<float> & values, std::size_t width) {
        return spreadOverCurves(interpolation, values, width, cut.front().curves, vertices.size());
    });
    const auto index = static_cast<std::uint32_t>(prototypes_.geometries.size());
    prototypes_.geometries.push_back(shape);
    geometryIndex_.emplace(handle.index, index);
    return index;
}

kernel::geometry::Sampled Walk::sampled(std::size_t first, std::size_t count, const std::vector<float> & times) {
    kernel::geometry::Sampled out;
    out.first = static_cast<std::uint32_t>(first);
    out.count = static_cast<std::uint32_t>(count);
    out.times = static_cast<std::uint32_t>(prototypes_.times.size());
    out.samples = times.size() > 1 ? static_cast<std::uint32_t>(times.size()) : 1U;
    if (times.size() > 1) {
        prototypes_.times.insert(prototypes_.times.end(), times.begin(), times.end());
    }
    return out;
}

void Walk::addPrimvars(const scene::Handle & handle, Geometry & geometry,
    const std::function<Spread(scene::Interpolation, const std::vector<float> &, std::size_t)> & spread) {
    const std::vector<std::string> & names = programs_.names();
    for (std::uint32_t number = 0; number < names.size(); ++number) {
        // a primvar the geometry already holds, such as a subdivision surface's limit normal, stays
        bool holds = false;
        for (std::uint32_t i = 0; i < geometry.primvarCount; ++i) {
            holds = holds || prototypes_.primvars[geometry.primvars + i].name == number;
        }
        for (const scene::Declaration & declaration : scene_.primvars(handle)) {
            if (holds || declaration.name != names[number]) {
                continue;
            }
            const std::string what = "primvar " + declaration.name + " of " + scene_.name(handle);
            const std::size_t width = scene::floatsOf(declaration.type) * declaration.width;
            const std::size_t wanted = widthOf(number);
            const bool fits = wanted == 0U ? width >= 1U && width <= scene::Declaration::kMostWidth : width == wanted;
            if (!fits) {
                report(what + " is not of the floats shading reads, so it is left out");
                break;
            }
            const scene::Value & value = scene_.value(handle, declaration.name);
            const std::optional<std::string> space = declaration.type == scene::Type::Colour ? std::optional<std::string>(value.space()) : std::nullopt;
            try {
                addPrimvar(geometry, number, static_cast<std::uint32_t>(width), spread(declaration.interpolation, value.floatArray(), width), space);
            } catch (const std::invalid_argument & refused) {
                report(what + " is left out, because " + refused.what());
            }
            break;
        }
    }
}

void Walk::addPrimvar(Geometry & geometry, std::uint32_t name, std::uint32_t components, const Spread & spread, const std::optional<std::string> & space) {
    // a geometry's primvars are added together, so they are one run
    if (geometry.primvarCount == 0) {
        geometry.primvars = static_cast<std::uint32_t>(prototypes_.primvars.size());
    }
    prototypes_.primvars.push_back(kernel::surface::Primvar { .name = name,
        .components = components,
        .first = static_cast<std::uint32_t>(prototypes_.primvarValues.size()),
        .perPrimitive = spread.perPrimitive });
    prototypes_.primvarValues.insert(prototypes_.primvarValues.end(), spread.values.begin(), spread.values.end());
    prototypes_.primvarSpaces.push_back(space);
    ++geometry.primvarCount;
}

const std::set<std::uint32_t> & Walk::held(std::uint32_t group) {
    const auto found = held_.find(group);
    if (found != held_.end()) {
        return found->second;
    }
    std::set<std::uint32_t> out;
    std::vector<std::uint32_t> open { group };
    while (!open.empty()) {
        const std::uint32_t at = open.back();
        open.pop_back();
        const auto members = groups_.find(at);
        if (members == groups_.end()) {
            continue;
        }
        for (const scene::Handle & handle : members->second) {
            const scene::Handle prototype = scene_.value(handle, "prototype").handleArray().front();
            if (prototype.valid() && scene_.contains(prototype) && scene_.schema().kind(prototype.kind).name == "group" &&
                out.insert(prototype.index).second) {
                open.push_back(prototype.index);
            }
        }
    }
    return held_.emplace(group, std::move(out)).first->second;
}

void Walk::deepen(const Group & group, std::uint32_t depth) {
    for (const std::uint32_t index : group.instances) {
        if (depth == kernel::geometry::kInstanceDepth) {
            report("instance " + scene_.name(handles_[index]) + " is nested deeper than " + std::to_string(kernel::geometry::kInstanceDepth) +
                " instances, and is left out");
            continue;
        }
        const Instance & instance = prototypes_.instances[index];
        // a group met again at one depth reports nothing new
        if (instance.kind == Prototype::Group && deepened_.emplace(instance.prototype, depth + 1).second) {
            deepen(prototypes_.groups[instance.prototype], depth + 1);
        }
    }
}

};  // namespace v3d::moya::geometry::internal
