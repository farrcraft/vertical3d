#ifndef INCLUDED_V3D_IMAGEREADER
#define INCLUDED_V3D_IMAGEREADER

#include "Image.h"

#include <boost/shared_ptr.hpp>

#include <string>

namespace v3D
{

	/**
	 *  
	 **/
	class ImageReader
	{
		public:
			ImageReader();
			virtual ~ImageReader();

			virtual boost::shared_ptr<Image> read(const std::string & filename);

		private:

	};

}; // end namespace v3D

#endif // INCLUDED_V3D_IMAGEREADER
