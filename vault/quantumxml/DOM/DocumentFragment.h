#ifndef INCLUDED_DOM_DOCUMENTFRAGMENT
#define INCLUDED_DOM_DOCUMENTFRAGMENT

#include "Node.h"

namespace DOM
{

	class DocumentFragment : public Node
	{
		friend class Document;

		public:
			DocumentFragment();
	};

}; // end namespace DOM

#endif // INCLUDED_DOM_DOCUMENTFRAGMENT
