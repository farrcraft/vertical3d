#include "Document.h"

#include <iostream>
#include <fstream>
#include <cassert>

using namespace XML;

Document::Document() : _flags(0)
{
}

Document::~Document()
{
}

unsigned int Document::flags(void)
{
	return _flags;
}

void Document::flags(unsigned int f)
{
	_flags = f;
}

void Document::setFlag(unsigned int f)
{
	_flags |= f;
}

void Document::setDoctype(const std::wstring & name)
{
	Doctype * dtd = new Doctype(name);
	doctype(dtd);
}

void Document::version(const std::wstring & vers)
{
	xmlVersion(vers);
}

void Document::standalone(const std::wstring & sa)
{
	if (sa == L"yes")
		xmlStandalone(true);
	else
		xmlStandalone(false);
}

void Document::encoding(const std::wstring & enc)
{
	xmlEncoding(enc);
}

boost::shared_ptr<Misc> Document::misc(void)
{
	return _misc;
}

void Document::setRootElement(boost::shared_ptr<Element> elem)
{
	documentElement(elem);
}

void Document::addMisc(boost::shared_ptr<Misc> m)
{
	if (m->position() == Misc::Head)
	{
		Doctype * dtd = dynamic_cast<Doctype*>(doctype());
		dtd->addMisc(m);
	}
	else
	{
		if (!_misc)
			_misc = m;
		else
			_misc->addSibling(m);
	}
}

bool Document::sync(const URL & uri)
{
	std::wofstream xml_file;
	try
	{
		//std::cout << "Trying URL File Open - " << uri.getPath().c_str() << std::endl;
		xml_file.open(uri.getPath().c_str(), std::ios::binary | std::ios::trunc);
		if (!xml_file)
		{
			std::cout << "Error opening file" << std::endl;
			return false;
		}

		// write xml decl
		xml_file << "<?xml version=\"";
		if (xmlVersion().size() > 0)
			xml_file << xmlVersion();
		else
			xml_file << "1.0";
		xml_file << "\"";
		if (xmlEncoding().size() > 0)
			xml_file << " encoding=\"" << xmlEncoding() << "\"";
		if (!(_flags & NoSDDecl))
		{
			std::string standalone = xmlStandalone() ? "true" : "false";
			xml_file << " standalone=\"" << standalone.c_str() << "\"";
		}
		xml_file << "?>" << std::endl;

		// write prolog misc section
		for (boost::shared_ptr<Misc> m = _misc; m != 0; m = boost::dynamic_pointer_cast<Misc, Node>(m->nextSibling()))
		{
			if (m->position() == Misc::Prolog)
				m->sync(xml_file);
		}

		// write root document element
		if (documentElement()->nodeName().size() > 0)
		{
			//Element * elem = dynamic_cast<Element*>(documentElement());
			// write element (and recurse over all children elements too)
			dynamic_cast<Element*>(documentElement().get())->sync(xml_file);
		}

		// write epilog misc section
		for (boost::shared_ptr<Misc> m = _misc; m != 0; m = boost::dynamic_pointer_cast<Misc, Node>(m->nextSibling()))
		{
			if (m->position() == Misc::Epilog)
				m->sync(xml_file);
		}
	}
	catch (std::wofstream::failure e)
	{
		std::cout << "Exception opening/reading file - " << e.what() << std::endl;
	}

	xml_file.close();

	return true;
}
