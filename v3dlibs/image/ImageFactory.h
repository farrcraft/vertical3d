/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "ImageReader.h"
#include "ImageWriter.h"

#include <map>

namespace v3d::image {
	/**
	 * 
	 **/
	class ImageFactory {
		public:
			ImageFactory();
			~ImageFactory() = default;

			void add(const std::string & name, const boost::shared_ptr<ImageReader> & reader);
			void add(const std::string & name, const boost::shared_ptr<ImageWriter> & writer);

			boost::shared_ptr<Image> read(const std::string & filename);
			bool write(const std::string & filename, const boost::shared_ptr<Image> & img);

		private:
			std::map<std::string, boost::shared_ptr<ImageReader> > readers_;
			std::map<std::string, boost::shared_ptr<ImageWriter> > writers_;
	};

};
