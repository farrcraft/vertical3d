#ifndef INCLUDED_V3D_PROPERTYTREE
#define INCLUDED_V3D_PROPERTYTREE

#include <string>
#include <list>

/**
 * A Tree structured container of property values.
 * The PropertyTree provides a query-able interface for configurable data that allows
 * the application to remain backend agnostic. The tree structure allows flexibility
 * for nested hierarchial formats such as XML to be used.
 */
class PropertyTree
{
	public:
		PropertyTree();
		~PropertyTree();

		typedef PropertyTree value_type;
		typedef std::list<value_type> container_type;
		typedef container_type::iterator iterator;
		typedef container_type::const_iterator const_iterator;

		/**
		 * Load data from a file into the tree.
		 * @param filename the name of the file to load
		 * @return whether the data was successfully loaded
		 */
		bool load(const std::string & filename);
		/**
		 * Set the key and value of this node.
		 * @param key the key name
		 * @param value the value to set
		 */
		void set(const std::string & key, const std::string & value);
		/**
		 * Append a new child node with the key value pair.
		 * @param key the key name of the new child node
		 * @param value the value of the new child node
		 * @return the newly created tree node
		 */
		PropertyTree & insert(const std::string & key, const std::string & value);
		/**
		 * Find a child tree node with the matching key name.
		 * @param key the key name to find in the tree
		 * @return the matching child node or an empty node if there was no match
		 */
		const PropertyTree & find(const std::string & key) const;
		/**
		 * Get the value of the child node with the matching key name.
		 * @param key the child node's key name
		 * @return the matching child node's value or an empty string if no matching node was found
		 */
		std::string get(const std::string & key) const;
		/**
		 * Check if the tree has any of its key, value, or child data set.
		 * @return whether the tree is empty or not
		 */
		bool empty(void) const;
		/**
		 * Get a const iterator to the front of the children node list.
		 * @return a const iterator
		 */
		const_iterator begin(void) const;
		/**
		 * Get a const iterator to the end of the children node list.
		 * @return a const iterator
		 */
		const_iterator end(void) const;
		/**
		 * Get an iterator to the front of the children node list.
		 * @return an iterator
		 */
		iterator begin(void);
		/**
		 * Get an iterator to the end of the children node list.
		 * @return an iterator
		 */
		iterator end(void);
		/**
		 * Get the key name of this node.
		 * @return the key name
		 */
		std::string key(void) const;
		/**
		 * Get the value of this node.
		 * @return the value string
		 */
		std::string value(void) const;
		/**
		 * Print out the contents of the tree.
		 * This is primarily a useful debugging method.
		 * @param depth the default depth spacer string
		 */
		void print(const std::string & depth = "");

	private:
		std::string key_;
		std::string value_;

		container_type children_;
};

#endif // INCLUDED_V3D_PROPERTYTREE
