#ifndef INCLUDED_DOM_PROCESSINGINSTRUCTION
#define INCLUDED_DOM_PROCESSINGINSTRUCTION

#include "Node.h"

namespace DOM
{

	class ProcessingInstruction : virtual public Node
	{
		friend class Document;

		public:
			ProcessingInstruction();

			DOMString target(void) const;
			DOMString data(void) const;

		protected:
			void target(const DOMString & str);
			void data(const DOMString & str);
	};

}; // end namespace DOM

#endif // INCLUDED_DOM_PROCESSINGINSTRUCTION
