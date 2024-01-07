/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
**/


#include <cstdlib>
#include <iostream>

#include "Controller.h"


int main(int argc, char *argv[]) {
    Controller controller;

    if (!controller.run()) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
