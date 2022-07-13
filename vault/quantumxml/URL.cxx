#include "URL.h"

#include <ctype.h>
// for atoi
#include <stdlib.h>

using namespace XML;

/*
	URL         = ( absoluteURL | relativeURL ) [ "#" fragment ]

	absoluteURL = generic-RL | ( scheme ":" *( uchar | reserved ) )

	generic-RL  = scheme ":" relativeURL

	relativeURL = net_path | abs_path | rel_path

	net_path    = "//" net_loc [ abs_path ]
	abs_path    = "/"  rel_path
	rel_path    = [ path ] [ ";" params ] [ "?" query ]

	path        = fsegment *( "/" segment )
	fsegment    = 1*pchar
	segment     =  *pchar

	params      = param *( ";" param )
	param       = *( pchar | "/" )

	scheme      = 1*( alpha | digit | "+" | "-" | "." )
	net_loc     =  *( pchar | ";" | "?" )
	query       =  *( uchar | reserved )
	fragment    =  *( uchar | reserved )

	pchar       = uchar | ":" | "@" | "&" | "="
	uchar       = unreserved | escape
	unreserved  = alpha | digit | safe | extra

	escape      = "%" hex hex
	hex         = digit | "A" | "B" | "C" | "D" | "E" | "F" |
							"a" | "b" | "c" | "d" | "e" | "f"

	alpha       = lowalpha | hialpha
	lowalpha    = "a" | "b" | "c" | "d" | "e" | "f" | "g" | "h" | "i" |
					"j" | "k" | "l" | "m" | "n" | "o" | "p" | "q" | "r" |
					"s" | "t" | "u" | "v" | "w" | "x" | "y" | "z"
	hialpha     = "A" | "B" | "C" | "D" | "E" | "F" | "G" | "H" | "I" |
					"J" | "K" | "L" | "M" | "N" | "O" | "P" | "Q" | "R" |
					"S" | "T" | "U" | "V" | "W" | "X" | "Y" | "Z"

	digit       = "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" |
					"8" | "9"

	safe        = "$" | "-" | "_" | "." | "+"
	extra       = "!" | "*" | "'" | "(" | ")" | ","
	national    = "{" | "}" | "|" | "\" | "^" | "~" | "[" | "]" | "`"
	reserved    = ";" | "/" | "?" | ":" | "@" | "&" | "="
	punctuation = "<" | ">" | "#" | "%" | <">


scheme ":" schemepart

http://<host>:<port>/<path>?<searchpart>

httpurl = "http://" hostport [ "/" hpath [ "?" search ]]
hostport = host [ ":" port ]
host = hostname | hostnumber
hostname = *[domainlabel "." ] toplabel
hpath = hsegment *[ "/" hsegment ]
hsegment = *[ uchar | ";" | ":" | "@" | "&" | "=" ]
search = *[ uchar | ";" | ":" | "@" | "&" | "=" ]

predefined schemes:
"ftp://" login [ "/" fpath [ ";type=" ftptype ]]
fpath = fsegment *[ "/" fsegment ]
fsegment *[ uchar | "?" | ":" | "@" | "&" | "=" ]
"file://" [ host | "localhost" ] "/" fpath
"http://" hostport [ "/" hpath [ "?" search ]]
"gopher://" hostport [ "/" [ gtype [ selector
["%09" search [ "%09" gopher+_string ] ] ] ] ]
"mailto:" encoded822addr
"news:" grouppart
"nntp://" hostport "/" group [ "/" digits ]
"telnet://" login [ "/" ]
"wais://" hostport "/" database [ ( "?" search ) | ( "/" wtype "/" wpath ) ]
"prospero://" hostport "/" ppath *[ fieldspec ]

see RFC 1630, 1738, 1808
http://www.ietf.org/rfc/rfc1738.txt?number=1738		Uniform Resource Locators (URL)
http://www.ietf.org/rfc/rfc1808.txt?number=1808		Relative Uniform Resource Locators
http://www.ietf.org/rfc/rfc1630.txt?number=1630		Universal Resource Identifiers in WWW
*/

URL::URL ()
{
}

URL::URL (const URL & url) : _url(url._url)
{
	parse();
}

URL::URL (const std::string & url) : _url(url)
{
	parse();
}

URL::~URL ()
{
}

URL & URL::operator = (const std::string & url)
{
	_url = url;
	parse();
	return * this;
}

