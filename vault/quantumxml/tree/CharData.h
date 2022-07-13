#ifndef INCLUDED_XML_CHARDATA
#define INCLUDED_XML_CHARDATA

#include "Node.h"

#include "../DOM/Text.h"

namespace XML
{

	class CharData : public Node, public DOM::Text
	{
		public:

			void setData(const std::wstring & str);

			bool sync(std::wofstream & file);
	};

}; // end namespace XML

#endif // INCLUDED_XML_CHARDATA
