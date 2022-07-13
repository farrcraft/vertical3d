#include "Stream.h"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include <iostream>
#include <fstream>
#include <iterator>
#include <locale>

using namespace XML;

Stream::Stream() : _position(0), _pushMark(0), _pushColumn(0),
	_pushLine(0), _mark(0), _column(0), _line(0)
{
}

Stream::Stream(const std::wstring & buf) : _pushMark(0), _pushColumn(0),
	_pushLine(0), _mark(0)
{
	setBuffer(buf);
}

Stream::~Stream()
{
}

int Stream::isSingleDoubleQuote(void)
{
	return ((_current == '"') ? 1 : (_current == '\'') ? 0 : -1);
}

bool Stream::setURI(const std::string & uri)
{
	_uri = uri;
	// should maybe have some sort of url handling system elsewhere that we use here instead?
	if (_uri.getScheme() == std::string("file"))
	{
		std::wifstream xml_file;
		int length = 0;
		wchar_t * buffer;
		xml_file.exceptions ( std::ifstream::eofbit | std::ifstream::failbit | std::ifstream::badbit );
		try
		{
			std::locale loc("");
			boost::filesystem::path full_path = boost::filesystem::system_complete(_uri.getPath());
			std::string filename = full_path.string();
			xml_file.open(filename.c_str(), std::ios::binary);
			if (!xml_file)
			{
				std::cout << "Error opening file - " << filename << std::endl;
				return false;
			}
			xml_file.imbue(loc);
			//istreambuf_iterator<wchar_t> isb(xml_file);
			// get length of file:
			xml_file.seekg(0, std::ios::end);
			length = xml_file.tellg();
			xml_file.seekg(0, std::ios::beg);

			// allocate memory:
			buffer = new wchar_t [length];

			// read data as a block:
			xml_file.read(buffer, length);

			_buffer = buffer;
			delete [] buffer;
		}
		catch (std::wifstream::failure e)
		{
			std::cout << "Exception opening/reading file '" << _uri.getPath().c_str() << "' - " << e.what() << std::endl;
			return false;
		}

		xml_file.close();

		//wcout << L"got buffer:" << _buffer << endl;
		return true;
	}
	return false;
}

std::string Stream::getURI(void) const
{
	return _uri.getURL();
}

void Stream::setDelimiter(wchar_t delim)
{
	_delimiter = delim;
}

wchar_t	Stream::getDelimiter(void) const
{
	return _delimiter;
}

void Stream::setBuffer(const std::wstring & buf)
{
	_buffer = buf;
	_position = _column = _line = 0;
	_current = _buffer[0];
}

void Stream::skipBlanks(void)
{
	while (IsBlank(_buffer[_position]))
		nextChar();
}

wchar_t Stream::nextChar(void)
{
	if (_position <= _buffer.length())
	{
		if (_current == '\r' || _current == '\n')
		{
			_line++;
			_column = 1;
			_position++;
			// roundabout way of detecting a CRLF pair
			if (_current == '\r' && _buffer[_position] == '\n')
				_position++;
		}
		else
		{
			_column++;
			_position++;
		}
		_current = _buffer[_position];
	}
	// else - error
	return _current;
}

void Stream::skipChars(unsigned long count)
{
	for (unsigned long l = 0; l < count; l++)
		nextChar();
}

unsigned int Stream::size(void) const
{
	return static_cast<unsigned int>(_buffer.size());
}

bool Stream::end(void) const
{
	return (_position >= _buffer.size());
}

wchar_t Stream::peek(unsigned long pos)
{
	if (pos + _position >= _buffer.length())
		pos = static_cast<unsigned long>(_buffer.length() - _position - 1);
	return _buffer[_position + pos];
}

unsigned long Stream::mark(void)
{
	_mark = _position;
	return _mark;
}

void Stream::push(void)
{
	_pushMark = _position;
	_pushColumn = _column;
	_pushLine = _line;
}

void Stream::pop(void)
{
	_position = _pushMark;
	_column = _pushColumn;
	_line = _pushLine;
	_current = _buffer[_position];
}

std::wstring Stream::literal(void)
{
	int c1 = (_current == '"' ? 1 : (_current == '\'' ? 0 : -1));
	if (c1 == -1)
		return L"";
	nextChar();
	mark();
	if (c1 == 1)
		while (_current != '"')
			nextChar();
	else
		while (_current != '\'')
			nextChar();
	std::wstring lit = substr();
	nextChar();
	return lit;
}

