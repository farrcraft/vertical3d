#ifndef INCLUDED_V3D_IMAGEWRITER
#define INCLUDED_V3D_IMAGEWRITER

#include "Image.h"

#include <boost/shared_ptr.hpp>

#include <string>

namespace v3D
{

	/**
	 *  
	 **/
	class ImageWriter
	{
		public:
			ImageWriter();
			virtual ~ImageWriter();

			virtual bool write(const std::string & filename, const boost::shared_ptr<Image> & img);

		private:

	};

}; // end namespace v3D

#endif // INCLUDED_V3D_IMAGEWRITER
