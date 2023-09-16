/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/
#pragma once

#include <map>
#include <string>
#include <string_view>
#include <variant>

#include "Asset.h"
#include "Type.h"
#include "../log/Logger.h"

#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::asset {
    class Manager;

    using ParameterValue = std::variant<unsigned int, float, std::string>;

    /**
     * The base interface for asset loaders
     **/
    class Loader {
     public:
        /**
         * Get the asset type
         * 
         * @return Type
         **/
        Type type() const;

        /**
         * Reset any intermediate loader state
         * Some loaders might require multiple calls to the loader for a single load operation.
         * Use reset to clear any state used to support any individual load operation.
        **/
        void reset();

        /**
         * Load an asset
         * 
         * @param name The name of the asset to be loaded
         **/
        virtual boost::shared_ptr<Asset> load(std::string_view name) = 0;

        /**
         * Set a loader-specific parameter
         * If a parameter with the same name already exists, it will be replaced with the new parameter.
         * 
         * @param name The name of the parameter to add
         * @param value The parameter value
         **/
        void parameter(const std::string &name, const ParameterValue &value);

        /**
         * Fetch a parameter
         **/
        boost::optional<ParameterValue> parameter(std::string_view name);

     protected:
        /**
         * Default constructor
         *
         * @param Type t The asset type this loader provides
         **/
        Loader(Manager* manager, Type t, const boost::shared_ptr<v3d::log::Logger>& logger);

     protected:
        boost::shared_ptr<v3d::log::Logger> logger_;
        Manager* manager_;
        std::map<std::string, ParameterValue> parameters_;

     private:
        Type type_;
    };
};  // namespace v3d::asset
