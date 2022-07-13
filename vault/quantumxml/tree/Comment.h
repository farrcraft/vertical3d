#ifndef INCLUDED_XML_COMMENT
#define INCLUDED_XML_COMMENT

#include <iostream>
#include <fstream>

#include "Misc.h"
#include "../DOM/Comment.h"

namespace XML
{

	class Comment : public Misc, public DOM::Comment
	{
		public:

			void comment(const std::wstring & comm);
			bool sync(std::wofstream & xml_file);
	};

}; // end namespace XML

#endif // INCLUDED_XML_COMMENT
