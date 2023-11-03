#pragma once

#include <vulkan/vulkan.h>
#include <SDL.h>
#include <vector>
#include <string>

class VulkanMgr
{
public:
	VulkanMgr(SDL_Window* _window) : window(_window) {};
	~VulkanMgr() = default;

	bool InitVulkan(std::vector<const char*>& _sdl_extensions, const std::vector<const char*>& _device_extensions);
	bool InitSwapchain(VkImageUsageFlags _flags);

	bool ExitVulkan();
	void DestroySwapchain();

	VkSurfaceKHR surface;
	VkInstance instance;

private:
	// sdl
	SDL_Window* window;

	// graphics queue
	VkQueue queue;
	uint32_t family_index;

	// swapchain
	VkSwapchainKHR swapchain;
	uint32_t width;
	uint32_t height;
	VkFormat format;
	std::vector<VkImage> images;

	// context
	VkPhysicalDevice physical_device;
	VkPhysicalDeviceProperties phys_dev_properties;
	VkDevice device;

	// gpu info
	std::string vendor;
	std::string driver_version;

	bool InitVulkanInstance(std::vector<const char*>& _sdl_extensions);
	bool InitPhysicalDevice();
	bool InitLogicalDevice(std::vector<const char*> _device_extensions);

	void SetGPUInfo();
};

