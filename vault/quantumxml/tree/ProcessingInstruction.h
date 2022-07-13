#ifndef INCLUDED_XML_PROCESSINGINSTRUCTION
#define INCLUDED_XML_PROCESSINGINSTRUCTION

#include <iostream>
#include <fstream>

#include "Misc.h"
#include "../DOM/ProcessingInstruction.h"

namespace XML
{

	class ProcessingInstruction : public Misc, public DOM::ProcessingInstruction
	{
		public:

			void setTarget(const std::wstring & tgt);
			void setData(const std::wstring & dta);

			bool sync(std::wofstream & xml_file);
	};

}; // end namespace XML

#endif // INCLUDED_XML_PROCESSINGINSTRUCTION
