#pragma once

#include <vulkan/vulkan.h>
#include <SDL.h>
#include <SDL_vulkan.h>
#include "imgui_impl_vulkan.h"
#include "imgui_impl_sdl2.h"

#include <vector>
#include <string>

class VulkanMgr
{
public:
	explicit VulkanMgr(SDL_Window* _window) : window(_window) {};
	~VulkanMgr() = default;

	void RenderFrame();

	bool InitVulkan(std::vector<const char*>& _sdl_extensions, std::vector<const char*>& _device_extensions);
	bool InitSwapchain(VkImageUsageFlags _flags);
	bool InitSurface();
	bool InitRenderPass();
	bool InitFrameBuffers();
	bool InitCommandBuffers();

	bool InitImGui();

	bool ExitVulkan();
	void DestroySwapchain(); 
	void DestroySurface();
	void DestroyRenderPass();
	void DestroyFrameBuffers();
	void DestroyCommandBuffer();

	void DestroyImgui();

	void WaitIdle();

	bool InitImgui();

	void NextFrameImGui();

private:
	// sdl
	SDL_Window* window;

	// graphics queue
	VkQueue queue = VK_NULL_HANDLE;
	uint32_t familyIndex = (uint32_t) - 1;

	// swapchain
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	uint32_t width;
	uint32_t height;
	VkFormat format;
	VkColorSpaceKHR colorSpace;
	std::vector<VkImage> images;
	uint32_t minImageCount;
	VkPresentModeKHR presentMode;
	std::vector<VkImageView> imageViews;

	// context
	VkSurfaceKHR surface;
	VkInstance instance = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkPhysicalDeviceProperties physicalDeviceProperties;

	// renderpass
	VkRenderPass renderPass;
	VkSampleCountFlagBits sample_count = VK_SAMPLE_COUNT_1_BIT;

	// framebuffer
	std::vector<VkFramebuffer> frameBuffers;

	// buffers
	VkCommandBuffer commandBuffer;
	VkCommandPool commandPool;

	// rendering
	VkFence fence;

	// imgui
	VkDescriptorPool imguiDescriptorPool;

	// gpu info
	std::string vendor;
	std::string driverVersion;

	// misc
	VkClearValue clearColor = { 0.f, 0.f, 0.f, 1.f };

	bool InitVulkanInstance(std::vector<const char*>& _sdl_extensions);
	bool InitPhysicalDevice();
	bool InitLogicalDevice(std::vector<const char*>& _device_extensions);

	void SetGPUInfo();
};