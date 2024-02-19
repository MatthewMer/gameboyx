#pragma once
/* ***********************************************************************************************************
	DESCRIPTION
*********************************************************************************************************** */
/*
*	This class represents the graphics backend (currently jsut Vulkan Graphics API).
*	It manages shaders, framebuffers, commandbuffers, swapchain, etc. (everything related to rendering on the GPU)
*/

#ifdef GRAPHICS_DEBUG
#ifndef VK_DEBUG_CALLBACK
#define VK_DEBUG_CALLBACK
#endif
#endif

#include "GraphicsMgr.h"

#include <vulkan/vulkan.h>
#include <SDL_vulkan.h>
#include <shaderc/shaderc.h>
#include <imgui_impl_vulkan.h>
#include <vector>
#include <thread>
#include <mutex>

#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "ext/matrix_transform.hpp"
#include "gtc/matrix_transform.hpp"
#endif

#define FRAMES_IN_FLIGHT	2

struct VulkanPipelineBufferInfo {
	std::vector<VkVertexInputAttributeDescription> attrDesc;
	std::vector<VkVertexInputBindingDescription> bindDesc;
};

struct vulkan_buffer {
	VkBuffer buffer;					// memory view (offset and size)
	VkDeviceMemory memory;				// allocated memory
};

struct vulkan_image {
	VkImage image;
	VkImageView image_view;
	VkDeviceMemory memory;
};

struct tex2d_data {
	VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

	vulkan_buffer vertex_buffer = {};
	vulkan_buffer index_buffer = {};
	vulkan_image image = {};

	u64 size = {};

	VkPipelineLayout pipeline_layout;
	VkPipeline pipeline;
	std::vector<vulkan_buffer> staging_buffer = std::vector<vulkan_buffer>();

	int update_index = 0;
	alignas(64) std::atomic<bool> submit_cmdbuffer_0 = false;
	alignas(64) std::atomic<bool> submit_cmdbuffer_1 = false;
	alignas(64) std::atomic<bool> submit_cmdbuffer_2 = false;

	std::vector<VkCommandPool> command_pool = std::vector<VkCommandPool>();
	std::vector<VkCommandBuffer> command_buffer = std::vector<VkCommandBuffer>();
	std::vector<VkFence> update_fence = std::vector<VkFence>();

	VkDescriptorPool descriptor_pool = {};
	VkDescriptorSet descriptor_set = {};
	std::vector<VkDescriptorSetLayout> descriptor_set_layout = std::vector<VkDescriptorSetLayout>(1);

	VkSampler sampler = {};

	std::vector<void*> mapped_image_data = std::vector<void*>();

	float scale_x = 1.f;
	float scale_y = 1.f;
	glm::mat4 scale_matrix = {};
};

class GraphicsVulkan : protected GraphicsMgr {
public:
	friend class GraphicsMgr;

	// render
	void RenderFrame() override;

	// (de)init
	bool InitGraphics() override;
	bool ExitGraphics() override;

	// deinit
	bool StartGraphics(bool& _present_mode_fifo, bool& _triple_buffering) override;
	void StopGraphics() override;

	// shader compilation
	void EnumerateShaders() override;
	void CompileNextShader() override;

	// imgui
	bool InitImgui() override;
	void DestroyImgui() override;
	void NextFrameImGui() const override;

	// update 3d/2d data
	void UpdateGpuData() override;

	void SetSwapchainSettings(bool& _present_mode_fifo, bool& _triple_buffering) override;

private:
	// constructor/destructor
	explicit GraphicsVulkan(SDL_Window** _window);
	~GraphicsVulkan() = default;

	// (de)init 2d render target
	bool Init2dGraphicsBackend() override;
	void Destroy2dGraphicsBackend() override;

	// graphics queue
	VkQueue queue = VK_NULL_HANDLE;
	std::thread queueSubmitThread;
	void QueueSubmit();
	alignas(64) std::atomic<bool> submitRunning = true;
	uint32_t familyIndex = (uint32_t)-1;
	VkPhysicalDeviceMemoryProperties devMemProps = {};

	// swapchain
	uint32_t minImageCount = 2;
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE;
	VkFormat swapchainFormat = {};
	VkColorSpaceKHR colorSpace = {};
	std::vector<VkImage> images;
	VkPresentModeKHR presentMode = {};
	std::vector<VkImageView> imageViews;
	
	// context
	VkSurfaceKHR surface = {};
	VkInstance vulkanInstance = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkPhysicalDeviceProperties physicalDeviceProperties = {};
	VkDebugUtilsMessengerEXT debugCallback = nullptr;

	// renderpass
	VkRenderPass renderPass = {};
	VkSampleCountFlagBits sample_count = VK_SAMPLE_COUNT_1_BIT;

