/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2021 Joshua Farr (josh@farrcraft.com)
 **/

#include "Frame.h"

using namespace odyssey::render;

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
