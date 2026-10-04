/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <optional>
#include <string>
#include <vector>

#include "DrawItem.h"

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime {

namespace vulkan::frame {
class RenderTarget;
};  // namespace vulkan::frame

/**
 * One pass of a frame - a target, what to do with what is already in it, a camera, and
 * the draw items to record into it.
 *
 * The pass is the unit of variation between 2D and 3D drawing rather than the engine
 * being, per ADR-0003: a sprite pass is one with no depth buffer and painter ordering,
 * a scene pass is one with depth and front to back ordering, and a viewport of an editor
 * is one more pass over the same device.
 *
 * A pass draws into the swapchain image unless it names one of its own - see target()
 * and ADR-0031.
 **/
class Pass final {
 public:
    /**
     * What vkCmdSetDepthBias is given, in its terms: a constant offset in units of the
     * depth format's smallest step, one scaled by the polygon's slope, and the most either
     * may add up to - zero for no limit.
     **/
    struct DepthBias final {
        float constant;
        float slope;
        float clamp;
    };

    /**
     * @param name what the pass is for, used in logs and debug markers
     **/
    explicit Pass(const std::string& name);

    /**
     * @return the name the pass was created with
     **/
    const std::string& name() const noexcept;

    /**
     * Clear the target to a colour before anything in the pass draws.
     * A pass that does not clear draws over whatever the pass before it left, which
     * means the first pass of a frame should always clear.
     **/
    void clearColour(const glm::vec4& colour) noexcept;

    /**
     * Leave whatever is in the target and draw over it.
     **/
    void keepColour() noexcept;

    /**
     * @return whether the pass clears its target before drawing
     **/
    bool clears() const noexcept;

    /**
     * @return the colour the target is cleared to
     **/
    const glm::vec4& clearColour() const noexcept;

    /**
     * Whether the pass depth tests. 2D passes do not - they rely on painter ordering.
     *
     * A pass that asks for depth is given one the size of what it draws into: the
     * context's depth buffer for a pass on the swapchain, allocated the first frame
     * anything asks for it, and the target's own for a pass with a target. Depth is
     * cleared exactly when colour is, so a pass drawing on top of what the pass before
     * it left keeps that pass's depth too.
     **/
    void depth(bool enabled) noexcept;

    /**
     * @return whether the pass depth tests
     **/
    bool depth() const noexcept;

    /**
     * Draw into an offscreen target rather than into the swapchain image.
     *
     * The recorder leaves a target readable by every pass after the last one that drew
     * into it, so a pass sampling what an earlier pass rendered names that target's
     * texture as a material like any other - ADR-0031 - and says so with reads().
     *
     * A pipeline is built against the format of what it draws into, so a pass whose target
     * is not the swapchain's format needs a pipeline built for that format. The recorder
     * throws when one is drawn into the other - ADR-0068.
     *
     * Passing an empty pointer puts the pass back on the swapchain image.
     **/
    void target(const boost::shared_ptr<vulkan::frame::RenderTarget>& target) noexcept;

    /**
     * @return the target the pass draws into, or an empty pointer for the swapchain image
     **/
    const boost::shared_ptr<vulkan::frame::RenderTarget>& target() const noexcept;

    /**
     * Say that this pass samples what another pass drew into a target, so the frame records
     * every pass drawing into it first - ADR-0068. Naming the target the pass draws into
     * itself orders nothing, which is what a pass reading its own previous() frame does.
     *
     * Kept across reset(), as the target is: it is what the pass is, not what it drew.
     **/
    void reads(const boost::shared_ptr<vulkan::frame::RenderTarget>& target);

    /**
     * @return the targets this pass samples, in the order they were named
     **/
    const std::vector<boost::shared_ptr<vulkan::frame::RenderTarget>>& reads() const noexcept;

    /**
     * The region of the target the pass draws into, as x, y, width, height in pixels.
     * A width or height of zero means the whole target, which is the default and what
     * every pass wants until an editor draws four viewports of one scene.
     **/
    void viewport(const glm::vec4& region) noexcept;

    /**
     * @return the region of the target the pass draws into
     **/
    const glm::vec4& viewport() const noexcept;

    /**
     * The camera every item in the pass draws through, bound once at set 0 per
     * ADR-0008 rather than pushed per draw.
     *
     * Both default to the identity, which is what a 2D pass wants: a canvas carries its
     * own orthographic projection in a push constant and nothing reads set 0.
     **/
    void camera(const glm::mat4& view, const glm::mat4& projection) noexcept;

    /**
     * @return the world to view transform the pass draws through
     **/
    const glm::mat4& view() const noexcept;

    /**
     * @return the view to clip transform the pass draws through
     **/
    const glm::mat4& projection() const noexcept;

    /**
     * What the pass binds at set 2 for every item whose pipeline declares one - the light,
     * the shadow map and whatever else a lit scene shares across a pass - per ADR-0064.
     *
     * Bound once for the pass, like the camera, and only for pipelines that declare a set
     * 2, so a quad drawn in the same pass binds nothing extra. An item whose pipeline
     * declares one, in a pass that names none, is an error at record time.
     *
     * @param set a set allocated against the layout those pipelines declare at 2, or null
     *        for none
     **/
    void scene(VkDescriptorSet set) noexcept;

    /**
     * @return what the pass binds at set 2, or null
     **/
    VkDescriptorSet scene() const noexcept;

    /**
     * The depth bias every pipeline built with one draws at in this pass - ADR-0064.
     *
     * A bias is the pass's rather than the item's, because what decides it is the target's
     * depth format and the light's angle, which every caster in a shadow pass shares. A
     * pipeline built without one ignores it, so other geometry can share the pass. An item
     * whose pipeline was built with one, in a pass that names none, is an error at record
     * time: vulkan would otherwise draw with whatever bias was last set, and say nothing.
     **/
    void depthBias(float constant, float slope, float clamp = 0.0f) noexcept;

    /**
     * @return the bias the pass draws at, or nothing when it names none
     **/
    const std::optional<DepthBias>& depthBias() const noexcept;

    /**
     * Record the pass's items in sort key order rather than in submission order.
     *
     * Off by default, and it has to be: 2D content is painter ordered, and the key sorts
     * by pipeline and material within a layer, so sorting a canvas's batches would put a
     * panel over the text on it. A depth tested scene pass is the case this is for - it
     * has one item per object, and grouping them by pipeline and material is what lets
     * the recorder skip rebinding between them.
     *
     * The sort is stable, so items whose keys are equal keep the order they arrived in.
     **/
    void sort(bool enabled) noexcept;  // NOLINT(build/include_what_you_use) - the name, not std::sort

    /**
     * @return whether the pass is recorded in sort key order
     **/
    bool sorts() const noexcept;

    /**
     * Add a draw item to the pass. The engine decides when it is recorded.
     **/
    void submit(const DrawItem& item);

    /**
     * @return the items submitted to the pass, in submission order
     **/
    const std::vector<DrawItem>& items() const noexcept;

    /**
     * The items in the order the engine records them - submission order, or sort key
     * order when the pass sorts.
     *
     * They come back as pointers because a draw item carries its push constant block by
     * value, so sorting the queue itself would move a hundred-odd bytes per swap. The
     * pointers are into the pass's own queue and stay valid until the next submit() or
     * reset().
     *
     * @param into cleared and filled with the items
     **/
    void ordered(std::vector<const DrawItem*>* into) const;

    /**
     * Drop the submitted items, keeping the pass's configuration. Called at the end of
     * a frame, so the next one starts from an empty queue without reallocating.
     **/
    void reset() noexcept;

 private:
    std::string name_;
    glm::vec4 clearColour_;
    glm::vec4 viewport_;
    glm::mat4 view_;
    glm::mat4 projection_;
    std::vector<DrawItem> items_;
    boost::shared_ptr<vulkan::frame::RenderTarget> target_;
    std::vector<boost::shared_ptr<vulkan::frame::RenderTarget>> reads_;
    VkDescriptorSet scene_;
    std::optional<DepthBias> bias_;
    bool clears_;
    bool depth_;
    bool sorts_;
};

};  // namespace v3d::render::realtime
