#ifndef INCLUDED_XML_PARSER
#define INCLUDED_XML_PARSER

#include <list>
#include <map>
#include <vector>

#include <boost/shared_ptr.hpp>

#include "XMLString.h"
#include "Exception.h"
#include "Tree.h"
#include "Validator.h"
#include "Stream.h"

namespace XML
{
	// pimpl class 
	class FSMParser;

	/**
	 * XML Parser interface.
	 * Client applications should use this class for parsing xml documents.
	 * This parser class does not do the actual document parsing - that task is 
	 * handled by the pimpl instance.
	 **/
	class Parser
	{
		public:
			/**
			 * Default Constructor
			 **/
			Parser();

			/**
			 * Default Destructor
			 **/
			~Parser();

			/**
			 * Initialize the parser. The parser is automatically reinitialized as
			 * necessary between parse calls so it is not required to be directly 
			 * initialized before usage.
			 * @return whether the parser was successfully initialized. 
			 **/
			bool initialize();

			/**
			 * Specify whether XML parsing exceptions print output. The default setting is to 
			 * display exception messages.
			 * @param quiet_output whether exceptions should be printed or not
			 */
			void quiet(bool quiet_output);

			/**
			 * Enable or disable document Well-Formedness Constraint checks by the parser.
			 * @param wfc_state whether the parsed document should be checked for wfc errors.
			 **/
			void enableWFC(bool wfc_state);

			/**
			 * Get parser WFC checking state.
			 * @return whether or not documents will be checked for WFC errors when they are parsed.
			 **/
			bool isWFCEnabled() const;

			/**
			 * Enable or disable document validation by the parser.
			 * @param validation_state whether the parsed document should be validated or not.
			 **/
			void enableValidation(bool validation_state);

			/**
			 * Get parser validation setting.
			 * @return whether or not documents will be validated when they are parsed.
			 **/
			bool isValidationEnabled() const;

			/**
			 * Set the URI of the document to be parsed.
			 * @param url a URI formatted string describing the location of the document to be parsed.
			 **/

			void uri(const std::string & url);

			/**
			 * Get the URI of the document to be parsed.
			 * @return a URI formatted string describing the location of the document to be parsed.
			 **/
			std::string	uri() const;

			/**
			 * Parse a document at the given location.
			 * @param url a URI formatted string describing the location of the document that will be parsed.
			 * @return whether the document was successfully parsed or not.
			 **/
			bool parse(const std::string & url);
			/**
			 * Parse a document. The location of the document to be parsed should have already been
			 * provided to the parser.
			 * @return whether the document was successfully parsed or not.
			 **/
			bool parse();

			/**
			 * Set a parser error condition.
			 * @param e the parser exception that has occured.
			 **/
			void error(const Exception & e);

			/**
			 * Write the document object to a uri location.
			 * @param uri a URI formatted string describing the location to write the document.
			 * @param doc a pointer to a document object to write.
			 * @return whether the document was successfully written or not.
			 **/
			bool write(const std::string & uri, boost::shared_ptr<DOM::Document> doc);

			/**
			 * Write the parser's internal document object to a uri location.
			 * @param uri a URI formatted string describing the location to write the document.
			 * @return whether the document was successfully written or not.
			 **/
			bool write(const std::string & uri);

			/**
			 * Get a pointer to the parsed document object.
			 * @return a pointer to the document object.
			 **/
			boost::shared_ptr<Document> document();
			/**
			 * Get a pointer to the unparsed document stream.
			 * @return a pointer to the stream object.
			 **/
			Stream * stream();

		private:

			enum ParserFlag
			{
				// internal parser states
				StateUninitialized	= (1<<1),
				StateParsing		= (1<<2),
				StateDirty			= (1<<3),
				StateReady			= (1<<4),
				// parser settings
				ValidationEnabled	= (1<<5),
				PreserveWhiteSpace	= (1<<6),
				RepaceEntities		= (1<<7),
				FatalError			= (1<<8),
				WFCEnabled			= (1<<9),
				QuietOutput			= (1<<10)
			};

			Stream				_stream;
			unsigned int		_parserFlags;
			FSMParser *			_pimpl;
	};

}; // end namespace XML

#endif // INCLUDED_XML_PARSER
