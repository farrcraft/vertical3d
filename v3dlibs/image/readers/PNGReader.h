#ifndef INCLUDED_V3D_PNGREADER
#define INCLUDED_V3D_PNGREADER

#include "../ImageReader.h"

namespace v3D
{

	class PNGReader : public ImageReader
	{
		public:
			PNGReader();
			~PNGReader();

			virtual boost::shared_ptr<Image> read(const std::string & filename);
	};

}; // end namespace v3D

#endif // INCLUDED_V3D_PNGREADER
