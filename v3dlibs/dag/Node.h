/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

namespace v3d::dag {

    class Node {
     public:
        Node();
        virtual ~Node();

        unsigned int id(void) const;

        static unsigned int baseID(void);

     private:
        unsigned int _id;
    };

};  // namespace v3d::dag
