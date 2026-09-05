/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "RenderContext.h"

#include <string>
#include <fstream>

namespace v3d::talyn {

    class RIBReader {
     public:
        explicit RIBReader(const boost::shared_ptr<RenderContext> & rc);

        bool read(const std::string & filename);


     protected:
        void parseToken(const std::string & token);
        std::string readToken();

     private:
        boost::shared_ptr<RenderContext> rc_;
        std::ifstream file_;
    };

};  // namespace v3d::talyn
