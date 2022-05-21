#ifndef INCLUDED_V3D_JPEGWRITER
#define INCLUDED_V3D_JPEGWRITER

#include "../ImageWriter.h"

namespace v3D
{

	class JPEGWriter : public ImageWriter
	{
		public:
			JPEGWriter();
			~JPEGWriter();

			virtual bool write(const std::string & filename, const boost::shared_ptr<Image> & img);
	};

}; // end namespace v3D

#endif // INCLUDED_V3D_JPEGWRITER
