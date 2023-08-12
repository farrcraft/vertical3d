/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#include "Frame.h"

namespace v3d::render::realtime {

/**
**/
Frame::Frame(boost::shared_ptr<Context> context) :
	context_(context) {

}

/**
	**/
void Frame::addOperation(boost::shared_ptr<Operation> operation) {
	operations_.push_back(operation);
}

void Frame::draw() {
	// iterate through operations
	for (std::vector<boost::shared_ptr<Operation>>::iterator it = operations_.begin(); it != operations_.end(); ++it) {
		(*it)->run(context_);
	}
}

};  // namespace v3d::render::realtime
