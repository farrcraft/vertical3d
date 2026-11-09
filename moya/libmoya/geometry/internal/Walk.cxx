/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/geometry/internal/Walk.h>

#include <api/log/Logger.h>
#include <moya/libmoya/camera/Camera.h>
#include <moya/libmoya/geometry/Curves.h>
#include <moya/libmoya/geometry/Geometry.h>
#include <moya/libmoya/geometry/Group.h>
#include <moya/libmoya/geometry/Instance.h>
#include <moya/libmoya/geometry/Motion.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/geometry/Quadrics.h>
#include <moya/libmoya/geometry/Segments.h>
#include <moya/libmoya/geometry/Surface.h>
#include <moya/libmoya/geometry/Tessellation.h>
#include <moya/libmoya/geometry/Topology.h>
#include <moya/libmoya/geometry/Triangulate.h>
#include <moya/libmoya/kernel/camera/Camera.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/primitive/Shapes.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/geometry/Sampled.h>
#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Samples.h>
#include <moya/libmoya/scene/Schema.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <glm/vec3.hpp>

namespace v3d::moya::geometry::internal {

namespace {

/** How many pieces each side of a subdivision surface's patch faces is cut into. **/
constexpr std::uint32_t kSubdivisionRate = 8;

/** The topology of one of the scene's subdivision surfaces, with RI's tags. **/
Topology topologyOf(const scene::Scene & scene, const scene::Handle & handle, std::uint32_t points) {
    const auto integers = [&](const char * name) {
        return scene.value(handle, name).integerArray();
    };
    const auto floats = [&](const char * name) {
        return scene.value(handle, name).floatArray();
    };
    Topology out;
    out.vertices = integers("vertices");
    out.indices = integers("indices");
    out.points = points;
    const std::string & scheme = scene.value(handle, "scheme").stringArray().front();
    if (scheme == "loop") {
        out.scheme = Scheme::Loop;
    } else if (scheme == "bilinear") {
        out.scheme = Scheme::Bilinear;
    }
    out.creases = integers("creases");
    out.creaseLengths = integers("creaseLengths");
    out.creaseSharpness = floats("creaseSharpness");
    out.corners = integers("corners");
    out.cornerSharpness = floats("cornerSharpness");
    out.holes = integers("holes");
    const std::string & boundary = scene.value(handle, "boundary").stringArray().front();
    if (boundary == "edges") {
        out.boundary = Boundary::Edges;
    } else if (boundary == "edges-and-corners") {
        out.boundary = Boundary::EdgesAndCorners;
    }
    return out;
}

};  // namespace

Walk::Walk(const scene::Scene & scene, const boost::shared_ptr<log::Logger> & logger) :
    scene_(scene), logger_(logger), groups_(byGroup(scene)) {
}

Prototypes Walk::take() {
    const kernel::camera::Camera camera = camera::fromScene(scene_);
    prototypes_.open = camera.open;
    prototypes_.close = camera.close;
    prototypes_.world = members(scene::Handle {});
    deepen(prototypes_.world, 0);
    return std::move(prototypes_);
}

std::map<std::uint32_t, std::vector<scene::Handle>> Walk::byGroup(const scene::Scene & scene) {
    std::map<std::uint32_t, std::vector<scene::Handle>> out;
    for (const scene::Handle & instance : scene.objects("instance")) {
        const scene::Handle group = scene.value(instance, "group").handleArray().front();
        if (!group.valid()) {
            out[kWorld].push_back(instance);
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
    const auto found = groups_.find(group.valid() ? group.index : kWorld);
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
    const std::string & kind = scene_.schema().kind(prototype.kind).name;
    if (kind == "group") {
        // the world is held by nothing, so only a group's member can close a loop
        if (holder.valid() && held(prototype.index).contains(holder.index)) {
            report("group " + scene_.name(prototype) + " holds itself, and is left out where it does");
            return std::nullopt;
        }
        out.kind = Prototype::Group;
        out.prototype = group(prototype);
    } else if (kind == "mesh") {
        const std::optional<std::uint32_t> cut = mesh(prototype);
        if (!cut) {
            return std::nullopt;
        }
        out.kind = Prototype::Geometry;
        out.prototype = *cut;
    } else if (kind == "points") {
        out.kind = Prototype::Geometry;
        out.prototype = cloud(prototype);
    } else if (kind == "sphere" || kind == "cone" || kind == "cylinder" || kind == "hyperboloid" || kind == "paraboloid" || kind == "disk" ||
        kind == "torus") {
        out.kind = Prototype::Geometry;
        out.prototype = quadric(prototype);
    } else if (kind == "subdivision") {
        const std::optional<std::uint32_t> cut = subdivision(prototype);
        if (!cut) {
            return std::nullopt;
        }
        out.kind = Prototype::Geometry;
        out.prototype = *cut;
    } else if (kind == "curves") {
        const std::optional<std::uint32_t> cut = curves(prototype);
        if (!cut) {
            return std::nullopt;
        }
        out.kind = Prototype::Geometry;
        out.prototype = *cut;
    } else {
        report("a " + kind + " is not traced, so " + scene_.name(prototype) + " is left out");
        return std::nullopt;
    }
    const auto index = static_cast<std::uint32_t>(prototypes_.instances.size());
    prototypes_.instances.push_back(out);
    handles_.push_back(handle);
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
    const auto index = static_cast<std::uint32_t>(prototypes_.groups.size());
    prototypes_.groups.push_back(std::move(gathered));
    groupIndex_.emplace(handle.index, index);
    return index;
}

std::optional<std::uint32_t> Walk::mesh(const scene::Handle & handle) {
    const auto found = geometryIndex_.find(handle.index);
    if (found != geometryIndex_.end()) {
        return found->second;
    }
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
    const auto index = static_cast<std::uint32_t>(prototypes_.geometries.size());
    prototypes_.geometries.push_back(mesh);
    geometryIndex_.emplace(handle.index, index);
    return index;
}

std::uint32_t Walk::cloud(const scene::Handle & handle) {
    const auto found = geometryIndex_.find(handle.index);
    if (found != geometryIndex_.end()) {
        return found->second;
    }
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
    const auto index = static_cast<std::uint32_t>(prototypes_.geometries.size());
    prototypes_.geometries.push_back(cloud);
    geometryIndex_.emplace(handle.index, index);
    return index;
}

std::uint32_t Walk::quadric(const scene::Handle & handle) {
    const auto found = geometryIndex_.find(handle.index);
    if (found != geometryIndex_.end()) {
        return found->second;
    }
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
    const auto index = static_cast<std::uint32_t>(prototypes_.geometries.size());
    prototypes_.geometries.push_back(shape);
    geometryIndex_.emplace(handle.index, index);
    return index;
}

std::optional<std::uint32_t> Walk::subdivision(const scene::Handle & handle) {
    const auto found = geometryIndex_.find(handle.index);
    if (found != geometryIndex_.end()) {
        return found->second;
    }
    const scene::Samples & samples = scene_.samples(handle, "P");
    const std::size_t count = samples.values.front().size();
    std::optional<Surface> surface;
    try {
        surface.emplace(topologyOf(scene_, handle, static_cast<std::uint32_t>(count)));
    } catch (const std::invalid_argument & refused) {
        report("subdivision " + scene_.name(handle) + " is left out, because " + refused.what());
        return std::nullopt;
    }
    const Tessellation tessellation = surface->cut(kSubdivisionRate);
    Geometry mesh;
    mesh.form = kernel::geometry::primitive::Form::Triangles;
    mesh.first = static_cast<std::uint32_t>(prototypes_.faces.size());
    mesh.count = static_cast<std::uint32_t>(tessellation.faces.size());
    mesh.points = sampled(prototypes_.points.size(), tessellation.corners.size(), samples.times);
    // every sample's points are evaluated on the one surface, whose patches its topology fixes
    for (const scene::Value & sample : samples.values) {
        std::vector<float> points;
        points.reserve(count * 3);
        for (std::size_t point = 0; point < count; ++point) {
            const glm::vec3 at = sample.triple(point);
            points.insert(points.end(), { at.x, at.y, at.z });
        }
        const std::vector<float> limit = surface->at(points, 3, scene::Interpolation::Vertex, tessellation);
        for (const std::uint32_t corner : tessellation.corners) {
            const std::size_t at = static_cast<std::size_t>(corner) * 3U;
            prototypes_.points.emplace_back(limit[at], limit[at + 1U], limit[at + 2U]);
        }
    }
    prototypes_.faces.insert(prototypes_.faces.end(), tessellation.faces.begin(), tessellation.faces.end());
    const auto index = static_cast<std::uint32_t>(prototypes_.geometries.size());
    prototypes_.geometries.push_back(mesh);
    geometryIndex_.emplace(handle.index, index);
    return index;
}

std::optional<std::uint32_t> Walk::curves(const scene::Handle & handle) {
    const auto found = geometryIndex_.find(handle.index);
    if (found != geometryIndex_.end()) {
        return found->second;
    }
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
