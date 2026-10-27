/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>
#include <vertical3d/src/command/CommandStack.h>
#include <vertical3d/src/command/Placement.h>
#include <vertical3d/src/command/VertexCommand.h>
#include <vertical3d/src/manipulator/Manipulator.h>
#include <vertical3d/src/manipulator/Target.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/selection/SelectMask.h>
#include <vertical3d/src/view/ViewPort.h>

#include <cstdint>
#include <string>
#include <vector>

#include "Tool.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>

namespace v3d::editor {

/**
 * Moving, turning and resizing the selection by dragging its handles.
 *
 * Which manipulator is drawn is the tool's mode, and there is a mode in which none is:
 * the selection is then only selectable. The mode is exclusive, because three sets of
 * handles about one origin could not be told apart by a click.
 *
 * What the handles move follows the select mask: the selected objects in object mode, and
 * the vertices of the selected components in a component mode. A gesture on objects is
 * recorded as a TransformCommand, and one on components as a VertexCommand.
 *
 * A press grabs a handle if one is under the cursor and does nothing if none is, so the
 * same button can also select. The controller offers the press here first and passes it
 * on only when no handle took it.
 **/
class TransformTool final : public Tool {
 public:
    /**
     * Which manipulator the selection carries.
     **/
    enum class Mode : std::uint8_t {
        None,
        Translate,
        Rotate,
        Scale
    };

    /**
     * @param scene what is being transformed in, which the controller owns
     **/
    TransformTool(const boost::shared_ptr<Scene>& scene, const boost::shared_ptr<v3d::log::Logger>& logger);

    // tool overrides
    void activate(const std::string& name) override;
    void deactivate(const std::string& name) override;
    void motion(const glm::vec2& position) override;
    void button(unsigned int button, bool pressed, const glm::vec2& position) override;

    /**
     * Where a finished gesture is recorded. A tool with no stack still transforms, but
     * nothing it does can then be undone.
     **/
    void commands(const boost::shared_ptr<CommandStack>& commands);

    /**
     * The view a drag is measured in, set as the cursor moves between viewports.
     **/
    void view(const boost::shared_ptr<ViewPort>& view) override;

    /**
     * @return the view the tool is transforming in, which may be null
     **/
    boost::shared_ptr<ViewPort> view() const;

    /**
     * @return which manipulator is in force
     **/
    Mode mode() const noexcept;

    /**
     * Choose the manipulator. Changing it drops any drag under way, since the handle
     * being dragged is about to stop existing.
     **/
    void mode(Mode mode);

    /**
     * @return the manipulator the selection carries, or an empty pointer in Mode::None
     **/
    boost::shared_ptr<Manipulator> manipulator() const;

    /**
     * Follow the select mask. Changing it ends any drag under way, and the change made so far
     * is recorded.
     **/
    void mask(SelectMask mask);

    /**
     * @return what the handles move: the set a drag grabbed while one is under way, and
     *         what the selection makes under the mask otherwise
     **/
    Target target() const;

    /**
     * @return whether a handle is being dragged. While it is, the same press does not
     *         also select
     **/
    bool dragging() const noexcept override;

    /**
     * @return whether a handle of the target is under the point
     **/
    bool takes(const glm::vec2& position) const override;

    /**
     * End a drag without recording it, as replacing the scene does. The mesh the gesture
     * holds is leaving the document, so a command would have nothing to be undone
     * against. The mode is kept.
     **/
    void cancel();

 private:
    /**
     * Record the gesture that has just ended, if it moved anything. Every path out of a
     * drag, a release or a mode change, comes through here. A change that reached the mesh
     * but not the stack could not be undone.
     **/
    void commit();

    /**
     * Highlight whatever handle the cursor is over, so the user can see which handle a
     * press would grab.
     **/
    void hover(const glm::vec2& position);

    boost::shared_ptr<Scene> scene_;
    boost::shared_ptr<v3d::log::Logger> logger_;
    boost::shared_ptr<CommandStack> commands_;
    boost::shared_ptr<ViewPort> view_;
    boost::shared_ptr<Manipulator> translate_;
    boost::shared_ptr<Manipulator> rotate_;
    boost::shared_ptr<Manipulator> scale_;
    Mode mode_;
    bool dragging_;
    glm::vec2 last_;
    SelectMask mask_;
    Target dragged_;
    std::vector<v3d::type::Transform> before_;
    std::vector<VertexCommand::Shape> shapes_;
};

/**
 * @param name one of "select", "translate", "rotate" or "scale"
 * @param mode set to the mode named, and untouched if the name is not one of them
 * @return whether the name named a mode
 **/
bool transformMode(const std::string& name, TransformTool::Mode* mode);

};  // namespace v3d::editor
