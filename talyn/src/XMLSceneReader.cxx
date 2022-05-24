/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "XMLSceneReader.h"

#include <boost/lexical_cast.hpp>

#include "../../quantumxml/Parser.h"

using namespace Talyn;

/*
<scene name="test-scene-01">
	<frame>
		<format width="640" height="480" />
		<clipping near="0.01" far="1.0" />
	</frame>
	<background>
		<color red="0.0" green="0.0" blue="0.0" />
	</background>
	<sphere radius="1.0" />
	<plane />
</scene>
*/
bool readXMLScene(boost::shared_ptr<XML::Document> doc)
{
	if (!doc)
	{
		return false;
	}

	boost::shared_ptr<DOM::Node> sceneNode = doc->documentElement();
	if (!sceneNode)
	{
		return false;
	}

	// walk through children nodes
	boost::shared_ptr<DOM::Node> childNode = sceneNode->firstChild();
	for (; childNode != 0; childNode = childNode->nextSibling())
	{
		if (childNode->nodeName() == std::wstring(L"frame"))
		{

		}
		else if (childNode->nodeName() == std::wstring(L"background"))
		{

		}
		else if (childNode->nodeName() == std::wstring(L"sphere"))
		{

		}
		else if (childNode->nodeName() == std::wstring(L"plane"))
		{

		}
	}
	return true;
}

bool XMLSceneReader::read(const boost::shared_ptr<RenderContext> & rc, const std::string & filename)
{
	// need to determine the filetype of infile
	// boost may have some file utilities for getting the file extension from the filename
	// it should either be an .xml file for our own internal scene description format
	// or .rib for renderman formatted files.
	XML::Parser parser;

	parser.enableValidation(true);
	parser.enableWFC(true);
	parser.quiet(false);

	if (!parser.parse(filename))
	{
		std::cout << "Abort. Fatal parser error while reading scene file: " << filename << std::endl;
		return false;
	}
	readXMLScene(parser.document());

	return true;
}

