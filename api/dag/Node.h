/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

namespace v3d::dag {

/**
 * Something with an identity: every node constructed gets an id no other has had, and the
 * editor keys its scene and its selection on that id. A copy would share the id it was
 * copied from, so a node cannot be copied.
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
