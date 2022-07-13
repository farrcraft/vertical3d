#ifndef INCLUDED_DOM_ATTR
#define INCLUDED_DOM_ATTR

#include "Node.h"

#include <boost/weak_ptr.hpp>

namespace DOM
{

	class Element;

	class Attr : public Node
	{
		friend class Element;
		friend class Document;

		public:
			Attr();

			DOMString name(void) const;
			bool specified(void) const;
			DOMString value(void);
			// Introduced in DOM Level 2:
			boost::shared_ptr<Element> ownerElement(void) const;

		protected:
			// nodename contains name
			void name(const DOMString & str);
			// nodevalue contains value
			void value(const DOMString & str);
			void specified(bool spec);
			void ownerElement(boost::weak_ptr<Element> elem);

		private:
			boost::weak_ptr<Element> _ownerElement;
			bool _specified;
	};


}; // end namespace DOM

#endif // INCLUDED_DOM_ATTR
