#include "VulkanMgr.h"

#include "logger.h"
#include "vulkan/vulkan.h"
#include "imguigameboyx_config.h"

#include <format>

bool VulkanMgr::InitVulkan(std::vector<const char*>& _sdl_enabled_extensions, const std::vector<const char*>& _device_extensions) {
	if (!InitVulkanInstance(_sdl_enabled_extensions)) {
		return false;
	}
	if (!InitPhysicalDevice()) {
		return false;
	}
	if (!InitLogicalDevice(_device_extensions)) {
		return false;
	}

	return true;
}

bool VulkanMgr::InitVulkanInstance(std::vector<const char*>& _sdl_enabled_extensions) {
	uint32_t vk_layer_property_count;
	if (vkEnumerateInstanceLayerProperties(&vk_layer_property_count, nullptr) != VK_SUCCESS) {
		LOG_ERROR("[vulkan] enumerate layer properties");
		return false;
	}
	auto vk_layer_properties = std::vector<VkLayerProperties>(vk_layer_property_count);
	if (vkEnumerateInstanceLayerProperties(&vk_layer_property_count, vk_layer_properties.data()) != VK_SUCCESS) { return false; }

	uint32_t vk_extension_count;
	if (vkEnumerateInstanceExtensionProperties(nullptr, &vk_extension_count, nullptr) != VK_SUCCESS) {
		LOG_ERROR("[vulkan] enumerate extension properties");
		return false;
	}
	auto vk_extension_properties = std::vector<VkExtensionProperties>(vk_extension_count);
	if (vkEnumerateInstanceExtensionProperties(nullptr, &vk_extension_count, vk_extension_properties.data()) != VK_SUCCESS) { return false; }



	VkApplicationInfo vk_app_info = {};
	vk_app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	vk_app_info.pApplicationName = APP_TITLE.c_str();
	vk_app_info.applicationVersion = VK_MAKE_VERSION(GBX_VERSION_MAJOR, GBX_VERSION_MINOR, GBX_VERSION_PATCH);
	vk_app_info.apiVersion = VK_API_VERSION_1_3;

	VkInstanceCreateInfo vk_create_info = {};
	vk_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	vk_create_info.pApplicationInfo = &vk_app_info;
	vk_create_info.enabledLayerCount = (uint32_t)VK_ENABLED_LAYERS.size();
	vk_create_info.ppEnabledLayerNames = VK_ENABLED_LAYERS.data();
	vk_create_info.enabledExtensionCount = (uint32_t)_sdl_enabled_extensions.size();
	vk_create_info.ppEnabledExtensionNames = _sdl_enabled_extensions.data();

	if (vkCreateInstance(&vk_create_info, nullptr, &instance) != VK_SUCCESS) {
		LOG_ERROR("[vulkan] create vulkan instance");
		return false;
	}

	for (const auto& n : VK_ENABLED_LAYERS) {
		LOG_INFO("[vulkan] ", n, " layer enabled");
	}
	for (const auto& n : _sdl_enabled_extensions) {
		LOG_INFO("[vulkan] ", n, " extension enabled");
	}

	return true;
}

bool VulkanMgr::InitPhysicalDevice() {
	uint32_t device_num;
	if (vkEnumeratePhysicalDevices(instance, &device_num, nullptr) != VK_SUCCESS) {
		LOG_ERROR("[vulkan] enumerate physical devices");
		return false;
	}

	if (device_num == 0) {
		LOG_ERROR("[vulkan] no GPUs suported by Vulkan found");
		physical_device = nullptr;
		return false;
	}

	auto vk_physical_devices = std::vector<VkPhysicalDevice>(device_num);
	if (vkEnumeratePhysicalDevices(instance, &device_num, vk_physical_devices.data()) != VK_SUCCESS) { return false; }

	LOG_INFO(device_num, " GPU(s) found:");
	for (const auto& n : vk_physical_devices) {
		VkPhysicalDeviceProperties vk_phys_dev_prop = {};
		vkGetPhysicalDeviceProperties(n, &vk_phys_dev_prop);
		LOG_INFO(vk_phys_dev_prop.deviceName);
	}

	physical_device = vk_physical_devices[0];
	vkGetPhysicalDeviceProperties(vk_physical_devices[0], &phys_dev_properties);
	SetGPUInfo();
	LOG_INFO(vendor, " ", phys_dev_properties.deviceName, " (", driver_version, ") selected");

	return true;
}

bool VulkanMgr::InitLogicalDevice(std::vector<const char*> _device_extensions) {
	uint32_t queue_families_num;
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_families_num, nullptr);
	auto queue_family_properties = std::vector<VkQueueFamilyProperties>(queue_families_num);
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_families_num, queue_family_properties.data());

	uint32_t graphics_queue_index = 0;
	for (uint32_t i = 0; const auto & n : queue_family_properties) {
		if ((n.queueCount > 0) && (n.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
			graphics_queue_index = i;
		}
		i++;
	}

	const float queue_priorities[] = { 1.f };
	VkDeviceQueueCreateInfo queue_create_info = {};
	queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_create_info.queueFamilyIndex = graphics_queue_index;
	queue_create_info.queueCount = 1;
	queue_create_info.pQueuePriorities = queue_priorities;

	VkPhysicalDeviceFeatures vk_enabled_features = {};

	VkDeviceCreateInfo vk_device_info = {};
	vk_device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	vk_device_info.queueCreateInfoCount = 1;
	vk_device_info.pQueueCreateInfos = &queue_create_info;
	vk_device_info.enabledExtensionCount = (uint32_t)_device_extensions.size();
	vk_device_info.ppEnabledExtensionNames = _device_extensions.data();
	vk_device_info.pEnabledFeatures = &vk_enabled_features;


	if (vkCreateDevice(physical_device, &vk_device_info, nullptr, &device) != VK_SUCCESS) {
		LOG_ERROR("[vulkan] create logical device");
		return false;
	}

	family_index = graphics_queue_index;
	vkGetDeviceQueue(device, graphics_queue_index, 0, &queue);

	return true;
}

