#ifndef INCLUDED_XML_NODE
#define INCLUDED_XML_NODE

#include <vector>
#include <iostream>
#include <fstream>

#include "../DOM/Node.h"

namespace XML
{

	class Node : virtual public DOM::Node
	{
		public:
			Node();
			virtual ~Node();

			void addSibling(boost::shared_ptr<Node> node);

			virtual bool sync(std::wofstream & file);
		};

}; // end namespace XML

#endif // INCLUDED_XML_NODE
