#ifndef INCLUDED_DOM_COMMENT
#define INCLUDED_DOM_COMMENT

#include "CharacterData.h"

namespace DOM
{

	class Comment : public CharacterData
	{
		friend class Document;

		public:
			Comment();
	};


}; // end namespace DOM

#endif // INCLUDED_DOM_COMMENT
