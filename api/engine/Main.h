/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

/**
 * The header for the one file that defines main() and calls run<T>.
 *
 * It brings in run<T>, and SDL's entry point. A windows subsystem executable starts in WinMain.
 * SDL_main.h defines WinMain, and WinMain calls main(). That definition must be in exactly one
 * translation unit, so no other file includes this header.
 **/

#include <api/engine/Application.h>  // IWYU pragma: export

#include <SDL3/SDL_main.h>