std::wstring Stream::chars(wchar_t sdelim, wchar_t edelim)
{
	std::wstring sub;
	if (_current != sdelim)
		return sub;
	nextChar();
	mark();
	while (_current != edelim)
		nextChar();
	sub = substr();
	nextChar();
	return sub;
}

/*
Nmtoken    ::= NameChar+
*/
std::wstring Stream::nmtoken(void)
{
	unsigned long start = _position;
	while (isNameChar())
		nextChar();
	return _buffer.substr(start, _position - start);
}

/*
Name			::= (Letter | '_' | ':') (NameChar)*
Name (XML 1.1)  ::=   NameStartChar NameChar*
*/
std::wstring Stream::name(void)
{
	unsigned long start = _position;
	if (isLetter() || _current == '_' || _current == ':')
	{
		nextChar();
		while (isNameChar())
			nextChar();
	}
	return _buffer.substr(start, _position - start);
}

/*
VersionNum     ::= ([a-zA-Z0-9_.:] | '-')+
*/
std::wstring Stream::versionNum(void)
{
	unsigned long start = _position;
	while ((_current >= 'a' && _current <= 'z') || 
	(_current >= 'A' && _current <= 'Z') ||
	(_current >= '0' && _current <= '9') || 
	_current == '_' || _current == '.' || 
	_current == ':' || _current == '-')
		nextChar();
	return _buffer.substr(start, _position - start);
}

/*
EncName        ::= [A-Za-z] ([A-Za-z0-9._] | '-')*
*/
std::wstring Stream::encName(void)
{
	if ((_current >= 'A' && _current <= 'Z') ||
		(_current >= 'a' && _current <= 'z'))
	{
		unsigned long start = _position;
		while ((_current >= 'A' && _current <= 'Z') ||
		(_current >= 'a' && _current <= 'z') ||
		(_current >= '0' && _current <= '9') ||
		_current == '.' || _current == '_' || _current == '-')
			nextChar();
		return _buffer.substr(start, _position - start);
	}
	return L"";
}

/*
SystemLiteral  ::= ('"' [^"]* '"') | ("'" [^']* "'")
*/
std::wstring Stream::systemLiteral(void)
{
	int c1 = (_current == '"') ? 1 : ((_current == '\'') ? 0 : -1);
	if (c1 == -1)
		return L"";
	unsigned long start = _position;
	if (c1 == 1)
		while (_current != '"')
			nextChar();
	else
		while (_current != '\'')
			nextChar();
	std::wstring str = _buffer.substr(start, _position - start);
	nextChar();
	return str;
}

/*
PubidChar      ::= #x20 | #xD | #xA | [a-zA-Z0-9] | [-'()+,./:=?;!*#@$_%]
PubidLiteral   ::= '"' PubidChar* '"' | "'" (PubidChar - "'")* "'"
*/
std::wstring Stream::pubidLiteral(void)
{
	int c1 = (_current == '"') ? 1 : ((_current == '\'') ? 0 : -1);
	if (c1 == -1)
		return L"";
	nextChar();
	unsigned long start = _position;
	if (c1 == 1)
		while (isPubidChar())
			nextChar();
	else
		while (isPubidChar() && _current != '\'')
			nextChar();
	std::wstring str = _buffer.substr(start, _position - start);
	int c2 = (_current == '"') ? 1 : ((_current == '\'') ? 0 : -1);
	if (c2 == -1)
	{
		// error expected ' or "
		return str;
	}
	if (c1 != c2)
	{
		// error mismatched ' or "
	}
	nextChar();
	return str;
}

/*
NSAttName       ::=  PrefixedAttName | DefaultAttName
PrefixedAttName ::=  'xmlns:' NCName
DefaultAttName  ::=  'xmlns'
*/
std::wstring Stream::nsAttName(void)
{
	if (!_buffer.compare(_position, 5, L"xmlns"))
	{
		std::wstring name;
		skipChars(5);
		if (_current == ':')
		{
			nextChar();
			name = ncName();
		}
		else
			name = L"xmlns:";
		return name;
	}
	return L"";
}

/*
NCName          ::=  (Letter | '_') (NCNameChar)*
NCNameChar      ::=  Letter | Digit | '.' | '-' | '_' | CombiningChar |
                     Extender
*/
std::wstring Stream::ncName(void)
{
	if (isLetter() || _current == '_')
	{
		unsigned long start = _position;
		nextChar();
		while (isNcNameChar())
			nextChar();
		return _buffer.substr(start, _position - start);
	}
	return L"";
}

