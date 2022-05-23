#ifndef INCLUDED_V3D_BMPREADER
#define INCLUDED_V3D_BMPREADER

#include "../ImageReader.h"

namespace v3D
{

	class BMPReader : public ImageReader
	{
		public:
			BMPReader();
			~BMPReader();

			virtual boost::shared_ptr<Image> read(const std::string & filename);
	};

}; // end namespace v3D

#endif // INCLUDED_V3D_BMPREADER
