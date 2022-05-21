#ifndef INCLUDED_V3D_PNGWRITER
#define INCLUDED_V3D_PNGWRITER

#include "../ImageWriter.h"

namespace v3D
{

	class PNGWriter : public ImageWriter
	{
		public:
			PNGWriter();
			~PNGWriter();

			virtual bool write(const std::string & filename, const boost::shared_ptr<Image> & img);
	};

}; // end namespace v3D

#endif // INCLUDED_V3D_PNGWRITER
