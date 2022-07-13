#include "FSMParser.h"
#include "Parser.h"

#include "states/Document.h"
#include "states/XMLDecl.h"
#include "states/Comment.h"
#include "states/ProcessingInstruction.h"
#include "states/DoctypeDecl.h"
#include "states/Misc.h"
#include "states/ElementDecl.h"
#include "states/AttlistDecl.h"
#include "states/NotationDecl.h"
#include "states/EntityDecl.h"
#include "states/CDSect.h"
#include "states/Element.h"
#include "states/Attribute.h"

using namespace XML;
using namespace FSM;

FSMParser::FSMParser(Parser * parser) : StateMachine(), _parser(parser)
{
	new FSMState::Document(this);
	new FSMState::XMLDecl(this);
	new FSMState::Misc(this);
	new FSMState::Comment(this);
	new FSMState::ProcessingInstruction(this);
	new FSMState::DoctypeDecl(this);
	new FSMState::ElementDecl(this);
	new FSMState::AttlistDecl(this);
	new FSMState::NotationDecl(this);
	new FSMState::EntityDecl(this);
	new FSMState::CDSect(this);
	new FSMState::Element(this);
	new FSMState::Attribute(this);
}

FSMParser::~FSMParser()
{
}

void FSMParser::run()
{
	_document = boost::shared_ptr<Document>(new Document());

	StateMachine::run("document");
}

Stream * FSMParser::stream()
{
	return _parser->stream();
}

boost::shared_ptr<Document> FSMParser::document()
{
	return _document;
}

void FSMParser::error(const Exception & e)
{
	_parser->error(e);
}

boost::shared_ptr<XML::Element> FSMParser::currentElement() const
{
	return _currentElement;
}

void FSMParser::currentElement(boost::shared_ptr<XML::Element> elem)
{
	_currentElement = elem;
}

Misc::MiscPosition FSMParser::getMiscLocation()
{
	if (history("prolog"))
	{
		if (!history("doctypedecl"))
			return Misc::Prolog;
		if (!(document()->flags() & Document::NoDocTypeDecl) && !history("element"))
			return Misc::Head;
		if (_currentElement)
			return Misc::Body;
		return Misc::Epilog;
	}
	return Misc::Body;
}

// [75] ExternalID		::= 'SYSTEM' S SystemLiteral | 'PUBLIC' S PubidLiteral S SystemLiteral  
bool FSMParser::getExternalID(ExternalID & extid)
{
	if (!stream()->compare(L"PUBLIC", 6))
	{
		stream()->skipChars(6);
		if (!stream()->isBlank())
		{
			Exception e(Exception::NoWhitespace);
			e.message("expected whitespace in external id.");
			error(e);
			return false;
		}
		else
			stream()->skipBlanks();
		std::wstring pub = stream()->pubidLiteral();

		// next sequence must be S SystemLiteral
		if (!stream()->isBlank())
		{
			Exception e(Exception::NoWhitespace);
			e.message("expected whitespace in external id.");
			error(e);
			return false;
		}
		else
			stream()->skipBlanks();
		std::wstring sys = stream()->systemLiteral();

		extid.publicId(pub);
		extid.systemId(sys);
	}
	else if (!stream()->compare(L"SYSTEM", 6))
	{
		stream()->skipChars(6);
		if (!stream()->isBlank())
		{
			Exception e(Exception::NoWhitespace);
			e.message("expected whitespace in external id.");
			error(e);
			return false;
		}
		else
			stream()->skipBlanks();
		std::wstring sys = stream()->systemLiteral();

		extid.systemId(sys);
	}
	return true;
}

// [67] Reference		::= EntityRef | CharRef 
/*
bool FSMParser::Reference(Reference & ref)
{
	if (EntityRef(ref) || CharRef(ref))
		return true;
	return false;
}
*/

// [69] PEReference    ::= '%' Name ';' 
bool FSMParser::PEReference(boost::shared_ptr<Reference> ref)
{
	if (stream()->current() == '%')
	{
		stream()->nextChar();
		std::wstring name = stream()->name();
		if (stream()->current() == ';')
			stream()->nextChar();
		else
		{
			Exception e(Exception::ExpectedChar);
			e.message("expected ';'");
			error(e);
		}
		ref.reset(new Reference);
		ref->type(Reference::PERef);
		ref->value(name);
		return true;
	}
	return false;
}

// [68] EntityRef		::= '&' Name ';'
bool FSMParser::EntityRef(boost::shared_ptr<Reference> ref)
{
	if (stream()->current() == '&' && stream()->peek() != '#')
	{
		stream()->nextChar();
		std::wstring name = stream()->name();
		if (stream()->current() != ';')
		{
			Exception e(Exception::ExpectedChar);
			e.message("expected ';' in entityref.");
			error(e);
		}
		else
			stream()->nextChar();
		ref.reset(new Reference);
		ref->type(Reference::EntityRef);
		ref->value(name);
		return true;
	}
	return false;
}

// [66] CharRef		::= '&#' [0-9]+ ';' | '&#x' [0-9a-fA-F]+ ';' 
bool FSMParser::CharRef(boost::shared_ptr<Reference> ref)
{
	if (stream()->current() == '&' && stream()->peek() == '#')
	{
		stream()->skipChars(2);
		int val = 0;
		wchar_t c;
		std::wstring name;
		if (stream()->current() == 'x')
		{
			stream()->nextChar();
			while (stream()->current() != ';')
			{
				if (stream()->current() >= '0' && stream()->current() <= '9')
					val = val * 16 + (stream()->current() - '0');
				else if (stream()->current() >= 'a' && stream()->current() <= 'f')
					val = val * 16 + (stream()->current() - 'a') + 10;
				else if (stream()->current() >= 'A' && stream()->current() <= 'F')
					val = val * 16 + (stream()->current() - 'A') + 10;
				else
				{
					Exception e(Exception::IllegalChar);
					e.message("illegal charref value.");
					error(e);
				}
				stream()->nextChar();
			}
			c = val;
			name = c;
		}
		else
		{
			while (stream()->current() != ';')
			{
				if (stream()->current() >= '0' && stream()->current() <= '9')
					val = val * 10 + (stream()->current() - '0');
				else
				{
					Exception e(Exception::IllegalChar);
					e.message("illegal charref value.");
					error(e);
					return false;
				}
				stream()->nextChar();
			}
			c = val;
			name = c;
		}
		if (!IsChar(c))
		{
			Exception e(Exception::IllegalChar);
			e.message("illegal charref value.");
			error(e);
		}
		ref.reset(new Reference);
		ref->type(Reference::CharRef);
		ref->value(name);
		return true;
	}
	return false;
}