bool URL::parse(void)
{
	// parse url into the components:
	// <scheme>://<net_loc>/<path>;<params>?<query>#<fragment>

	std::string parse_string = _url;

	// remove fragment id
	std::string::size_type pos = parse_string.find("#", 0);
	if (pos != std::string::npos)
		parse_string.erase(pos, parse_string.length() - pos);

	// get scheme
	for (unsigned int i = 0; i < parse_string.length(); i++)
	{
		// found end of scheme
		if (parse_string[i] == ':')
		{
			_scheme = parse_string.substr(0, i);
			// remove scheme + ':'
			parse_string.erase(0, i + 1);
			break;
		}
		if (!isalpha(parse_string[i]) && !isdigit(parse_string[i]) && 
			!(parse_string[i] == '+') && !(parse_string[i] == '-') && !(parse_string[i] == '.'))
			break;
	}
	// get net location
	if ((parse_string.length() >= 2) && (parse_string.substr(0, 2) == std::string("//")) &&
		(_scheme.length() != 0)) // ignore location if no scheme (allows local file paths)
	{
		if ((parse_string.length() >= 3) && (parse_string[2] != '/'))
		{
			for (unsigned int j = 2; j < parse_string.length(); j++)
			{
				// found end of net location
				if (parse_string[j] == '/')
				{
					_netLocation = parse_string.substr(2, j - 2);
					// remove '//' + net location
					parse_string.erase(0, j); // j+1 would erase the '/' - leave it with the path instead
					break;
				}
			}
			if (_netLocation.length() == 0)
			{
				_netLocation = parse_string.substr(2, parse_string.length() - 2);
				parse_string.erase(0, parse_string.length());
			}
		}
		else
		{
			// no net location, just erase '//'
			parse_string.erase(0, 2);
		}
	}
	// get query
	for (unsigned int k = 0; k < parse_string.length(); k++)
	{
		if (parse_string[k] == '?')
		{
			_query = parse_string.substr(k + 1, parse_string.length() - k);
			parse_string.erase(k, parse_string.length() - k + 1);
			break;
		}
	}
	// get params
	for (unsigned int m = 0; m < parse_string.length(); m++)
	{
		if (parse_string[m] == ';')
		{
			_params = parse_string.substr(m + 1, parse_string.length() - m);
			parse_string.erase(m, parse_string.length() - m + 1);
			break;
		}
	}

	// if no scheme is set then default to local relative filepath
	if (_scheme.length() == 0)
		_scheme = "file";

	_path = parse_string;

	return true;
}

std::string URL::getURL(void) const
{
	return _url;
}

std::string URL::getScheme(void) const
{
	return _scheme;
}

std::string URL::getParams(void) const
{
	return _params;
}

std::string URL::getNetLocation(void) const
{
	return _netLocation;
}

std::string URL::getQuery(void) const
{
	return _query;
}

std::string URL::getPath(void) const
{
	return _path;
}

std::string URL::getHost(void) const
{
	std::string::const_iterator it = _netLocation.begin();
	std::string host;
	// host, port
	std::string::const_iterator beg = it;
	// the allowed host name or number has a few more constraints than this..
	while ((it != _netLocation.end()) && (isalnum(*it) || (*it == '.'))) 
		it++;
	if (it == beg)
		return host;
	host = _netLocation.substr(beg - _netLocation.begin(), it - beg);
	return host;
}

short URL::getPort(void) const
{
	short port = -1;
	unsigned int start = 0;
	for (unsigned int i = 0; i < _netLocation.length(); i++)
	{
		if (_netLocation[i] == ':')
		{
			std::string port_str = _netLocation.substr(i+1, (_netLocation.length() - (i+1)));
			return atoi(port_str.c_str());
		}
	}
	return port;
}

// unreserved = alpha | digit | safe | extra
// safe = "$" | "-" | " " | "." | "+"
// extra = "!" | "*" | "'" | "(" | ")" | ","
inline bool isUnreserved(char c)
{
	if (isalpha(c) || isdigit(c) || 
	(c == '$') || (c == '-') || (c == ' ') || (c == '.') || (c == '+') ||
	(c == '!') || (c == '*') || (c == '\'') || (c == '(') || (c == ')') || (c == ','))
		return true;
	return false;
}

// uchar = unreserved | escape
// escape = "%" hex hex
// hex = digit | [A-F] | [a-f]
// isxdigit()
inline bool isUChar(char c)
{
	if (isUnreserved(c) || (c == '%'))
		return true;
	return false;
}


