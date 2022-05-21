/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "PropertyTree.h"

#include "../../quantumxml/Parser.h"

/**
 * A class for populating a PropertyTree from an XML file backend.
 */
class PropertyTreeReader {
	public:

		/**
		 * Load the file contents into the tree.
		 * The root document element in the XML document will correspond to a child in the 
		 * provided tree. This allows multiple documents to be stored within the same tree
		 * (as long as the root document element has a different name.) Individual attributes
		 * of an element are stored as the children nodes of a special node with a key name
		 * of \<xmlattr>. This node is a direct child of the corresponding element tree node.
		 * The value of the "name" attribute of the root element "config" can be queried as:
		 * tree->get("config.<xmlattr>.name").
		 * @param filename the name of the file to load
		 * @param tree a pointer to the tree to be populated
		 * @return whether the file contents were successfully loaded into the tree.
		 */
		bool read(const std::string & filename, PropertyTree * tree);

	protected:
		void read(PropertyTree & sections, DOM::Node::ptr node);
		void read_attributes(PropertyTree & tree, DOM::Node::ptr node);
};
