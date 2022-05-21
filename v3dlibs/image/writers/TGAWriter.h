#ifndef INCLUDED_V3D_TGAWRITER
#define INCLUDED_V3D_TGAWRITER

#include "../ImageWriter.h"

namespace v3D
{

	class TGAWriter : public ImageWriter
	{
		public:
			TGAWriter();
			~TGAWriter();

			virtual bool write(const std::string & filename, const boost::shared_ptr<Image> & img);
	};

}; // end namespace v3D

#endif // INCLUDED_V3D_TGAWRITER
