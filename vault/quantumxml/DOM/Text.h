#ifndef INCLUDED_DOM_TEXT
#define INCLUDED_DOM_TEXT

#include "CharacterData.h"

namespace DOM
{

	class Text : public CharacterData
	{
		friend class Document;

		public:
			Text();

			Text * splitText(unsigned long offset);
	};

}; // end namespace DOM

#endif // INCLUDED_DOM_TEXT
