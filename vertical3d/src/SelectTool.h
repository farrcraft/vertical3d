/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "Picker.h"
#include "Scene.h"
#include "SelectMask.h"
#include "Tool.h"
#include "ViewPort.h"

#include "../../api/log/Logger.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>

namespace v3d::editor {

    /**
     * Selecting by clicking.
     *
     * A press casts a pick into the view the cursor is over and writes what it found into
     * the scene's selection flags. The mask lives here rather than on a view, because it
     * decides what a click looks for and not what a view draws.
     *
     * Missing in object mode clears the whole selection; missing in a component mode
     * clears the components and leaves the object selected, so a stray click does not
     * throw away the object the user is working inside. Clicking the same component twice
     * deselects it, and one component of a kind is selected at a time.
     **/
    class SelectTool final : public Tool {
     public:
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
        void view(const boost::shared_ptr<ViewPort>& view);

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
         * @return what the last click found
         **/
        const Picker::Hit& hit() const noexcept;

     private:
        /**
         * Write a pick into the scene's selection flags.
         **/
        void apply(const Picker::Hit& hit);

        /**
         * Select one mesh and nothing else.
         * @param id the node id, or zero for none - no mesh has id zero, the counter
         *        starts above it
         **/
        void selectObject(unsigned int id);

        /**
         * Toggle one component of a mesh, clearing every other component of it.
         **/
        void selectComponent(const Picker::Hit& hit);

        boost::shared_ptr<Scene> scene_;
        boost::shared_ptr<v3d::log::Logger> logger_;
        boost::shared_ptr<ViewPort> view_;
        Picker picker_;
        SelectMask mask_;
        Picker::Hit hit_;
        glm::vec2 cursor_;
    };

};  // namespace v3d::editor