	// main buffers
	std::vector<VkFramebuffer> frameBuffers;
	VkCommandBuffer commandBuffers[FRAMES_IN_FLIGHT] = {};
	VkCommandPool commandPools[FRAMES_IN_FLIGHT] = {};

	VkBufferUsageFlags bufferUsageFlags = {};
	VkMemoryPropertyFlags memoryPropertyFlags = {};

	// graphics pipeline
	std::vector<std::string> enumeratedShaderFiles;										// contains all shader source files
	std::vector<std::pair<std::string, std::string>> shaderSourceFiles;					// contains the vertex and fragment shaders in groups of two
	shaderc_compiler_t compiler = {};
	shaderc_compile_options_t options = {};
	std::vector<std::pair<VkPipelineLayout, VkPipeline>> pipelines;
	VkViewport viewport = {};
	VkRect2D scissor = {};

	// main sync
	std::vector<VkFence> renderFences = std::vector<VkFence>(FRAMES_IN_FLIGHT);
	std::vector<VkSemaphore> acquireSemaphores = std::vector<VkSemaphore>(FRAMES_IN_FLIGHT);
	std::vector<VkSemaphore> releaseSemaphores = std::vector<VkSemaphore>(FRAMES_IN_FLIGHT);
	VkPipelineStageFlags waitFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;				// swapchain

	// imgui
	VkDescriptorPool imguiDescriptorPool = {};

	// clear color
	VkClearValue clearColor = { 0.f, 0.f, 0.f, 1.f };

	// render functions
	typedef void (GraphicsVulkan::* update_function)();
	update_function updateFunction = nullptr;
	update_function updateFunctionSubmit = nullptr;
	std::mutex mutQueue;

	void UpdateDummy();

	typedef void (GraphicsVulkan::* render_function)(VkCommandBuffer& _command_buffer);
	render_function bindPipelines = nullptr;
	void BindPipelinesDummy(VkCommandBuffer& _command_buffer);

	// initialize
	bool InitVulkanInstance(std::vector<const char*>& _sdl_extensions);
	bool InitPhysicalDevice();
	bool InitLogicalDevice(std::vector<const char*>& _device_extensions);
	bool InitSwapchain(const VkImageUsageFlags& _flags);
	bool InitSurface();
	bool InitRenderPass();
	bool InitFrameBuffers();
	bool InitCommandBuffers();
	bool InitShaderModule(const std::vector<char>& _byte_code, VkShaderModule& _shader);
	bool InitPipeline(VkShaderModule& _vertex_shader, VkShaderModule& _fragment_shader, VkPipelineLayout& _layout, VkPipeline& _pipeline, VulkanPipelineBufferInfo& _info, std::vector<VkDescriptorSetLayout>& _set_leyouts, std::vector<VkPushConstantRange>& _push_constants);
	void SetGPUInfo();
	bool InitBuffer(vulkan_buffer& _buffer, u64 _size, VkBufferUsageFlags _usage, VkMemoryPropertyFlags _memory_properties);
	bool InitImage(vulkan_image& _image, u32 _width, u32 _height, VkFormat _format, VkImageUsageFlags _usage, VkImageTiling _tiling);
	bool InitSemaphore(VkSemaphore& _semaphore);

	bool LoadBuffer(vulkan_buffer& _buffer, void* _data, u64 _size);

	// deinitialize
	void DestroySwapchain(const bool& _rebuild);
	void DestroySurface();
	void DestroyRenderPass();
	void DestroyFrameBuffers();
	void DestroyCommandBuffer();
	void DestroyPipelines();
	void DestroyBuffer(vulkan_buffer& _buffer);
	void DestroyImage(vulkan_image& _image);
	void DestroySemaphore(VkSemaphore& _semaphore);

	// rebuild -> resize window(surface/render area)
	void RebuildSwapchain();

	// render target 2d texture
	tex2d_data tex2dData = {};

	void UpdateTex2dSubmit();
	void UpdateTex2d() override;
	void RecalcTex2dScaleMatrix() override;

	bool InitTex2dPipeline();
	void DestroyTex2dPipeline();
	void BindPipelines2d(VkCommandBuffer& _command_buffer);
	bool InitTex2dBuffers();
	bool InitTex2dSampler();
	void DestroyTex2dSampler();
	bool InitTex2dDescriptorSets();

	u32 FindMemoryTypes(u32 _type_filter, VkMemoryPropertyFlags _mem_properties);
	void DetectResizableBar() override;

	// sync to gpu (work done)
	void WaitIdle();

#ifdef VK_DEBUG_CALLBACK
	VkDebugUtilsMessengerEXT RegisterDebugCallback();
#endif
};