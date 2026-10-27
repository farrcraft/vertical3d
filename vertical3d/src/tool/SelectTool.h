/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/view/ViewPort.h>

#include <optional>
#include <string>

#include "Picker.h"
#include "SelectMask.h"
#include "Tool.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>

namespace v3d::editor {

/**
 * Selecting by clicking, and by dragging a rubber band.
 *
 * A press casts a pick into the view the cursor is over and writes what it found into
 * the scene's selection flags. The mask lives here rather than on a view, because it
 * decides what a click looks for and not what a view draws.
 *
 * A plain click replaces the selection with what it hit. A miss in object mode clears the
 * whole selection. A miss in a component mode clears the components and leaves the objects
 * selected, so a stray click does not lose the objects the user is working inside. A plain
 * click on a selected component deselects it along with the rest.
 *
 * While adding, a click toggles what it hit and leaves the rest of the selection alone, and
 * a miss changes nothing. Components are picked only inside selected objects, so a component
 * selection may span every selected object.
 *
 * A press on nothing is applied as a miss at once, and then held. If the cursor moves further
 * than the click tolerance before the release, the drag is a rubber band, and the release
 * selects everything inside it. A band adds to what the miss left, so it replaces the
 * selection unless adding.
 **/
class SelectTool final : public Tool {
 public:
    /**
     * How far the cursor may move, in pixels along either axis, between a press on nothing
     * and its release while still counting as a click rather than a rubber band.
     **/
    static constexpr float clickTolerance = 4.0F;

    /**
     * A rubber band, in window pixels. Either corner may be the smaller.
     **/
    struct Band final {
        glm::vec2 corner;
        glm::vec2 opposite;
    };

    /**
     * @param scene what is being selected in, which the controller owns
     **/
    SelectTool(const boost::shared_ptr<Scene>& scene, const boost::shared_ptr<v3d::log::Logger>& logger);

    // tool overrides
    void activate(const std::string& name) override;
    void deactivate(const std::string& name) override;
    void motion(const glm::vec2& position) override;
    void button(unsigned int button, bool pressed, const glm::vec2& position) override;

    /**
     * The view a click will be cast into, set as the cursor moves between viewports.
     **/
    void view(const boost::shared_ptr<ViewPort>& view) override;

    /**
     * @return the view the tool is picking in, which may be null
     **/
    boost::shared_ptr<ViewPort> view() const;

    /**
     * @return what a click is looking for
     **/
    SelectMask mask() const noexcept;

    /**
     * What a click looks for. Changing it clears the component selection, because the
     * components of one kind mean nothing to an operation working in another.
     **/
    void mask(SelectMask mask);

    /**
     * Whether a click adds what it finds to the selection, or takes it away if it is already
     * selected, rather than replacing the selection. It follows a held key.
     **/
    void adding(bool adding) noexcept;
    bool adding() const noexcept;

    /**
     * @return what the last click found
     **/
    const Picker::Hit& hit() const noexcept;

    /**
     * @return whether a press on nothing is held. The view it was made in stays the view
     *         the release is measured in
     **/
    bool dragging() const noexcept override;

    /**
     * @return true: a press no other tool takes picks
     **/
    bool takes(const glm::vec2& position) const override;

    /**
     * @return the rubber band being dragged, or nothing unless a held press has moved far
     *         enough to be a band
     **/
    std::optional<Band> band() const;

 private:
    /**
     * Write a pick into the scene's selection flags.
     **/
    void apply(const Picker::Hit& hit);

    /**
     * Select one mesh, replacing the selection or, while adding, toggling that mesh alone.
     * @param id the node id, or zero for none. No mesh has id zero, because the id
     *        counter starts above it
     **/
    void selectObject(unsigned int id);

    /**
     * Toggle one component of a mesh. Unless adding, every other component of every mesh is
     * cleared first.
     **/
    void selectComponent(const Picker::Hit& hit);

    /**
     * Select everything inside the rubber band.
     **/
    void selectInside();

    boost::shared_ptr<Scene> scene_;
    boost::shared_ptr<v3d::log::Logger> logger_;
    boost::shared_ptr<ViewPort> view_;
    Picker picker_;
    SelectMask mask_;
    bool adding_;
    Picker::Hit hit_;
    glm::vec2 cursor_;
    bool held_;
    glm::vec2 anchor_;
};

};  // namespace v3d::editor
