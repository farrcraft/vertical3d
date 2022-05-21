#ifndef INCLUDED_V3D_BMPWRITER
#define INCLUDED_V3D_BMPWRITER

#include "../ImageWriter.h"

namespace v3D
{

	class BMPWriter : public ImageWriter
	{
		public:
			BMPWriter();
			~BMPWriter();

			virtual bool write(const std::string & filename, const boost::shared_ptr<Image> & img);
	};

}; // end namespace v3D

#endif // INCLUDED_V3D_BMPWRITER
