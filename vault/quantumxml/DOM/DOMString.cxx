#include "DOMString.h"

#include <iostream>

using namespace DOM;


// conversions to/from char & wchar_t strings
std::wstring DOM::str2wide(const std::string & str)
{
#ifdef _MSC_VER
//	cout << "DOM: converting: " << str << endl;
	if (str.length() == 0)
		return L"";
	wchar_t * wbuf;
	const char * buf = str.c_str();
	std::string::size_type len = str.length();
	mbstate_t pstate;
	wbuf = new wchar_t [len + 1];
	size_t conv_count;
	mbsrtowcs_s(&conv_count, wbuf, len, &buf, len, &pstate);
	std::wstring wstr(wbuf);
	delete [] wbuf;
//	wcout << L"converted: " << wstr << endl;
	return wstr;
#else
	// mbstrtowcs_s is MSC only
	std::wstring wstr;
	wstr.assign(str.begin(), str.end());
	return wstr;
#endif
}

std::string DOM::wide2str(const std::wstring & wstr)
{
#ifdef _MSC_VER
//	std::wcout << L"converting: " << wstr << std::endl;
	if (wstr.length() == 0)
		return "";
	size_t len = wstr.length() + 1;
	size_t converted = 0;
	char * buf = new char [len];
	wcstombs_s(&converted, buf, len, wstr.c_str(), _TRUNCATE);
	std::string str(buf);
	delete [] buf;
//	std::cout << "converted: " << str << std::endl;
	return str;
#else
	// wcstombs_s is MSC only
	std::string str;
	str.assign(wstr.begin(), wstr.end());
	return str;
#endif
}
