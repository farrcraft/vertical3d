#ifndef INCLUDED_V3D_JPEGREADER
#define INCLUDED_V3D_JPEGREADER

#include "../ImageReader.h"

namespace v3D
{

	class JPEGReader : public ImageReader
	{
		public:
			JPEGReader();
			~JPEGReader();

			virtual boost::shared_ptr<Image> read(const std::string & filename);
	};

}; // end namespace v3D

#endif // INCLUDED_V3D_JPEGREADER
