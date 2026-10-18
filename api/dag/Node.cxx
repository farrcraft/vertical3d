/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#include "Node.h"

namespace v3d::dag {

namespace {

unsigned int baseNodeID = 0;

};  // namespace

Node::Node() {
    _id = ++baseNodeID;
}

Node::~Node() {
}

unsigned int Node::id(void) const {
    return _id;
}

unsigned int Node::baseID(void) {
    return baseNodeID;
}

};  // namespace v3d::dag
