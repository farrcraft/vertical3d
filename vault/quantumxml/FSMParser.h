#ifndef INCLUDED_FSM_PARSER
#define INCLUDED_FSM_PARSER

#include "fsm/StateMachine.h"

#include "Stream.h"
#include "Tree.h"
#include "Exception.h"

#include "tree/Reference.h"

#include <boost/shared_ptr.hpp>

namespace XML
{

	class Parser;

	/**
	 * A Finite State Machine based XML parser.
	 * this also essentially provides a pimpl idiom for the public parser interface
	 * this is the private implementation and is never seen or referenced by client code.
	 * users will instead access the public Parser interface
	 **/
	class FSMParser : public FSM::StateMachine
	{
		public:
			/**
			 * Default Constructor.
			 * @param parser a pointer to the public parser interface.
			 **/
			FSMParser(Parser * parser);
			/**
			 * Virtual Destructor.
			 **/
			virtual ~FSMParser();

			/**
			 * Begin parser execution. 
			 **/
			virtual void run();

			/**
			 * Get a pointer to the unparsed document stream.
			 * @return a pointer to the stream object.
			 **/
			Stream * stream();
			/**
			 * Get a pointer to the parsed document object.
			 * @return a pointer to the document object.
			 **/
			boost::shared_ptr<Document> document();
			/**
			 * Set a parser error condition.
			 * @param e the parser exception that has occured.
			 **/
			void error(const Exception & e);

			/**
			 * Get the location of a Misc xml production within the document.
			 * @return a token describing where the Misc node has occured.
			 **/
			Misc::MiscPosition getMiscLocation();

			/**
			 * Parse an ExternalID from the document stream.
			 * @param extid a reference to the parsed ExternalID data.
			 * @return whether the ExternalID was successfully parsed.
			 **/
			bool getExternalID(ExternalID & extid);

			/**
			 * Parse a character reference from the document stream.
			 * @param ref a pointer to the parsed reference object.
			 * @return whether a reference was successfully parsed.
			 **/
			bool CharRef(boost::shared_ptr<Reference> ref);
			/**
			 * Parse an entity reference from the document stream.
			 * @param ref a pointer to the parsed entity reference.
			 * @return whether a reference was successfully parsed.
			 **/
			bool EntityRef(boost::shared_ptr<Reference> ref);
			/**
			 * Parse a PE reference from the document stream.
			 * @param ref a pointer to the parsed reference object.
			 * @return whether a reference was successfully parsed.
			 **/
			bool PEReference(boost::shared_ptr<Reference> ref);

			/**
			 * Get the current element that is being parsed.
			 * @return a pointer to the current element.
			 **/
			boost::shared_ptr<XML::Element> currentElement() const;
			/**
			 * Set the current element that is being parsed.
			 * @param elem a pointer to the new current element.
			 **/
			void currentElement(boost::shared_ptr<XML::Element> elem);

		private:
			Parser * _parser;
			boost::shared_ptr<XML::Element> _currentElement;
			boost::shared_ptr<Document> _document;
	};

}; // end namespace XML


#endif // INCLUDED_FSM_PARSER