/*
QName           ::=  (Prefix ':')? LocalPart
Prefix          ::=  NCName
LocalPart       ::=  NCName
*/
std::wstring Stream::qName(void)
{
	std::wstring prefix, localpart, qname;

	prefix = ncName();
	if (_current == ':')
	{
		nextChar();
		localpart = ncName();
		qname = prefix + L":" + localpart;
	}
	else
		qname = prefix; // it's really the localpart
	return qname;
}

/*
LanguageID     ::= Langcode ('-' Subcode)*
Langcode       ::= ISO639Code |  IanaCode |  UserCode
ISO639Code     ::= ([a-z] | [A-Z]) ([a-z] | [A-Z])
IanaCode       ::= ('i' | 'I') '-' ([a-z] | [A-Z])+
UserCode       ::= ('x' | 'X') '-' ([a-z] | [A-Z])+
Subcode        ::= ([a-z] | [A-Z])+
*/
std::wstring Stream::languageID(void)
{
	std::wstring lang;
	return lang;
}

bool Stream::isName(const std::wstring & str)
{
	if (!IsLetter(str[0]) && (str[0] != '_') && (str[0] != ':'))
		return false;
	for (unsigned int i = 1; i < str.length(); i++)
		if (!IsNameChar(str[i]))
			return false;
	return true;
}

/*
Names      ::= Name (S Name)*
*/
bool Stream::isNames(const std::wstring &str)
{
	unsigned int i = 0;
	bool start = true;
	for (; i < str.length(); i++)
	{
		if (IsBlank(str[i]))
		{
			start = true;
			continue;
		}
		if (start)
		{
			if (!IsLetter(str[i]) && str[i] != '_' && str[i] != ':')
				return false;
			start = false;
		}
		else if (!IsNameChar(str[i]))
			return false;
	}
	return true;
}

bool Stream::isNmtoken(const std::wstring &str)
{
	for (unsigned int i = 0; i < str.length(); i++)
		if (!IsNameChar(str[i]))
			return false;
	return true;
}

/*
Nmtokens   ::= Nmtoken (S Nmtoken)*
*/
bool Stream::isNmtokens(const std::wstring &str)
{
	for (unsigned int i = 0; i < str.length(); i++)
		if (!IsBlank(str[i]) && !IsNameChar(str[i]))
			return false;
	return true;
}

bool Stream::isBlank(void) { return IsBlank(_current); }
bool Stream::isChar(void) { return IsChar(_current); }
bool Stream::isLetter(void) { return IsLetter(_current); }
bool Stream::isNameChar(void) { return IsNameChar(_current); }
bool Stream::isPubidChar(void) { return IsPubidChar(_current); }

bool Stream::isNcNameChar(void)
{
	return IsNCNameChar(_current);
}

unsigned long Stream::position(void) { return _position; }
wchar_t Stream::current(void) { return _current; }
unsigned long Stream::column(void) { return _column; }
unsigned long Stream::line(void) { return _line; }

int Stream::compare(const std::wstring &str, unsigned long length)
{
	/*
	if (!wcout)
		cout << "no wcout!" << endl;
	wcout << L"comparing:" << str << " to " << length << " chars of: " << _buffer << endl;
	*/
#ifdef _DEBUG
	std::wstring sect = _buffer.substr(_position, length);
#endif
	return _buffer.compare(_position, length, str);
}

int Stream::compare(const std::wstring & str, unsigned long start, unsigned long length)
{
	if (_position + start >= _buffer.length())
		start = static_cast<unsigned long>(_buffer.length() - _position - 1);
#ifdef _DEBUG
	std::wstring sect = _buffer.substr(_position + start, length);
#endif
	return _buffer.compare(_position + start, length, str);
}

std::wstring Stream::substr(unsigned long start, unsigned long length)
{
	if (_position + start >= _buffer.length())
		start = static_cast<unsigned long>(_buffer.length() - _position - 1);

	return _buffer.substr(_position + start, length);
}

std::wstring Stream::substr(unsigned long length)
{
	return substr(0, length);
}

std::wstring Stream::substr(void)
{
	return _buffer.substr(_mark, _position - _mark);
}

const wchar_t & Stream::operator[](unsigned long sub) const
{
	if (sub >= _buffer.length())
		sub = static_cast<unsigned long>(_buffer.length() - 1);
	return _buffer[sub];
}

/*
void Stream::eatNewLines(void)
{
	while (_current == '\n' || _current == '\r')
	{
		if (_buffer[_position] == '\r')
		{
			_line++;
			_column = 1;
			_position++;
		}
		if (_buffer[_position] == '\n')
		{
			if (_buffer[_position - 1] != '\r')
				_line++;
			_column = 1;
			_position++;
		}
		_current = _buffer[_position];
	}
	skipBlanks();
}
*/
