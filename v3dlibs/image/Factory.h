/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Reader.h"
#include "Writer.h"

#include <map>

namespace v3d::image {
	/**
	 * 
	 **/
	class Factory {
		public:
			Factory();
			~Factory() = default;

			void add(const std::string & name, const boost::shared_ptr<Reader> & reader);
			void add(const std::string & name, const boost::shared_ptr<Writer> & writer);

			boost::shared_ptr<Image> read(std::string_view filename);
			bool write(std::string_view filename, const boost::shared_ptr<Image> & img);

		private:
			std::map<std::string, boost::shared_ptr<Reader> > readers_;
			std::map<std::string, boost::shared_ptr<Writer> > writers_;
	};

};
