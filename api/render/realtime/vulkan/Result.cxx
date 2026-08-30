/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Result.h"

#include <sstream>
#include <string>

namespace v3d::render::realtime::vulkan {

    /**
     **/
    std::string resultString(VkResult result) {
        switch (result) {
            case VK_SUCCESS:
                return "success";
            case VK_NOT_READY:
                return "not ready";
            case VK_TIMEOUT:
                return "timeout";
            case VK_INCOMPLETE:
                return "incomplete";
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                return "out of host memory";
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                return "out of device memory";
            case VK_ERROR_INITIALIZATION_FAILED:
                return "initialization failed";
            case VK_ERROR_DEVICE_LOST:
                return "device lost";
            case VK_ERROR_MEMORY_MAP_FAILED:
                return "memory map failed";
            case VK_ERROR_LAYER_NOT_PRESENT:
                return "layer not present";
            case VK_ERROR_EXTENSION_NOT_PRESENT:
                return "extension not present";
            case VK_ERROR_FEATURE_NOT_PRESENT:
                return "feature not present";
            case VK_ERROR_INCOMPATIBLE_DRIVER:
                return "incompatible driver";
            case VK_ERROR_TOO_MANY_OBJECTS:
                return "too many objects";
            case VK_ERROR_FORMAT_NOT_SUPPORTED:
                return "format not supported";
            case VK_ERROR_SURFACE_LOST_KHR:
                return "surface lost";
            case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
                return "native window in use";
            case VK_ERROR_OUT_OF_DATE_KHR:
                return "out of date";
            case VK_ERROR_UNKNOWN:
                return "unknown error";
            default:
                break;
        }
        std::stringstream msg;
        msg << "error " << static_cast<int>(result);
        return msg.str();
    }

};  // namespace v3d::render::realtime::vulkan
