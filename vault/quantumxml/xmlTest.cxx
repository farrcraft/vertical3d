/**
 * XMLTest - An application to test the features of the xml parser library.
 *
 * (c) 2006-2008
 **/

#include "Parser.h"
#include "getopts.h"


void print_attributes(boost::shared_ptr<DOM::Node> elem)
{
	DOM::NamedNodeMap * attribs = elem->attributes();
	unsigned int count = attribs->length();
	if (count > 0)
		std::wcout << " (attrs)";
	for (unsigned int i = 0; i < count; i++)
	{
		boost::shared_ptr<DOM::Attr> att(boost::dynamic_pointer_cast<DOM::Attr, DOM::Node>(attribs->item(i)));
		if (!att)
			std::wcout << "NULL Attr Cast!";
		std::wcout << " " << att->name() << "=" << att->value();
	}
}

void print_nodetype(boost::shared_ptr<DOM::Node> node)
{
	std::wcout << " type: ";
	switch (node->nodeType())
	{
		case DOM::Node::ATTRIBUTE_NODE : 
			std::wcout << "[Attribute] ";
			break;
		case DOM::Node::CDATA_SECTION_NODE :
			std::wcout << "[CDSECT] ";
			break;
		case DOM::Node::COMMENT_NODE :
			std::wcout << "[COMMENT] ";
			break;
		case DOM::Node::DOCUMENT_FRAGMENT_NODE :
			std::wcout << "[DOCFRAG] ";
			break;
		case DOM::Node::DOCUMENT_NODE :
			std::wcout << "[DOCUMENT] ";
			break;
		case DOM::Node::DOCUMENT_TYPE_NODE :
			std::wcout << "[DTD] ";
			break;
		case DOM::Node::ELEMENT_NODE :
			std::wcout << "[ELEMENT] ";
			break;
		case DOM::Node::ENTITY_NODE :
			std::wcout << "[ENTITY] ";
			break;
		case DOM::Node::ENTITY_REFERENCE_NODE :
			std::wcout << "[ENTREF] ";
			break;
		case DOM::Node::NOTATION_NODE :
			std::wcout << "[NOTATION] ";
			break;
		case DOM::Node::PROCESSING_INSTRUCTION_NODE :
			std::wcout << "[PI] ";
			break;
		case DOM::Node::TEXT_NODE :
			std::wcout << "[TEXT] ";
			break;
		default:
			std::wcout << "[UNKNOWN] ";
			break;
	}
}

void print_node(boost::shared_ptr<DOM::Node> node, int depth)
{
	if (!node)
	{
		std::cout << "print_node passed bad node!" << std::endl;
		return;
	}

	for (int i = 0; i < depth; i++)
		std::wcout << "    ";

	std::wcout << "node name: " << node->nodeName();

	print_nodetype(node);

	if (node->nodeType() == DOM::Node::ELEMENT_NODE)
		print_attributes(node);

	std::wcout << std::endl;

	if (node->firstChild())
		print_node(node->firstChild(), depth + 1);

	if (node->nextSibling())
		print_node(node->nextSibling(), depth);
}

void print_misc(boost::shared_ptr<XML::Misc> misc)
{
	if (!misc)
		return;
	std::wcout << "misc name: " << misc->nodeName();
	std::wcout << " value: " << misc->nodeValue();
	std::wcout << " position: ";
	if (misc->position() == XML::Misc::Prolog)
		std::wcout << "Prolog";
	else if (misc->position() == XML::Misc::Head)
		std::wcout << "Head";
	else if (misc->position() == XML::Misc::Epilog)
		std::wcout << "Epilog";
	std::wcout << std::endl;
}

void print_document(boost::shared_ptr<XML::Document> doc)
{
	std::cout << std::endl << "Printing document..." << std::endl;
	std::wcout << "xml version: " << doc->xmlVersion() << std::endl;
	print_misc(doc->misc());
	print_node(doc->documentElement(), 0);
}

void usage(void)
{
	std::cout << "usage:" << std::endl;
	std::cout << "xmlTest --file filename --outfile filename --wfc --validate --silent -q" << std::endl;
	std::cout << "	--wfc		- well-formedness checking" << std::endl;
	std::cout << "	--validate	- validation checking" << std::endl;
	std::cout << "	--silent	- don't print the resulting parsed tree" << std::endl;
	std::cout << "  --quiet		- don't print XML parser exception messages" << std::endl;
	std::cout << "	--file		- input filename to be parsed" << std::endl;
	std::cout << "	--outfile	- sync resulting parsed tree back to the filename" << std::endl;
}

int main (int argc, char *argv[])
{

	GetOpts getopts(argc, argv, "q|quiet:w|wfc:v|validate:s|silent:f|file*:o|outfile*");

	bool validate = false;
	bool wfc = false;
	bool silent = false;
	bool sync = false;
	bool quiet = false;
	std::string infile;
	std::string outfile;

	int opt;
	while ((opt = getopts.next()) != EOF)
	{
		switch (opt)
		{
			case 'w':
				wfc = true;
				break;
			case 'v':
				validate = true;
				break;
			case 's':
				silent = true;
				break;
			case 'q':
				quiet = true;
				break;
			case 'f':
				infile = getopts.getarg();
				break;
			case 'o':
				sync = true;
				outfile = getopts.getarg();
				break;
			default:
				break;
		}
	}

	if (infile.empty())
	{
		usage();
		exit(EXIT_SUCCESS);
	}

	XML::Parser parser;

	parser.enableValidation(validate);
	parser.enableWFC(wfc);
	parser.quiet(quiet);

	if (!silent)
		std::cout << "Parsing: " << infile << std::endl;

	if (!parser.parse(infile))
	{
		std::cout << "Abort. Fatal parser error." << std::endl;
		return EXIT_FAILURE;
	}

	if (!silent)
		print_document(parser.document());

	if (sync)
		parser.write(outfile);

	return EXIT_SUCCESS;
}