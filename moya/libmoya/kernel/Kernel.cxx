/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

// Every header of kernel code, compiled under the kernel target's rules: the kernel's standard,
// with no exceptions and no RTTI. A header missing from this list is compiled only under the
// rules of whatever includes it.

#include <moya/libmoya/kernel/Array.h>  // IWYU pragma: keep
#include <moya/libmoya/kernel/camera/Camera.h>  // IWYU pragma: keep
#include <moya/libmoya/kernel/film/Filter.h>  // IWYU pragma: keep
#include <moya/libmoya/kernel/film/Pixels.h>  // IWYU pragma: keep
#include <moya/libmoya/kernel/geometry/BruteForce.h>  // IWYU pragma: keep
#include <moya/libmoya/kernel/geometry/Hit.h>  // IWYU pragma: keep
#include <moya/libmoya/kernel/geometry/Offset.h>  // IWYU pragma: keep
#include <moya/libmoya/kernel/geometry/Ray.h>  // IWYU pragma: keep
#include <moya/libmoya/kernel/geometry/Traversal.h>  // IWYU pragma: keep
#include <moya/libmoya/kernel/geometry/Triangle.h>  // IWYU pragma: keep
#include <moya/libmoya/kernel/Launch.h>  // IWYU pragma: keep
#include <moya/libmoya/kernel/Qualifiers.h>  // IWYU pragma: keep
#include <moya/libmoya/kernel/sample/Hash.h>  // IWYU pragma: keep
#include <moya/libmoya/kernel/sample/Sobol.h>  // IWYU pragma: keep
#include <moya/libmoya/kernel/sample/ZSobol.h>  // IWYU pragma: keep
#include <moya/libmoya/kernel/spectrum/Acescg.h>  // IWYU pragma: keep
#include <moya/libmoya/kernel/spectrum/CieDaylight.h>  // IWYU pragma: keep
#include <moya/libmoya/kernel/spectrum/CieD65.h>  // IWYU pragma: keep
#include <moya/libmoya/kernel/spectrum/CieObserver.h>  // IWYU pragma: keep
#include <moya/libmoya/kernel/spectrum/Colorimetry.h>  // IWYU pragma: keep
#include <moya/libmoya/kernel/spectrum/Daylight.h>  // IWYU pragma: keep
#include <moya/libmoya/kernel/spectrum/Rgb.h>  // IWYU pragma: keep
#include <moya/libmoya/kernel/spectrum/Spectrum.h>  // IWYU pragma: keep
#include <moya/libmoya/kernel/spectrum/Visible.h>  // IWYU pragma: keep
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>  // IWYU pragma: keep
#include <moya/libmoya/kernel/view/HitView.h>  // IWYU pragma: keep
