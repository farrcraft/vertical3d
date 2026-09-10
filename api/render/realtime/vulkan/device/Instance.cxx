/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Instance.h"

#include "Result.h"

#include <cstring>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace v3d::render::realtime::vulkan::device {

namespace {

const char* const validationLayer = "VK_LAYER_KHRONOS_validation";

/**
 * What the validation layer has to say, put through the logger at a severity that
 * matches its own. An error here is a real one: the layer only speaks when the api
 * has been used in a way that is undefined or about to be.
 **/
VKAPI_ATTR VkBool32 VKAPI_CALL report(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT types, const VkDebugUtilsMessengerCallbackDataEXT* data, void* user) {
    static_cast<void>(types);

    v3d::log::Logger* logger = static_cast<v3d::log::Logger*>(user);
    if (logger == nullptr || data == nullptr || data->pMessage == nullptr) {
        return VK_FALSE;
    }

    if ((severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) != 0) {
        logger->get()->error("vulkan: {}", data->pMessage);
    } else if ((severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) != 0) {
        logger->get()->warn("vulkan: {}", data->pMessage);
    } else {
        logger->get()->info("vulkan: {}", data->pMessage);
    }

    // false: the call the layer is complaining about still goes through
    return VK_FALSE;
}

};  // namespace

/**
 **/
Instance::Instance(const boost::shared_ptr<v3d::log::Logger>& logger, const std::vector<const char*>& extensions) :
    instance_(VK_NULL_HANDLE),
    messenger_(VK_NULL_HANDLE),
    logger_(logger),
    validating_(false) {
    requireExtensions(extensions);

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vertical3D";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Vertical3D Vulkan Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    // the layer and the extension that carries its messages are both optional - a machine
    // without the sdk installed has neither, and should still run
    validating_ = hasValidationLayer() && hasExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    std::vector<const char*> enabled(extensions);
    if (validating_) {
        enabled.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(enabled.size());
    createInfo.ppEnabledExtensionNames = enabled.empty() ? nullptr : enabled.data();
    if (validating_) {
        createInfo.enabledLayerCount = 1;
        createInfo.ppEnabledLayerNames = &validationLayer;
    }

    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance_);
    if (result != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to create vulkan instance - " << resultString(result);
        throw std::runtime_error(msg.str());
    }

    logger_->get()->info("Created vulkan instance with {} extension(s), validation {}",
        enabled.size(), validating_ ? "on" : "off");

    createMessenger();
}

/**
 **/
Instance::~Instance() {
    if (messenger_ != VK_NULL_HANDLE) {
        PFN_vkDestroyDebugUtilsMessengerEXT destroy = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance_, "vkDestroyDebugUtilsMessengerEXT"));
        if (destroy != nullptr) {
            destroy(instance_, messenger_, nullptr);
        }
        messenger_ = VK_NULL_HANDLE;
    }
    if (instance_ != VK_NULL_HANDLE) {
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
    }
}

/**
 **/
VkInstance Instance::handle() const noexcept {
    return instance_;
}

/**
 **/
bool Instance::validating() const noexcept {
    return validating_;
}

/**
 **/
void Instance::createMessenger() {
    if (!validating_) {
        return;
    }

    PFN_vkCreateDebugUtilsMessengerEXT create = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance_, "vkCreateDebugUtilsMessengerEXT"));
    if (create == nullptr) {
        logger_->get()->warn("The vulkan validation layer is enabled but its messenger cannot be created");
        return;
    }

    VkDebugUtilsMessengerCreateInfoEXT info{};
    info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    // info and verbose are left out: they are the layer narrating what it was asked to do
    // rather than telling us anything is wrong, and they bury the two that matter
    info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    info.pfnUserCallback = report;
    info.pUserData = logger_.get();

    VkResult result = create(instance_, &info, nullptr, &messenger_);
    if (result != VK_SUCCESS) {
        logger_->get()->warn("Unable to create the vulkan debug messenger - {}", resultString(result));
        messenger_ = VK_NULL_HANDLE;
    }
}

/**
 **/
bool Instance::hasValidationLayer() {
    uint32_t count = 0;
    if (vkEnumerateInstanceLayerProperties(&count, nullptr) != VK_SUCCESS || count == 0) {
        return false;
    }

    std::vector<VkLayerProperties> available(count);
    if (vkEnumerateInstanceLayerProperties(&count, available.data()) != VK_SUCCESS) {
        return false;
    }

    for (const VkLayerProperties& layer : available) {
        if (std::strcmp(layer.layerName, validationLayer) == 0) {
            return true;
        }
    }
    return false;
}

/**
 **/
bool Instance::hasExtension(const char* extension) {
    uint32_t count = 0;
    if (vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr) != VK_SUCCESS || count == 0) {
        return false;
    }

    std::vector<VkExtensionProperties> available(count);
    if (vkEnumerateInstanceExtensionProperties(nullptr, &count, available.data()) != VK_SUCCESS) {
        return false;
    }

    for (const VkExtensionProperties& candidate : available) {
        if (std::strcmp(candidate.extensionName, extension) == 0) {
            return true;
        }
    }
    return false;
}

/**
 **/
void Instance::requireExtensions(const std::vector<const char*>& extensions) {
    uint32_t count = 0;
    VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
    if (result != VK_SUCCESS && result != VK_INCOMPLETE) {
        std::stringstream msg;
        msg << "Unable to count vulkan instance extensions - " << resultString(result);
        throw std::runtime_error(msg.str());
    }

    std::vector<VkExtensionProperties> available(count);
    if (count > 0) {
        result = vkEnumerateInstanceExtensionProperties(nullptr, &count, available.data());
        if (result != VK_SUCCESS && result != VK_INCOMPLETE) {
            std::stringstream msg;
            msg << "Unable to enumerate vulkan instance extensions - " << resultString(result);
            throw std::runtime_error(msg.str());
        }
    }

    std::stringstream missing;
    size_t missingCount = 0;
    for (const char* extension : extensions) {
        bool found = false;
        for (const VkExtensionProperties& candidate : available) {
            if (std::strcmp(extension, candidate.extensionName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            if (missingCount > 0) {
                missing << ", ";
            }
            missing << extension;
            missingCount++;
        }
    }

    if (missingCount > 0) {
        std::stringstream msg;
        msg << "The vulkan loader is missing required instance extension(s) - " << missing.str();
        throw std::runtime_error(msg.str());
    }
}

};  // namespace v3d::render::realtime::vulkan::device
