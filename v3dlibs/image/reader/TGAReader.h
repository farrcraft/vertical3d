#ifndef INCLUDED_V3D_TGAREADER
#define INCLUDED_V3D_TGAREADER

#include "../ImageReader.h"

namespace v3D
{

	class TGAReader : public ImageReader
	{
		public:
			TGAReader();
			~TGAReader();

			virtual boost::shared_ptr<Image> read(const std::string & filename);
	};

}; // end namespace v3D

#endif // INCLUDED_V3D_TGAREADER
