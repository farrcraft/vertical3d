/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Component.h"

#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::ui {

    class Container {
     public:
        explicit Container(const std::string& name, bool visible);
        /**
         * Get the name of the container
         * @return name of the container
         **/
        std::string_view name() const;
        /**
         * Get whether the container is visible or not
         * @return true if the container is visible
         */
        bool visible() const;
        /**
         * Set the visibility state of the container
         * @param vis the new visibility setting
         */
        void visible(bool vis);

        void add(const boost::shared_ptr<Component>& component);
        boost::shared_ptr<Component> get(const std::string& name) const;

     private:
        std::string name_;
        std::vector<boost::shared_ptr<Component>> components_;
        bool visible_;
    };

};  // namespace v3d::ui
