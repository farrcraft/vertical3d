/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/engine/Application.h>
#include <api/engine/Main.h>
#include <demos/diorama/src/Diorama.h>

int main(int /* argc */, char* argv[]) {
    return v3d::engine::run<diorama::Diorama>(argv[0], { .organization = "Vertical3D", .application = "Diorama" });
}
