/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include "Transform.h"

#include <vector>

namespace v3d::dag {

      class Node;

        class Group {
         public:
            Group();
            virtual ~Group();

         private:
            Transform transform_;
            std::vector<Node *> children_;
        };

};  // namespace v3d::dag
