#ifndef INCLUDED_XML_MISC
#define INCLUDED_XML_MISC

#include "Node.h"

namespace XML
{

	// represents either a comment or processing instruction
	class Misc : public Node
	{
		public:
			typedef enum
			{
				Prolog,// after xmldecl before dtd
				Head,  // right after dtd
				Body,  // within element content
				Epilog // after element
			} MiscPosition;

			void position(MiscPosition mp);
			MiscPosition position(void) const;

		private:
			MiscPosition	_position;
	};

}; // end namespace XML

#endif // INCLUDED_XML_MISC