void VulkanMgr::SetGPUInfo() {
	u16 vendor_id = phys_dev_properties.vendorID & 0xFFFF;
	vendor = get_vendor(vendor_id);

	u32 driver = phys_dev_properties.driverVersion;
	if (vendor_id == ID_NVIDIA) {
		driver_version = std::format("{}.{}", (driver >> 22) & 0x3ff, (driver >> 14) & 0xff);
	}
	else if (vendor_id == ID_AMD) {
		driver_version = std::format("{}.{}", driver >> 14, driver & 0x3fff);
	}
	else {
		driver_version = "";
	}
}

bool VulkanMgr::InitSwapchain(VkImageUsageFlags _flags) {
	VkBool32 supports_present = 0;
	if ((vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, family_index, surface, &supports_present) != VK_SUCCESS) || !supports_present) {
		LOG_ERROR("[vulkan] graphics queue doesn't support present");
		return false;
	}

	uint32_t formats_num;
	if ((vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &formats_num, nullptr) != VK_SUCCESS) || !formats_num) {
		LOG_ERROR("[vulkan] couldn't acquire image formats");
		return false;
	}
	auto image_formats = std::vector<VkSurfaceFormatKHR>(formats_num);
	if (vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &formats_num, image_formats.data()) != VK_SUCCESS) { return swapchain; }

	// TODO: look into formats and which to use
	format = image_formats[0].format;
	VkColorSpaceKHR color_space = image_formats[0].colorSpace;

	VkSurfaceCapabilitiesKHR surface_capabilites;
	if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilites) != VK_SUCCESS) {
		LOG_ERROR("[vulkan] couldn't acquire surface capabilities");
		return false;
	}
	if (surface_capabilites.currentExtent.width == 0xFFFFFFFF) {
		surface_capabilites.currentExtent.width = surface_capabilites.minImageExtent.width;
	}
	if (surface_capabilites.currentExtent.height == 0xFFFFFFFF) {
		surface_capabilites.currentExtent.height = surface_capabilites.minImageExtent.height;
	}
	// unlimited images
	if (surface_capabilites.maxImageCount == 0) {
		surface_capabilites.maxImageCount = 8;
	}

	// TODO: look into swapchain settings
	VkSwapchainCreateInfoKHR swapchain_info;
	swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_info.surface = surface;
	swapchain_info.minImageCount = 3;										// triple buffering
	swapchain_info.imageFormat = format;									// bit depth/format
	swapchain_info.imageColorSpace = color_space;							// used color space
	swapchain_info.imageExtent = surface_capabilites.currentExtent;			// image size
	swapchain_info.imageArrayLayers = 1;									// multiple images at once (e.g. VR)
	swapchain_info.imageUsage = _flags;										// usage of swapchain
	swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;			// sharing between different queues
	swapchain_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;	// no transform
	swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;		// no transparency (opaque)
	swapchain_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;					// simple image fifo (vsync)
	swapchain_info.clipped = VK_FALSE;										// draw only visible window areas if true (clipping)

	if (vkCreateSwapchainKHR(device, &swapchain_info, nullptr, &swapchain) != VK_SUCCESS) {
		LOG_ERROR("[vulkan] couldn't create swapchain");
		return false;
	}

	width = surface_capabilites.currentExtent.width;
	height = surface_capabilites.currentExtent.height;

	uint32_t image_num = 0;
	if (vkGetSwapchainImagesKHR(device, swapchain, &image_num, nullptr) != VK_SUCCESS) {
		LOG_ERROR("[vulkan] couldn't acquire image count");
		return false;
	}

	images.resize(image_num);
	if (vkGetSwapchainImagesKHR(device, swapchain, &image_num, images.data()) != VK_SUCCESS) { return false; }

	return true;
}

bool VulkanMgr::ExitVulkan() {
	if (vkDeviceWaitIdle(device) == VK_SUCCESS) {
		LOG_INFO("[vulkan] GPU wait idle success");
	}
	else {
		LOG_ERROR("[vulkan] GPU wait idle");
	}

	vkDestroyDevice(device, nullptr);
	vkDestroyInstance(instance, nullptr);
	
	return true;
}

void VulkanMgr::DestroySwapchain() {
	if (vkDeviceWaitIdle(device) != VK_SUCCESS) {
		LOG_ERROR("[vulkan] device wait idle");
	}

	vkDestroySwapchainKHR(device, swapchain, nullptr);
	vkDestroySurfaceKHR(instance, surface, 0);
}