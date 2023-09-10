/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <vector>

namespace v3d::dag {

    class Node;

    class Root {
     public:
        Root();
        virtual ~Root();

     private:
        std::vector<Node *> children_;
    };

};  // namespace v3d::dag
