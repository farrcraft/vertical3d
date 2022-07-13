/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include <cstdlib>

#include "Controller.h"

int main(int argc, char *argv[]) {
    Controller controller;

    if (!controller.run()) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
