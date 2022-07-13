#ifndef INCLUDED_XML_TREE
#define INCLUDED_XML_TREE

#include <list>
#include <set>

#include "tree/Document.h"

/*
These xmlTree classes are used by the parser to store additional document information
Member variables have public access and there are no member functions. These classes are
not meant to be used as a complete hierarchial DOM style document tree. It only needs
to maintain enough information during parsing to call event handlers and do validation.
a DOM based tree can be built by using an appropriate event handler when parsing.
some of these classes may be passed to event handlers.
*/

namespace XML
{


	// the current implementation of elementDecl doesn't work
	class ElementContent
	{
		public:
			typedef enum { SEQ, CHOICE } ContentType;
			typedef enum { ONCE, OPT, MULT, PLUS } ContentOccur;

			class xmlAtom
			{
				public:
					bool			_container;
					ContentOccur	_occur;
					ContentType		_type;
					std::wstring	_name;
					std::list<xmlAtom>	_children;
			};
			xmlAtom				_children;
	};

}; // end namespace XML

#endif // INCLUDED_XML_TREE
