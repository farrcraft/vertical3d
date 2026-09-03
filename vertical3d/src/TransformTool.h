/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "CommandStack.h"
#include "Manipulator.h"
#include "Scene.h"
#include "Tool.h"
#include "TransformCommand.h"
#include "ViewPort.h"

#include "../../api/log/Logger.h"

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
     * A press grabs a handle if one is under the cursor and does nothing if none is, which
     * is what lets the same button also select - the controller offers the press here first
     * and passes it on only when no handle took it.
     **/
    class TransformTool final : public Tool {
     public:
        /**
         * Which manipulator the selection carries.
         **/
        enum class Mode {
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
         * Where a finished gesture is recorded. A tool with no stack still transforms;
         * nothing it does can then be undone.
         **/
        void commands(const boost::shared_ptr<CommandStack>& commands);

        /**
         * The view a drag is measured in, set as the cursor moves between viewports.
         **/
        void view(const boost::shared_ptr<ViewPort>& view);

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
         * @return whether a handle is being dragged, which is what stops the same press
         *         from also selecting
         **/
        bool dragging() const noexcept;

        /**
         * End a drag without recording it, which is what replacing the scene does: the
         * mesh the gesture is holding is leaving the document, so there is nothing left
         * for a command to be undone against. The mode is kept.
         **/
        void cancel();

     private:
        /**
         * Record the gesture that has just ended, if it moved anything. Every path out of a
         * drag comes through here - a release and a mode change alike - because a change
         * that reached the mesh but not the stack is one undo cannot reach.
         **/
        void commit();

        /**
         * Highlight whatever handle the cursor is over, so that a handle says it can be
         * grabbed before it is.
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
        boost::shared_ptr<v3d::brep::BRep> dragged_;
        Placement before_;
    };

    /**
     * @param name one of "select", "translate", "rotate" or "scale"
     * @param mode where the answer goes - untouched if the name is not one of them
     * @return whether the name named a mode
     **/
    bool transformMode(const std::string& name, TransformTool::Mode* mode);

};  // namespace v3d::editor
