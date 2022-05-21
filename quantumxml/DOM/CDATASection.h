#ifndef INCLUDED_DOM_CDATASECTION
#define INCLUDED_DOM_CDATASECTION

#include "Text.h"

namespace DOM
{

	class CDATASection : public Text
	{
		friend class Document;

		public:
			CDATASection();

	};

}; // end namespace DOM

#endif // INCLUDED_DOM_CDATASECTION
