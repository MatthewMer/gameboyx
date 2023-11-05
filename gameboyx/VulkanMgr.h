#pragma once

#include <vulkan/vulkan.h>
#include <SDL.h>
#include <SDL_vulkan.h>
#include <shaderc/shaderc.h>
#include "imgui_impl_vulkan.h"
#include "imgui_impl_sdl2.h"
#include "information_structs.h"

#include <vector>
#include <string>

struct vk_shader_context {
	char* shader_src = nullptr;
	size_t shader_src_size = 0;
	shaderc_shader_kind type;
	char* file_name = nullptr;
	char* entry_point_name = nullptr;
	shaderc_compile_options_t options;
};

class VulkanMgr
{
public:
	explicit VulkanMgr(SDL_Window* _window, graphics_information& _graphics_info) 
		: window(_window), graphicsInfo(_graphics_info)
	{};
	~VulkanMgr() = default;

	void RenderFrame();

	bool InitVulkan(std::vector<const char*>& _sdl_extensions, std::vector<const char*>& _device_extensions);
	bool InitSwapchain(VkImageUsageFlags _flags);
	bool InitSurface();
	bool InitRenderPass();
	bool InitFrameBuffers();
	bool InitCommandBuffers();

	void EnumerateShaders();
	void CompileNextShader();
	bool InitShaderModule();

	bool InitImgui();

	void RebuildSwapchain();

	bool ExitVulkan();
	void DestroySwapchain(const bool& _rebuild);
	void DestroySurface();
	void DestroyRenderPass();
	void DestroyFrameBuffers();
	void DestroyCommandBuffer();

	void DestroyImgui();

	void WaitIdle();

	void NextFrameImGui() const;

private:
	// sdl
	SDL_Window* window;

	// graphics queue
	VkQueue queue = VK_NULL_HANDLE;
	uint32_t familyIndex = (uint32_t) - 1;

	// swapchain
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE;
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

	// buffers
	std::vector<VkFramebuffer> frameBuffers;
	VkCommandBuffer commandBuffer;
	VkCommandPool commandPool;

	// rendering
	VkFence fence;

	// graphics pipeline
	std::vector<std::pair<shaderc_shader_kind, std::string>> shaders;
	std::vector<VkShaderModule> shaderModules;
	shaderc_compiler_t compiler = shaderc_compiler_initialize();

	// imgui
	VkDescriptorPool imguiDescriptorPool;

	// gpu info
	std::string vendor;
	std::string driverVersion;

	graphics_information& graphicsInfo;

	// misc
	VkClearValue clearColor = { 0.f, 0.f, 0.f, 1.f };

	bool InitVulkanInstance(std::vector<const char*>& _sdl_extensions);
	bool InitPhysicalDevice();
	bool InitLogicalDevice(std::vector<const char*>& _device_extensions);

	void SetGPUInfo();
};