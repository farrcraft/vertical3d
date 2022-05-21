#ifndef INCLUDED_DOM_NODELIST
#define INCLUDED_DOM_NODELIST

#include <vector>
#include <boost/shared_ptr.hpp>

namespace DOM
{

	class Node;

	class NodeList
	{
		friend class Document;

		public:
			boost::shared_ptr<Node> item(unsigned long index) const;
			unsigned long length(void) const;

		protected:
			void push_back(boost::shared_ptr<Node> node);

		private:
			std::vector< boost::shared_ptr<Node> > _nodes;
	};

}; // end namespace DOM

#endif // INCLUDED_DOM_NODELIST
