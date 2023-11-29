#pragma once
/* ***********************************************************************************************************
	DESCRIPTION
*********************************************************************************************************** */
/*
*	This class represents the graphics backend (currently jsut Vulkan Graphics API).
*	It manages shaders, framebuffers, commandbuffers, swapchain, etc. (everything related to rendering on the GPU)
*/

#include <vulkan/vulkan.h>
#include <SDL.h>
#include <SDL_vulkan.h>
#include <shaderc/shaderc.h>
#include "imgui_impl_vulkan.h"
#include "imgui_impl_sdl2.h"
#include "information_structs.h"

#include <vector>
#include <string>

#define FRAMES_IN_FLIGHT	2

class VulkanMgr{
public:
	// get/reset vkInstance
	static VulkanMgr* getInstance(SDL_Window* _window, graphics_information& _graphics_info);
	static void resetInstance();

	// render
	void RenderFrame();

	// initialize
	bool InitGraphics();
	bool StartGraphics();

	// deinit
	bool ExitGraphics();
	void StopGraphics();

	// shader compilation
	void EnumerateShaders();
	void CompileNextShader();

	// imgui
	bool InitImgui();
	void DestroyImgui();
	void NextFrameImGui() const;

	// images (textures)
	VkImage mainImage;
	VkImageView mainImageView;
	VkDeviceMemory mainImageMemory;

private:
	static VulkanMgr* instance;

	explicit VulkanMgr(SDL_Window* _window, graphics_information& _graphics_info)
		: window(_window), graphicsInfo(_graphics_info)
	{};
	~VulkanMgr() = default;

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
	VkInstance vulkanInstance = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkPhysicalDeviceProperties physicalDeviceProperties;

	// renderpass
	VkRenderPass renderPass;
	VkSampleCountFlagBits sample_count = VK_SAMPLE_COUNT_1_BIT;

	// buffers
	std::vector<VkFramebuffer> frameBuffers;
	VkCommandBuffer commandBuffers[FRAMES_IN_FLIGHT];
	VkCommandPool commandPools[FRAMES_IN_FLIGHT];

	// sync
	VkFence fence;
	VkSemaphore acquireSemaphore;
	VkSemaphore releaseSemaphore;
	VkPipelineStageFlags waitFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;				// swapchain

	// graphics pipeline
	std::vector<std::string> enumeratedShaderFiles;										// contains all shader source files
	std::vector<std::pair<std::string, std::string>> shaderSourceFiles;					// contains the vertex and fragment shaders in groups of two
	shaderc_compiler_t compiler;
	shaderc_compile_options_t options;
	VkPipelineLayout mainPipelineLayout;
	VkPipeline mainPipeline;
	std::vector<std::pair<VkPipelineLayout, VkPipeline>> pipelines;
	VkViewport viewport;
	VkRect2D scissor;

	// imgui
	VkDescriptorPool imguiDescriptorPool;

	// gpu info
	std::string vendor;
	std::string driverVersion;

	graphics_information& graphicsInfo;

	// misc
	VkClearValue clearColor = { 0.f, 0.f, 0.f, 1.f };

	// initialize
	bool InitVulkanInstance(std::vector<const char*>& _sdl_extensions);
	bool InitPhysicalDevice();
	bool InitLogicalDevice(std::vector<const char*>& _device_extensions);
	bool InitSwapchain(VkImageUsageFlags _flags);
	bool InitSurface();
	bool InitRenderPass();
	bool InitFrameBuffers();
	bool InitCommandBuffers();
	bool InitMainShader();
	bool InitShaderModule(const std::vector<char>& _byte_code, VkShaderModule& _shader);
	bool InitPipeline(VkShaderModule& _vertex_shader, VkShaderModule& _fragment_shader, VkPipelineLayout& _layout, VkPipeline& _pipeline);
	void SetGPUInfo();

	// deinit
	void DestroySwapchain(const bool& _rebuild);
	void DestroySurface();
	void DestroyRenderPass();
	void DestroyFrameBuffers();
	void DestroyCommandBuffer();
	void DestroyPipelines();
	void DestroyMainShader();

	// rebuild -> resize window(surface/render area)
	void RebuildSwapchain();
	void RebuildPipelines();

	// sync to gpu (work done)
	void WaitIdle();
};