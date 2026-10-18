/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

namespace v3d::dag {

/**
 * An object with a unique id. Each node constructed gets an id that no earlier node had. The
 * editor's scene and selection are keyed on that id. A copy would share the id of the node it
 * was copied from, so a node cannot be copied.
 **/
class Node {
 public:
    Node();
    virtual ~Node();
    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;

    unsigned int id(void) const;

    static unsigned int baseID(void);

 private:
    unsigned int _id;
};

};  // namespace v3d::dag
