/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

namespace v3D {

    class Visitor {
     public:
            Visitor();
            virtual ~Visitor();

            // virtual void visit(Particle * particle) = 0;
    };

};  // end namespace v3D

