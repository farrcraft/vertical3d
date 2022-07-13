#include "Doctype.h"

using namespace XML;

Doctype::Doctype(const std::wstring & name)
{
	nodeName(name);
}

void Doctype::externalID(const ExternalID & extid)
{
	_externalID = extid;
}

ExternalID & Doctype::externalID(void)
{
	return _externalID;
}

std::map<std::wstring, ElemDecl> & Doctype::elementDecl(void)
{
	return _elementDecl;
}

boost::shared_ptr<Misc> Doctype::misc(void)
{
	return _misc;
}

void Doctype::addMisc(boost::shared_ptr<Misc> m)
{
	if (!_misc)
		_misc = m;
	else
		_misc->addSibling(m);
}

bool Doctype::sync(std::wofstream & xml_file)
{
	if (nodeName().size() == 0)
		return true;
	xml_file << "<!DOCTYPE " << nodeName() << " ";
	_externalID.sync(xml_file);

	// S? ('[' (markupdecl | DeclSep)* ']' S?)?
	// [28a] DeclSep		::= PEReference | S 
	// [29]	markupdecl		::= elementdecl | AttlistDecl | EntityDecl | NotationDecl

	// write head misc section
	for (boost::shared_ptr<Misc> m = _misc; m != 0; m = boost::dynamic_pointer_cast<Misc, Node>(m->nextSibling()))
	{
		if (m->position() == Misc::Head)
			m->sync(xml_file);
	}

	xml_file << '>' << std::endl;

	return true;
}
