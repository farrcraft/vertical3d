#ifndef INCLUDED_DOM_DOMSTRING
#define INCLUDED_DOM_DOMSTRING

#include <string>

namespace DOM
{

	typedef std::wstring DOMString;

	// conversions to/from char & wchar_t strings
	std::wstring str2wide(const std::string & str);
	std::string wide2str(const std::wstring & wstr);


}; // end namespace DOM


#endif // INCLUDED_DOM_DOMSTRING
