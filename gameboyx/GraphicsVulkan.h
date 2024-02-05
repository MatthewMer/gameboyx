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

struct VulkanBuffer {
	VkBuffer buffer;					// memory view (offset and size)
	VkDeviceMemory memory;				// allocated memory
};

struct VulkanImage {
	VkImage image;
	VkImageView image_view;
	VkDeviceMemory memory;
};

class GraphicsVulkan : protected GraphicsMgr{
public:
	friend class GraphicsMgr;
	
	// render
	void RenderFrame() override;

	// initialize
	bool InitGraphics() override;
	bool StartGraphics() override;

	// deinit
	bool ExitGraphics() override;
	void StopGraphics() override;

	// shader compilation
	void EnumerateShaders() override;
	void CompileNextShader() override;

	// imgui
	bool InitImgui() override;
	void DestroyImgui() override;
	void NextFrameImGui() const override;

	void UpdateGpuData() override;

	bool Init2dGraphicsBackend() override;
	void Destroy2dGraphicsBackend() override;

private:
	explicit GraphicsVulkan(SDL_Window* _window, graphics_information& _graphics_info, game_status& _game_stat) : GraphicsMgr(_window, _graphics_info, _game_stat) {};
	~GraphicsVulkan() = default;

	void RecalcTex2dScaleMatrixInput() override;

	// graphics Queue
	VkQueue queue = VK_NULL_HANDLE;
	uint32_t familyIndex = (uint32_t) - 1;
	VkPhysicalDeviceMemoryProperties devMemProps = {};

	// swapchain
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE;
	VkFormat swapchainFormat = {};
	VkColorSpaceKHR colorSpace = {};
	std::vector<VkImage> images;
	uint32_t minImageCount = 0;
	VkPresentModeKHR presentMode = {};
	std::vector<VkImageView> imageViews;

	// context
	VkSurfaceKHR surface = {};
	VkInstance vulkanInstance = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkPhysicalDeviceProperties physicalDeviceProperties;
	VkDebugUtilsMessengerEXT debugCallback;

	// renderpass
	VkRenderPass renderPass = {};
	VkSampleCountFlagBits sample_count = VK_SAMPLE_COUNT_1_BIT;

	// buffers
	std::vector<VkFramebuffer> frameBuffers;
	VkCommandBuffer commandBuffers[FRAMES_IN_FLIGHT] = {};
	VkCommandPool commandPools[FRAMES_IN_FLIGHT] = {};

	VkBufferUsageFlags bufferUsageFlags = {};
	VkMemoryPropertyFlags memoryPropertyFlags = {};

	typedef void (GraphicsVulkan::* update_function)();
	update_function updateFunction = nullptr;
	void UpdateDummy();

	typedef void (GraphicsVulkan::* render_function)(VkCommandBuffer& _command_buffer);
	render_function bindPipelines = nullptr;
	void BindPipelinesDummy(VkCommandBuffer& _command_buffer);

	// render target 2d texture
	void UpdateTex2d();

	VkFormat tex2dFormat = VK_FORMAT_R8G8B8A8_UNORM;

	VulkanBuffer tex2dVertexBuffer = {};
	VulkanBuffer tex2dIndexBuffer = {};
	VulkanImage tex2dImage = {};

	VkPipelineLayout tex2dPipelineLayout;
	VkPipeline tex2dPipeline;
	std::vector<VulkanBuffer> tex2dStagingBuffer = std::vector<VulkanBuffer>(2);

	int tex2dUpdateIndex = 0;

	std::vector<VkCommandPool> tex2dCommandPool = std::vector<VkCommandPool>(2);
	std::vector<VkCommandBuffer> tex2dCommandBuffer = std::vector<VkCommandBuffer>(2);
	std::vector<VkFence> tex2dUpdateFence = std::vector<VkFence>(2);

	VkDescriptorPool tex2dDescPool = {};
	VkDescriptorSet tex2dDescSet = {};
	std::vector<VkDescriptorSetLayout> tex2dDescSetLayout = std::vector<VkDescriptorSetLayout>(1);

	VkSampler samplerTex2d = {};

	std::vector<void*> mappedImageData = std::vector<void*>(2);

	u64 currentSize = {};

	bool InitTex2dRenderTarget();
	void DestroyTex2dRenderTarget();
	bool InitTex2dPipeline();
	void DestroyTex2dShader();
	void BindPipelines2d(VkCommandBuffer& _command_buffer);
	bool InitTex2dBuffers();
	bool InitTex2dSampler();
	void DestroyTex2dSampler();
	bool InitTex2dDescriptorSets();

	glm::mat4 tex2dScaleMat = {};
	float tex2dScaleX = 1.f;
	float tex2dScaleY = 1.f;

	// sync
	VkFence renderFence = {};
	VkSemaphore acquireSemaphore = {};
	VkSemaphore releaseSemaphore = {};
	VkPipelineStageFlags waitFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;				// swapchain

	// graphics pipeline
	std::vector<std::string> enumeratedShaderFiles;										// contains all shader source files
	std::vector<std::pair<std::string, std::string>> shaderSourceFiles;					// contains the vertex and fragment shaders in groups of two
	shaderc_compiler_t compiler = {};
	shaderc_compile_options_t options = {};
	std::vector<std::pair<VkPipelineLayout, VkPipeline>> pipelines;
	VkViewport viewport = {};
	VkRect2D scissor = {};

	// imgui
	VkDescriptorPool imguiDescriptorPool = {};

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
	bool InitShaderModule(const std::vector<char>& _byte_code, VkShaderModule& _shader);
	bool InitPipeline(VkShaderModule& _vertex_shader, VkShaderModule& _fragment_shader, VkPipelineLayout& _layout, VkPipeline& _pipeline, VulkanPipelineBufferInfo& _info, std::vector<VkDescriptorSetLayout>& _set_leyouts, std::vector<VkPushConstantRange>& _push_constants);
	void SetGPUInfo();
	bool InitBuffer(VulkanBuffer& _buffer, u64 _size, VkBufferUsageFlags _usage, VkMemoryPropertyFlags _memory_properties);
	bool InitImage(VulkanImage& _image, u32 _width, u32 _height, VkFormat _format, VkImageUsageFlags _usage, VkImageTiling _tiling);
	bool InitSemaphore(VkSemaphore& _semaphore);

	bool LoadBuffer(VulkanBuffer& _buffer, void* _data, u64 _size);

	// deinit
	void DestroySwapchain(const bool& _rebuild);
	void DestroySurface();
	void DestroyRenderPass();
	void DestroyFrameBuffers();
	void DestroyCommandBuffer();
	void DestroyPipelines();
	void DestroyBuffer(VulkanBuffer& _buffer);
	void DestroyImage(VulkanImage& _image);
	void DestroySemaphore(VkSemaphore& _semaphore);

	// rebuild -> resize window(surface/render area)
	void RebuildSwapchain();

	u32 FindMemoryTypes(u32 _type_filter, VkMemoryPropertyFlags _mem_properties);
	void DetectResizableBar() override;

	// sync to gpu (work done)
	void WaitIdle();

#ifdef VK_DEBUG_CALLBACK
	VkDebugUtilsMessengerEXT RegisterDebugCallback();
#endif
};