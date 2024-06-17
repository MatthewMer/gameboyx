#include "GraphicsVulkan.h"

#include "logger.h"
#include "general_config.h"
#include "helper_functions.h"
#include "data_io.h"
#include "general_config.h"

#include <unordered_map>
#include <format>
#include <iostream>

using namespace std;

#ifdef GRAPHICS_DEBUG
#define VK_VALIDATION "VK_LAYER_KHRONOS_validation"
#endif

namespace Backend {
	namespace Graphics {
		const vector<const char*> VK_ENABLED_LAYERS = {
		#ifdef GRAPHICS_DEBUG
			"VK_LAYER_KHRONOS_validation"
		#endif
		};

		#define VERT_EXT		"vert"
		#define FRAG_EXT		"frag"
		#define COMP_EXT		"comp"
		#define SPIRV_EXT		"spv"

		const unordered_map<string, shaderc_shader_kind> SHADER_TYPES = {
			{VERT_EXT, shaderc_glsl_vertex_shader},
			{FRAG_EXT, shaderc_glsl_fragment_shader},
			{COMP_EXT, shaderc_glsl_compute_shader}
		};

		float vertexData[] = {
			-1.f, 1.f,
			.0f, 1.f,

			1.f, 1.f,
			1.f, 1.f,

			1.f, -1.f,
			1.f, .0f,

			-1.f, -1.f,
			.0f, .0f
		};

		u32 indexData[] = {
			2, 1, 0,
			3, 2, 0
		};

		// main vertex and fragment shader for rendering as byte code
		// created with ./shader/spir_v/print_array.py
		const vector<u8> tex2dVertShader = {
				0x03, 0x02, 0x23, 0x07, 0x00, 0x00, 0x01, 0x00, 0x0b, 0x00, 0x0d, 0x00, 0x2a, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x06, 0x00,
				0x01, 0x00, 0x00, 0x00, 0x47, 0x4c, 0x53, 0x4c, 0x2e, 0x73, 0x74, 0x64, 0x2e, 0x34, 0x35, 0x30,
				0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
				0x0f, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x6d, 0x61, 0x69, 0x6e,
				0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00,
				0x20, 0x00, 0x00, 0x00, 0x03, 0x00, 0x03, 0x00, 0x02, 0x00, 0x00, 0x00, 0xc2, 0x01, 0x00, 0x00,
				0x04, 0x00, 0x0a, 0x00, 0x47, 0x4c, 0x5f, 0x47, 0x4f, 0x4f, 0x47, 0x4c, 0x45, 0x5f, 0x63, 0x70,
				0x70, 0x5f, 0x73, 0x74, 0x79, 0x6c, 0x65, 0x5f, 0x6c, 0x69, 0x6e, 0x65, 0x5f, 0x64, 0x69, 0x72,
				0x65, 0x63, 0x74, 0x69, 0x76, 0x65, 0x00, 0x00, 0x04, 0x00, 0x08, 0x00, 0x47, 0x4c, 0x5f, 0x47,
				0x4f, 0x4f, 0x47, 0x4c, 0x45, 0x5f, 0x69, 0x6e, 0x63, 0x6c, 0x75, 0x64, 0x65, 0x5f, 0x64, 0x69,
				0x72, 0x65, 0x63, 0x74, 0x69, 0x76, 0x65, 0x00, 0x05, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00,
				0x6d, 0x61, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x04, 0x00, 0x09, 0x00, 0x00, 0x00,
				0x6f, 0x75, 0x74, 0x5f, 0x75, 0x76, 0x73, 0x00, 0x05, 0x00, 0x04, 0x00, 0x0b, 0x00, 0x00, 0x00,
				0x69, 0x6e, 0x5f, 0x75, 0x76, 0x73, 0x00, 0x00, 0x05, 0x00, 0x06, 0x00, 0x10, 0x00, 0x00, 0x00,
				0x6d, 0x6f, 0x64, 0x65, 0x6c, 0x5f, 0x76, 0x69, 0x65, 0x77, 0x5f, 0x70, 0x72, 0x6f, 0x6a, 0x00,
				0x05, 0x00, 0x06, 0x00, 0x11, 0x00, 0x00, 0x00, 0x70, 0x75, 0x73, 0x68, 0x43, 0x6f, 0x6e, 0x73,
				0x74, 0x61, 0x6e, 0x74, 0x73, 0x00, 0x00, 0x00, 0x06, 0x00, 0x07, 0x00, 0x11, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x6d, 0x6f, 0x64, 0x65, 0x6c, 0x5f, 0x76, 0x69, 0x65, 0x77, 0x5f, 0x70,
				0x72, 0x6f, 0x6a, 0x00, 0x05, 0x00, 0x05, 0x00, 0x13, 0x00, 0x00, 0x00, 0x70, 0x75, 0x73, 0x68,
				0x5f, 0x63, 0x6f, 0x6e, 0x73, 0x74, 0x00, 0x00, 0x05, 0x00, 0x06, 0x00, 0x1c, 0x00, 0x00, 0x00,
				0x67, 0x6c, 0x5f, 0x50, 0x65, 0x72, 0x56, 0x65, 0x72, 0x74, 0x65, 0x78, 0x00, 0x00, 0x00, 0x00,
				0x06, 0x00, 0x06, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x67, 0x6c, 0x5f, 0x50,
				0x6f, 0x73, 0x69, 0x74, 0x69, 0x6f, 0x6e, 0x00, 0x06, 0x00, 0x07, 0x00, 0x1c, 0x00, 0x00, 0x00,
				0x01, 0x00, 0x00, 0x00, 0x67, 0x6c, 0x5f, 0x50, 0x6f, 0x69, 0x6e, 0x74, 0x53, 0x69, 0x7a, 0x65,
				0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x07, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
				0x67, 0x6c, 0x5f, 0x43, 0x6c, 0x69, 0x70, 0x44, 0x69, 0x73, 0x74, 0x61, 0x6e, 0x63, 0x65, 0x00,
				0x06, 0x00, 0x07, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x67, 0x6c, 0x5f, 0x43,
				0x75, 0x6c, 0x6c, 0x44, 0x69, 0x73, 0x74, 0x61, 0x6e, 0x63, 0x65, 0x00, 0x05, 0x00, 0x03, 0x00,
				0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00,
				0x69, 0x6e, 0x5f, 0x70, 0x6f, 0x73, 0x69, 0x74, 0x69, 0x6f, 0x6e, 0x00, 0x47, 0x00, 0x04, 0x00,
				0x09, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00,
				0x0b, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x48, 0x00, 0x04, 0x00,
				0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00,
				0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x48, 0x00, 0x05, 0x00, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
				0x10, 0x00, 0x00, 0x00, 0x47, 0x00, 0x03, 0x00, 0x11, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
				0x48, 0x00, 0x05, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
				0x0b, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00, 0x1c, 0x00, 0x00, 0x00,
				0x02, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00,
				0x1c, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
				0x47, 0x00, 0x03, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00,
				0x20, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0x00, 0x02, 0x00,
				0x02, 0x00, 0x00, 0x00, 0x21, 0x00, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
				0x16, 0x00, 0x03, 0x00, 0x06, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00,
				0x07, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00,
				0x08, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00,
				0x08, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00,
				0x0a, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00,
				0x0a, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00,
				0x0d, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x18, 0x00, 0x04, 0x00,
				0x0e, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00,
				0x0f, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x03, 0x00,
				0x11, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00,
				0x09, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00,
				0x13, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x15, 0x00, 0x04, 0x00, 0x14, 0x00, 0x00, 0x00,
				0x20, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x14, 0x00, 0x00, 0x00,
				0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x16, 0x00, 0x00, 0x00,
				0x09, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x15, 0x00, 0x04, 0x00, 0x19, 0x00, 0x00, 0x00,
				0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x19, 0x00, 0x00, 0x00,
				0x1a, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x04, 0x00, 0x1b, 0x00, 0x00, 0x00,
				0x06, 0x00, 0x00, 0x00, 0x1a, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x06, 0x00, 0x1c, 0x00, 0x00, 0x00,
				0x0d, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x00, 0x00,
				0x20, 0x00, 0x04, 0x00, 0x1d, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00,
				0x3b, 0x00, 0x04, 0x00, 0x1d, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
				0x3b, 0x00, 0x04, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
				0x2b, 0x00, 0x04, 0x00, 0x06, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x2b, 0x00, 0x04, 0x00, 0x06, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3f,
				0x20, 0x00, 0x04, 0x00, 0x28, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00,
				0x36, 0x00, 0x05, 0x00, 0x02, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x03, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x05, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00,
				0x0f, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00,
				0x07, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00,
				0x09, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x41, 0x00, 0x05, 0x00, 0x16, 0x00, 0x00, 0x00,
				0x17, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00,
				0x0e, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x17, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00,
				0x10, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x0e, 0x00, 0x00, 0x00,
				0x1f, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x07, 0x00, 0x00, 0x00,
				0x21, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x51, 0x00, 0x05, 0x00, 0x06, 0x00, 0x00, 0x00,
				0x24, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x51, 0x00, 0x05, 0x00,
				0x06, 0x00, 0x00, 0x00, 0x25, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
				0x50, 0x00, 0x07, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x26, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00,
				0x25, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x91, 0x00, 0x05, 0x00,
				0x0d, 0x00, 0x00, 0x00, 0x27, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x26, 0x00, 0x00, 0x00,
				0x41, 0x00, 0x05, 0x00, 0x28, 0x00, 0x00, 0x00, 0x29, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00,
				0x15, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00, 0x29, 0x00, 0x00, 0x00, 0x27, 0x00, 0x00, 0x00,
				0xfd, 0x00, 0x01, 0x00, 0x38, 0x00, 0x01, 0x00
		};

		const vector<u8> tex2dFragShader = {
				0x03, 0x02, 0x23, 0x07, 0x00, 0x00, 0x01, 0x00, 0x0b, 0x00, 0x0d, 0x00, 0x17, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x06, 0x00,
				0x01, 0x00, 0x00, 0x00, 0x47, 0x4c, 0x53, 0x4c, 0x2e, 0x73, 0x74, 0x64, 0x2e, 0x34, 0x35, 0x30,
				0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
				0x0f, 0x00, 0x07, 0x00, 0x04, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x6d, 0x61, 0x69, 0x6e,
				0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x10, 0x00, 0x03, 0x00,
				0x04, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x03, 0x00, 0x03, 0x00, 0x02, 0x00, 0x00, 0x00,
				0xc2, 0x01, 0x00, 0x00, 0x04, 0x00, 0x0a, 0x00, 0x47, 0x4c, 0x5f, 0x47, 0x4f, 0x4f, 0x47, 0x4c,
				0x45, 0x5f, 0x63, 0x70, 0x70, 0x5f, 0x73, 0x74, 0x79, 0x6c, 0x65, 0x5f, 0x6c, 0x69, 0x6e, 0x65,
				0x5f, 0x64, 0x69, 0x72, 0x65, 0x63, 0x74, 0x69, 0x76, 0x65, 0x00, 0x00, 0x04, 0x00, 0x08, 0x00,
				0x47, 0x4c, 0x5f, 0x47, 0x4f, 0x4f, 0x47, 0x4c, 0x45, 0x5f, 0x69, 0x6e, 0x63, 0x6c, 0x75, 0x64,
				0x65, 0x5f, 0x64, 0x69, 0x72, 0x65, 0x63, 0x74, 0x69, 0x76, 0x65, 0x00, 0x05, 0x00, 0x04, 0x00,
				0x04, 0x00, 0x00, 0x00, 0x6d, 0x61, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x05, 0x00,
				0x09, 0x00, 0x00, 0x00, 0x74, 0x65, 0x78, 0x5f, 0x73, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x00, 0x00,
				0x05, 0x00, 0x06, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x69, 0x6e, 0x5f, 0x74, 0x65, 0x78, 0x74, 0x75,
				0x72, 0x65, 0x32, 0x64, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x04, 0x00, 0x11, 0x00, 0x00, 0x00,
				0x69, 0x6e, 0x5f, 0x75, 0x76, 0x73, 0x00, 0x00, 0x05, 0x00, 0x05, 0x00, 0x15, 0x00, 0x00, 0x00,
				0x6f, 0x75, 0x74, 0x5f, 0x63, 0x6f, 0x6c, 0x6f, 0x72, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00,
				0x0d, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00,
				0x0d, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00,
				0x11, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00,
				0x15, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0x00, 0x02, 0x00,
				0x02, 0x00, 0x00, 0x00, 0x21, 0x00, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
				0x16, 0x00, 0x03, 0x00, 0x06, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00,
				0x07, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00,
				0x08, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x19, 0x00, 0x09, 0x00,
				0x0a, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x1b, 0x00, 0x03, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00,
				0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00,
				0x0c, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00,
				0x0f, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00,
				0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00,
				0x10, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00,
				0x14, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00,
				0x14, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x36, 0x00, 0x05, 0x00,
				0x02, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
				0xf8, 0x00, 0x02, 0x00, 0x05, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x08, 0x00, 0x00, 0x00,
				0x09, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x0b, 0x00, 0x00, 0x00,
				0x0e, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x0f, 0x00, 0x00, 0x00,
				0x12, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x57, 0x00, 0x05, 0x00, 0x07, 0x00, 0x00, 0x00,
				0x13, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00,
				0x09, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x07, 0x00, 0x00, 0x00,
				0x16, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00, 0x15, 0x00, 0x00, 0x00,
				0x16, 0x00, 0x00, 0x00, 0xfd, 0x00, 0x01, 0x00, 0x38, 0x00, 0x01, 0x00
		};

		bool compile_shader(vector<char>& _byte_code, const string& _shader_source_file, const shaderc_compiler_t& _compiler, const shaderc_compile_options_t& _options);
		#ifdef VK_DEBUG_CALLBACK
		VkBool32 VKAPI_CALL debug_report_callback(VkDebugUtilsMessageSeverityFlagBitsEXT _severity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* _callback_data, void* userData);
		#endif

		GraphicsVulkan::GraphicsVulkan(SDL_Window** _window) {
			auto window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
			*_window = SDL_CreateWindow(Config::APP_TITLE.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, GUI_WIN_WIDTH, GUI_WIN_HEIGHT, window_flags);
			window = *_window;

			if (!window) {
				LOG_ERROR("[vulkan]", SDL_GetError());
			} else {
				LOG_INFO("[vulkan] window created");
			}
		};

		bool GraphicsVulkan::InitGraphics() {
		#ifdef VK_DEBUG_CALLBACK
			vector<const char*> additional_extensions = {
				VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
				VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME
			};
		#endif

			uint32_t sdl_extension_count;
			SDL_Vulkan_GetInstanceExtensions(window, &sdl_extension_count, nullptr);
			auto sdl_extensions = vector<const char*>(sdl_extension_count);
			SDL_Vulkan_GetInstanceExtensions(window, &sdl_extension_count, sdl_extensions.data());
		#ifdef VK_DEBUG_CALLBACK
			sdl_extensions.insert(sdl_extensions.end(), additional_extensions.begin(), additional_extensions.end());
		#endif

			vector<const char*> device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

			if (!InitVulkanInstance(sdl_extensions)) {
				return false;
			}
			if (!InitPhysicalDevice()) {
				return false;
			}
			if (!InitLogicalDevice(device_extensions)) {
				return false;
			}

			return true;
		}

		bool GraphicsVulkan::ExitGraphics() {
			WaitIdle();

			vkDestroyDevice(device, nullptr);
		#ifdef VK_DEBUG_CALLBACK
			if (debugCallback) {
				PFN_vkDestroyDebugUtilsMessengerEXT pfnDestroyDebugUtilsMessengerEXT;
				pfnDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vulkanInstance, "vkDestroyDebugUtilsMessengerEXT");
				pfnDestroyDebugUtilsMessengerEXT(vulkanInstance, debugCallback, nullptr);
				debugCallback = nullptr;
			}
		#endif
			vkDestroyInstance(vulkanInstance, nullptr);

			return true;
		}

		bool GraphicsVulkan::StartGraphics(bool& _present_mode_fifo, bool& _triple_buffering) {
			if (!InitSurface()) {
				return false;
			}

			presentMode = (_present_mode_fifo ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR);
			minImageCount = (_triple_buffering ? 3 : 2);

			if (!InitSwapchain(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)) {
				return false;
			}
			if (!InitRenderPass()) {
				return false;
			}
			if (!InitFrameBuffers()) {
				return false;
			}
			if (!InitCommandBuffers()) {
				return false;
			}

			bindPipelines = &GraphicsVulkan::BindPipelinesDummy;
			updateFunction = &GraphicsVulkan::UpdateDummy;

			_present_mode_fifo = (presentMode == VK_PRESENT_MODE_FIFO_KHR ? true : false);
			_triple_buffering = (minImageCount == 3 ? true : false);

			return true;
		}

		void GraphicsVulkan::StopGraphics() {
			DestroyCommandBuffer();
			DestroyFrameBuffers();
			DestroyPipelines();
			DestroyRenderPass();
			DestroySwapchain(false);
			DestroySurface();
		}

		void GraphicsVulkan::SetSwapchainSettings(bool& _present_mode_fifo, bool& _triple_buffering) {
			presentMode = (_present_mode_fifo ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR);
			if (presentMode == VK_PRESENT_MODE_FIFO_KHR) {
				minImageCount = 2;
			}
			else {
				minImageCount = (_triple_buffering ? 3 : 2);
			}

			unique_lock<mutex> lock_queue(mutQueue);
			RebuildSwapchain();
			lock_queue.unlock();

			_present_mode_fifo = (presentMode == VK_PRESENT_MODE_FIFO_KHR ? true : false);
			_triple_buffering = (minImageCount == 3 ? true : false);
		}

		bool GraphicsVulkan::Init2dGraphicsBackend() {
			for (int i = 0; i < FRAMES_IN_FLIGHT_2D; i++) {
				VkCommandPoolCreateInfo cmd_pool_info = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
				cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
				cmd_pool_info.queueFamilyIndex = familyIndex;
				tex2dData.command_pool.emplace_back();
				if (vkCreateCommandPool(device, &cmd_pool_info, nullptr, &tex2dData.command_pool[i]) != VK_SUCCESS) {
					LOG_ERROR("[vulkan] create command pool for tex2d update");
				}

				VkCommandBufferAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
				alloc_info.commandPool = tex2dData.command_pool[i];
				alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				alloc_info.commandBufferCount = 1;
				tex2dData.command_buffer.emplace_back();
				if (vkAllocateCommandBuffers(device, &alloc_info, &tex2dData.command_buffer[i]) != VK_SUCCESS) {
					LOG_ERROR("[vulkan] init command buffer for tex2d update");
				}
			}

			// vertex buffer for triangle
			if (!InitBuffer(tex2dData.vertex_buffer, sizeof(vertexData), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | bufferUsageFlags, memoryPropertyFlags)) {
				LOG_ERROR("[vulkan] init vertex buffer");
				return false;
			}
			if (!LoadBuffer(tex2dData.vertex_buffer, vertexData, sizeof(vertexData))) {
				LOG_ERROR("[vulkan] copy data to vertex buffer");
				return false;
			}

			// index buffer for triangle
			if (!InitBuffer(tex2dData.index_buffer, sizeof(indexData), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | bufferUsageFlags, memoryPropertyFlags)) {
				LOG_ERROR("[vulkan] init index buffer");
				return false;
			}
			if (!LoadBuffer(tex2dData.index_buffer, indexData, sizeof(indexData))) {
				LOG_ERROR("[vulkan] copy data to index buffer");
				return false;
			}

			VkFenceCreateInfo fence_info = {};
			fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
			for (int i = 0; i < FRAMES_IN_FLIGHT_2D; i++) {
				tex2dData.update_fence.emplace_back();
				if (vkCreateFence(device, &fence_info, nullptr, &tex2dData.update_fence[i]) != VK_SUCCESS) {
					LOG_ERROR("[vulkan] create tex2dUpdateFence");
					return false;
				}
			}

			if (!InitTex2dBuffers()) { return false; }

			if (!InitTex2dSampler()) { return false; }

			if (!InitTex2dDescriptorSets()) { return false; }

			// shader to present 2d texture
			if (!InitTex2dPipeline()) {
				LOG_ERROR("[vulkan] init shader for 2d texture output");
				return false;
			}

			RecalcTex2dScaleMatrix();

			bindPipelines = &GraphicsVulkan::BindPipelines2d;
			updateFunction = &GraphicsVulkan::UpdateTex2d;

			submitRunning.store(true);
			queueSubmitThread = thread([this]() -> void { QueueSubmit(); });
			if (!queueSubmitThread.joinable()) {
				LOG_ERROR("[vulkan] create queue submit thread");
			}

			UpdateTex2d();

			LOG_INFO("[vulkan] 2d graphics backend initialized");

			return true;
		}

		void GraphicsVulkan::Destroy2dGraphicsBackend() {
			submitRunning.store(false);
			queueNotify.notify_one();
			if (queueSubmitThread.joinable()) {
				queueSubmitThread.join();
			}

			WaitIdle();
			DestroyTex2dSampler();
			DestroyTex2dPipeline();
			vkDestroyDescriptorPool(device, tex2dData.descriptor_pool, nullptr);
			vkDestroyDescriptorSetLayout(device, tex2dData.descriptor_set_layout[0], nullptr);
			for (auto& n : tex2dData.staging_buffer) {
				DestroyBuffer(n);
			}
			tex2dData.staging_buffer.clear();
			WaitIdle();
			DestroyImage(tex2dData.image);
			DestroyBuffer(tex2dData.vertex_buffer);
			DestroyBuffer(tex2dData.index_buffer);
			for (auto& n : tex2dData.command_pool) {
				vkDestroyCommandPool(device, n, nullptr);
			}
			tex2dData.command_pool.clear();
			WaitIdle();
			for (auto& n : tex2dData.update_fence) {
				vkDestroyFence(device, n, nullptr);
			}
			tex2dData.update_fence.clear();

			updateFunction = &GraphicsVulkan::UpdateDummy;
			bindPipelines = &GraphicsVulkan::BindPipelinesDummy;

			LOG_INFO("[vulkan] 2d graphics backend stopped");
		}

		void GraphicsVulkan::RenderFrame() {
			u32 image_index = 0;
			static u32 frame_index = 0;
			static bool rebuild = false;

			// wait for fence that signals processing of submitted command buffer finished
			if (vkWaitForFences(device, 1, &renderFences[frame_index], VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
				LOG_ERROR("[vulkan] wait for fences");
				return;
			}

			if (VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, acquireSemaphores[frame_index], 0, &image_index); result != VK_SUCCESS) {
				if (result == VK_SUBOPTIMAL_KHR) {
					rebuild = true;
				} else {
					LOG_ERROR("[vulkan] acquire image from swapchain");
					return;
				}
			}

			if (vkResetFences(device, 1, &renderFences[frame_index]) != VK_SUCCESS) {
				LOG_ERROR("[vulkan] reset fences");
			}

			if (vkResetCommandPool(device, commandPools[frame_index], 0) != VK_SUCCESS) {
				LOG_ERROR("[vulkan] reset command pool");
			}

			VkCommandBufferBeginInfo commandbuffer_info = {};
			commandbuffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			commandbuffer_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			{
				VkCommandBuffer& commandBuffer = commandBuffers[frame_index];
				if (vkBeginCommandBuffer(commandBuffer, &commandbuffer_info) != VK_SUCCESS) {
					LOG_ERROR("[vulkan] begin command buffer");
				}

				// render commands
				VkRenderPassBeginInfo begin_info = {};
				begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				begin_info.renderPass = renderPass;
				begin_info.framebuffer = frameBuffers[image_index];
				begin_info.renderArea = { {0, 0}, {win_width, win_height} };
				begin_info.clearValueCount = 1;
				begin_info.pClearValues = &clearColor;
				vkCmdBeginRenderPass(commandBuffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);

				(this->*bindPipelines)(commandBuffer);

				// imgui -> last
				ImGui::Render();
				ImDrawData* drawData = ImGui::GetDrawData();
				ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);

				vkCmdEndRenderPass(commandBuffer);

				if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
					LOG_ERROR("[vulkan] end command buffer");
				}
			}

			// submit buffer to queue
			VkSubmitInfo submit_info = {};
			submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submit_info.commandBufferCount = 1;
			submit_info.pCommandBuffers = &commandBuffers[frame_index];
			submit_info.waitSemaphoreCount = 1;
			submit_info.pWaitSemaphores = &acquireSemaphores[frame_index];
			submit_info.pWaitDstStageMask = &waitFlags;
			submit_info.signalSemaphoreCount = 1;
			submit_info.pSignalSemaphores = &releaseSemaphores[frame_index];
			{
				unique_lock<mutex> lock_queue(mutQueue);
				if (vkQueueSubmit(queue, 1, &submit_info, renderFences[frame_index]) != VK_SUCCESS) {
					LOG_ERROR("[vulkan] submit command buffer to queue");
				}

				// present
				VkPresentInfoKHR present_info = {};
				present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
				present_info.pSwapchains = &swapchain;
				present_info.swapchainCount = 1;
				present_info.pImageIndices = &image_index;
				present_info.waitSemaphoreCount = 1;
				present_info.pWaitSemaphores = &releaseSemaphores[frame_index];
				if (VkResult result = vkQueuePresentKHR(queue, &present_info); result != VK_SUCCESS) {
					if (rebuild || result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
						RebuildSwapchain();
						RecalcTex2dScaleMatrix();
						rebuild = false;
					}
					else {
						LOG_ERROR("[vulkan] present result");
					}
				}
			}

			++frame_index %= FRAMES_IN_FLIGHT;
		}

		void GraphicsVulkan::QueueSubmit() {
			while (submitRunning.load()) {
				unique_lock<mutex> lock_submit(mutSubmit);
				queueNotify.wait(lock_submit);

				for (auto& n : queueSubmitData) {
					VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
					submitInfo.commandBufferCount = 1;
					submitInfo.pCommandBuffers = get<0>(n);

					std::unique_lock<mutex> lock_queue(mutQueue);
					if (vkQueueSubmit(queue, 1, &submitInfo, *get<1>(n)) != VK_SUCCESS) {
						LOG_ERROR("[vulkan] queue submit texture2d update");
					}
					get<2>(n)->store(true);
				}
				queueSubmitData.clear();
			}
		}

		void GraphicsVulkan::BindPipelinesDummy(VkCommandBuffer& _command_buffer) {
			return;
		}

		void GraphicsVulkan::UpdateDummy() {
			return;
		}

		void GraphicsVulkan::BindPipelines2d(VkCommandBuffer& _command_buffer) {
			vkCmdBindPipeline(_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tex2dData.pipeline);
			VkDeviceSize offset = 0;
			vkCmdBindVertexBuffers(_command_buffer, 0, 1, &tex2dData.vertex_buffer.buffer, &offset);
			vkCmdBindIndexBuffer(_command_buffer, tex2dData.index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdBindDescriptorSets(_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tex2dData.pipeline_layout, 0, 1, &tex2dData.descriptor_set, 0, nullptr);
			vkCmdPushConstants(_command_buffer, tex2dData.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &tex2dData.scale_matrix);
			vkCmdSetViewport(_command_buffer, 0, 1, &viewport);
			vkCmdSetScissor(_command_buffer, 0, 1, &scissor);
			vkCmdDrawIndexed(_command_buffer, sizeof(indexData) / sizeof(u32), 1, 0, 0, 0);
		}

		void GraphicsVulkan::RecalcTex2dScaleMatrix() {
			if (aspectRatio > virtGraphicsInfo.aspect_ratio) {
				tex2dData.scale_x = virtGraphicsInfo.aspect_ratio / aspectRatio;
				tex2dData.scale_y = 1.f;
			} else {
				tex2dData.scale_x = 1.f;
				tex2dData.scale_y = (1.f / virtGraphicsInfo.aspect_ratio) / (1.f / aspectRatio);
			}

			tex2dData.scale_matrix = glm::scale(glm::mat4(1.f), glm::vec3(tex2dData.scale_x, tex2dData.scale_y, 1.f));
		}

		void GraphicsVulkan::UpdateGpuData() {
			(this->*updateFunction)();
		}

		void GraphicsVulkan::UpdateTex2d() {
			int& update_index = tex2dData.update_index;
			std::atomic<bool>* signal = tex2dData.cmdbufSubmitSignals[update_index];

			if (signal->load()) {
				VkResult result = vkWaitForFences(device, 1, &tex2dData.update_fence[update_index], VK_TRUE, 0);
				switch (result) {
				case VK_TIMEOUT:
					return;
					break;
				case VK_SUCCESS:
					break;
				default:
					LOG_ERROR("[vulkan] wait for texture2d update fence");
					return;
					break;
				}

				if (vkResetFences(device, 1, &tex2dData.update_fence[update_index]) != VK_SUCCESS) {
					LOG_ERROR("[vulkan] reset texture2d update fence");
				}

				memcpy(tex2dData.mapped_image_data[update_index], virtGraphicsInfo.image_data->data(), virtGraphicsInfo.image_data->size());

				if (vkResetCommandPool(device, tex2dData.command_pool[update_index], 0) != VK_SUCCESS) {
					LOG_ERROR("[vulkan] reset texture2d command pool");
				}

				VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
				beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
				if (vkBeginCommandBuffer(tex2dData.command_buffer[update_index], &beginInfo) != VK_SUCCESS) {
					LOG_ERROR("[vulkan] begin command buffer texture2d update");
				}

				// synchronize texture upload to shader stages -> shader stage TRANSFER with corresponding access mask for TRANSFER (L2 Cache) 
				// to make sure the copy is not interfering with the fragment shader read and image is in proper layout and memory location for update
				{
					VkImageMemoryBarrier imageBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
					imageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
					imageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
					imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					imageBarrier.image = tex2dData.image.image;
					imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					imageBarrier.subresourceRange.levelCount = 1;
					imageBarrier.subresourceRange.layerCount = 1;
					imageBarrier.srcAccessMask = VK_ACCESS_NONE;
					imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					vkCmdPipelineBarrier(tex2dData.command_buffer[update_index], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);
				}

				VkBufferImageCopy region = {};
				region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				region.imageSubresource.layerCount = 1;
				region.imageExtent = { virtGraphicsInfo.lcd_width, virtGraphicsInfo.lcd_height, 1 };
				vkCmdCopyBufferToImage(tex2dData.command_buffer[update_index], tex2dData.staging_buffer[update_index].buffer, tex2dData.image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

				{
					VkImageMemoryBarrier imageBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
					imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
					imageBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					imageBarrier.image = tex2dData.image.image;
					imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					imageBarrier.subresourceRange.levelCount = 1;
					imageBarrier.subresourceRange.layerCount = 1;
					imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
					vkCmdPipelineBarrier(tex2dData.command_buffer[update_index], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);
				}

				if (vkEndCommandBuffer(tex2dData.command_buffer[update_index]) != VK_SUCCESS) {
					LOG_ERROR("[vulkan] end command buffer texture2d update");
				}

				signal->store(false);
				unique_lock<mutex> lock_submit(mutSubmit);
				queueSubmitData.emplace_back(&tex2dData.command_buffer[update_index], &tex2dData.update_fence[update_index], signal);
				queueNotify.notify_one();

				++update_index %= FRAMES_IN_FLIGHT_2D;
			}
		}

		bool GraphicsVulkan::InitVulkanInstance(std::vector<const char*>& _sdl_extensions) {
			u32 vk_layer_property_count;
			if (vkEnumerateInstanceLayerProperties(&vk_layer_property_count, nullptr) != VK_SUCCESS) {
				LOG_ERROR("[vulkan] enumerate layer properties");
				return false;
			}
			auto vk_layer_properties = std::vector<VkLayerProperties>(vk_layer_property_count);
			if (vkEnumerateInstanceLayerProperties(&vk_layer_property_count, vk_layer_properties.data()) != VK_SUCCESS) { return false; }

			u32 vk_extension_count;
			if (vkEnumerateInstanceExtensionProperties(nullptr, &vk_extension_count, nullptr) != VK_SUCCESS) {
				LOG_ERROR("[vulkan] enumerate extension properties");
				return false;
			}
			auto vk_extension_properties = std::vector<VkExtensionProperties>(vk_extension_count);
			if (vkEnumerateInstanceExtensionProperties(nullptr, &vk_extension_count, vk_extension_properties.data()) != VK_SUCCESS) { return false; }

			// only for debug
		#ifdef VK_DEBUG_CALLBACK
			vector<VkValidationFeatureEnableEXT>  validation_features_list = {
				//VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
				VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
				VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT
			};
			VkValidationFeaturesEXT validation_features = { VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT };
			validation_features.enabledValidationFeatureCount = (u32)validation_features_list.size();
			validation_features.pEnabledValidationFeatures = validation_features_list.data();
		#endif

			VkApplicationInfo vk_app_info = {};
			vk_app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
			vk_app_info.pApplicationName = Config::APP_TITLE.c_str();
			vk_app_info.applicationVersion = VK_MAKE_VERSION(GBX_VERSION_MAJOR, GBX_VERSION_MINOR, GBX_VERSION_PATCH);
			vk_app_info.apiVersion = VK_API_VERSION_1_0;

			VkInstanceCreateInfo vk_create_info = {};
			vk_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		#ifdef VK_DEBUG_CALLBACK
			vk_create_info.pNext = &validation_features;
		#endif
			vk_create_info.pApplicationInfo = &vk_app_info;
			vk_create_info.enabledLayerCount = (u32)VK_ENABLED_LAYERS.size();
			vk_create_info.ppEnabledLayerNames = VK_ENABLED_LAYERS.data();
			vk_create_info.enabledExtensionCount = (u32)_sdl_extensions.size();
			vk_create_info.ppEnabledExtensionNames = _sdl_extensions.data();

			if (vkCreateInstance(&vk_create_info, nullptr, &vulkanInstance) != VK_SUCCESS) {
				LOG_ERROR("[vulkan] create vulkan vulkanInstance");
				return false;
			}

		#ifdef VK_DEBUG_CALLBACK
			debugCallback = RegisterDebugCallback();
		#endif

			for (const auto& n : VK_ENABLED_LAYERS) {
		#ifdef GRAPHICS_DEBUG
				if (strcmp(n, VK_VALIDATION) == 0) {
					LOG_WARN("[vulkan] ", n, " layer enabled");
				} else {
					LOG_INFO("[vulkan] ", n, " layer enabled");
				}
		#else
				LOG_INFO("[vulkan] ", n, " layer enabled");
		#endif
			}
			for (const auto& n : _sdl_extensions) {
				LOG_INFO("[vulkan] ", n, " extension enabled");
			}

			return true;
		}

		bool GraphicsVulkan::InitPhysicalDevice() {
			u32 device_num;
			if (vkEnumeratePhysicalDevices(vulkanInstance, &device_num, nullptr) != VK_SUCCESS) {
				LOG_ERROR("[vulkan] enumerate physical devices");
				return false;
			}

			if (device_num == 0) {
				LOG_ERROR("[vulkan] no GPUs supported by Vulkan found");
				physicalDevice = nullptr;
				return false;
			}

			auto vk_physical_devices = std::vector<VkPhysicalDevice>(device_num);
			if (vkEnumeratePhysicalDevices(vulkanInstance, &device_num, vk_physical_devices.data()) != VK_SUCCESS) { return false; }

			int discrete_device_index = -1;
			LOG_INFO("[vulkan] ", device_num, " GPU(s) found:");
			for (int i = 0; const auto & n : vk_physical_devices) {
				VkPhysicalDeviceProperties vk_phys_dev_prop = {};
				vkGetPhysicalDeviceProperties(n, &vk_phys_dev_prop);
				VkPhysicalDeviceFeatures vk_phys_dev_feat = {};
				vkGetPhysicalDeviceFeatures(n, &vk_phys_dev_feat);
				if (vk_phys_dev_prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
					vk_phys_dev_feat.geometryShader == VK_TRUE) {
					discrete_device_index = i;
				}
				LOG_INFO("[vulkan] ", vk_phys_dev_prop.deviceName);
				i++;
			}

			if (discrete_device_index > -1) {
				physicalDevice = vk_physical_devices[discrete_device_index];
				vkGetPhysicalDeviceProperties(vk_physical_devices[discrete_device_index], &physicalDeviceProperties);
			} else {
				LOG_INFO("[vulkan] fallback to first entry");
				physicalDevice = vk_physical_devices[0];
				vkGetPhysicalDeviceProperties(vk_physical_devices[0], &physicalDeviceProperties);
			}

			SetGPUInfo();
			LOG_INFO("[vulkan] ", physicalDeviceProperties.deviceName, " (", driverVersion, ") selected");

			return true;
		}

		bool GraphicsVulkan::InitLogicalDevice(std::vector<const char*>& _device_extensions) {
			u32 queue_families_num;
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queue_families_num, nullptr);
			auto queue_family_properties = std::vector<VkQueueFamilyProperties>(queue_families_num);
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queue_families_num, queue_family_properties.data());

			u32 graphics_queue_index = 0;
			for (u32 i = 0; const auto & n : queue_family_properties) {
				if ((n.queueCount > 0) && (n.queueFlags & (VK_QUEUE_GRAPHICS_BIT & VK_QUEUE_TRANSFER_BIT))) {
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
			vk_device_info.enabledExtensionCount = (u32)_device_extensions.size();
			vk_device_info.ppEnabledExtensionNames = _device_extensions.data();
			vk_device_info.pEnabledFeatures = &vk_enabled_features;

			if (vkCreateDevice(physicalDevice, &vk_device_info, nullptr, &device) != VK_SUCCESS) {
				LOG_ERROR("[vulkan] create logical device");
				return false;
			}

			familyIndex = graphics_queue_index;
			vkGetDeviceQueue(device, graphics_queue_index, 0, &queue);

			vkGetPhysicalDeviceMemoryProperties(physicalDevice, &devMemProps);
			DetectResizableBar();

			return true;
		}

		void GraphicsVulkan::SetGPUInfo() {
			std::string name = std::string(physicalDeviceProperties.deviceName);

			u16 vendor_id = physicalDeviceProperties.vendorID & 0xFFFF;
			if (Config::VENDOR_IDS.find(vendor_id) != Config::VENDOR_IDS.end()) {
				vendor = Config::VENDOR_IDS.at(vendor_id);
			} else {
				vendor = N_A;
			}

			u32 driver = physicalDeviceProperties.driverVersion;
			if (vendor_id == Config::ID_NVIDIA) {
				driverVersion = std::format("{}.{}", (driver >> 22) & 0x3ff, (driver >> 14) & 0xff);
			}
			// TODO: doesn't work properly for AMD cards, will get fixed in the future
			else if (vendor_id == Config::ID_AMD) {
				driverVersion = std::format("{}.{}", driver >> 14, driver & 0x3fff);
			} else {
				driverVersion = N_A;
			}
		}

		bool GraphicsVulkan::InitSurface() {
			if (!SDL_Vulkan_CreateSurface(window, vulkanInstance, &surface)) {
				LOG_ERROR("[vulkan] ", SDL_GetError());
				return false;
			} else {
				return true;
			}
		}

		bool GraphicsVulkan::InitSwapchain(const VkImageUsageFlags& _flags) {
			VkBool32 supports_present = 0;
			if ((vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, familyIndex, surface, &supports_present) != VK_SUCCESS) || !supports_present) {
				LOG_ERROR("[vulkan] graphics queue doesn't support present");
				return false;
			}

			u32 formats_num;
			if ((vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formats_num, nullptr) != VK_SUCCESS) || !formats_num) {
				LOG_ERROR("[vulkan] couldn't acquire image formats");
				return false;
			}
			auto image_formats = std::vector<VkSurfaceFormatKHR>(formats_num);
			if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formats_num, image_formats.data()) != VK_SUCCESS) { return false; }

			swapchainFormat = image_formats[0].format;
			colorSpace = image_formats[0].colorSpace;

			VkSurfaceCapabilitiesKHR surface_capabilites;
			if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surface_capabilites) != VK_SUCCESS) {
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

			if (minImageCount < surface_capabilites.minImageCount) {
				minImageCount = surface_capabilites.minImageCount;
			}

			// TODO: look into swapchain settings
			VkSwapchainCreateInfoKHR swapchain_info;
			swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			swapchain_info.flags = 0;
			swapchain_info.surface = surface;
			swapchain_info.minImageCount = minImageCount;							// triple buffering
			swapchain_info.imageFormat = swapchainFormat;							// bit depth/tex2dFormat
			swapchain_info.imageColorSpace = colorSpace;							// used color space
			swapchain_info.imageExtent = surface_capabilites.currentExtent;			// image size
			swapchain_info.imageArrayLayers = 1;									// multiple images at once (e.g. VR)
			swapchain_info.imageUsage = _flags;										// usage of swapchain
			swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;			// sharing between different queues
			swapchain_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;	// no transform
			swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;		// no transparency (opaque)
			swapchain_info.presentMode = presentMode;								// simple image fifo (vsync) or immediate
			swapchain_info.clipped = VK_FALSE;										// draw only visible window areas if true (clipping)
			swapchain_info.oldSwapchain = oldSwapchain;
			swapchain_info.pNext = nullptr;

			if (vkCreateSwapchainKHR(device, &swapchain_info, nullptr, &swapchain) != VK_SUCCESS) {
				LOG_ERROR("[vulkan] couldn't create swapchain");
				return false;
			}

			win_width = surface_capabilites.currentExtent.width;
			win_height = surface_capabilites.currentExtent.height;
			viewport = { .0f, .0f, (float)win_width, (float)win_height, .0f, 1.f };
			scissor = { {0, 0}, {win_width, win_height} };

			u32 image_num = 0;
			if (vkGetSwapchainImagesKHR(device, swapchain, &image_num, nullptr) != VK_SUCCESS) {
				LOG_ERROR("[vulkan] couldn't acquire image count");
				return false;
			}

			images.clear();
			images.resize(image_num);
			if (vkGetSwapchainImagesKHR(device, swapchain, &image_num, images.data()) != VK_SUCCESS) { return false; }

			for (auto& n : imageViews) {
				vkDestroyImageView(device, n, nullptr);
			}
			imageViews.clear();
			imageViews.resize(image_num);
			for (u32 i = 0; i < image_num; i++) {
				VkImageViewCreateInfo imageview_info = {};
				imageview_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				imageview_info.image = images[i];
				imageview_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
				imageview_info.format = swapchainFormat;
				imageview_info.components = { VK_COMPONENT_SWIZZLE_IDENTITY };
				imageview_info.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

				if (vkCreateImageView(device, &imageview_info, nullptr, &imageViews[i]) != VK_SUCCESS) {
					LOG_ERROR("[vulkan] create image view ", i);
					return false;
				}
			}

			aspectRatio = (float)win_width / win_height;

			return true;
		}

		bool GraphicsVulkan::InitRenderPass() {
			VkAttachmentDescription attachment_description = {};
			attachment_description.format = swapchainFormat;
			attachment_description.samples = sample_count;														// MSAA
			attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;										// clear previous
			attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;										// store result
			attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;									// before renderpass -> don't care
			attachment_description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;								// final tex2dFormat to present

			VkAttachmentReference attachment_reference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };		// tex2dFormat for rendering
			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;										// pass type: graphics
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &attachment_reference;

			VkRenderPassCreateInfo renderpass_info = {};
			renderpass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderpass_info.attachmentCount = 1;
			renderpass_info.pAttachments = &attachment_description;
			renderpass_info.subpassCount = 1;
			renderpass_info.pSubpasses = &subpass;

			if (vkCreateRenderPass(device, &renderpass_info, nullptr, &renderPass) != VK_SUCCESS) {
				LOG_ERROR("[vulkan] create renderpass");
				return false;
			}

			return true;
		}

		bool GraphicsVulkan::InitFrameBuffers() {
			frameBuffers.clear();
			frameBuffers.resize(images.size());
			for (u32 i = 0; i < images.size(); i++) {
				VkFramebufferCreateInfo framebuffer_info = {};
				framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				framebuffer_info.renderPass = renderPass;
				framebuffer_info.attachmentCount = 1;
				framebuffer_info.pAttachments = &imageViews[i];
				framebuffer_info.width = win_width;
				framebuffer_info.height = win_height;
				framebuffer_info.layers = 1;

				if (vkCreateFramebuffer(device, &framebuffer_info, nullptr, &frameBuffers[i]) != VK_SUCCESS) {
					LOG_ERROR("[vulkan] create framebuffer ", i);
					return false;
				}
			}

			return true;
		}

		bool GraphicsVulkan::InitCommandBuffers() {
			VkCommandPoolCreateInfo cmdpool_info = {};
			cmdpool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			cmdpool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;					// command buffers only valid for one frame
			cmdpool_info.queueFamilyIndex = familyIndex;

			for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
				if (vkCreateCommandPool(device, &cmdpool_info, nullptr, &commandPools[i]) != VK_SUCCESS) {
					LOG_ERROR("[vulkan] create command pool ", i);
					return false;
				}
			}

			VkFenceCreateInfo fence_info = {};
			fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
			for (auto& n : renderFences) {
				if (vkCreateFence(device, &fence_info, nullptr, &n) != VK_SUCCESS) {
					LOG_ERROR("[vulkan] create renderFence");
					return false;
				}
			}

			for (auto& n : acquireSemaphores) {
				if (!InitSemaphore(n)) { return false; }
			}

			for (auto& n : releaseSemaphores) {
				if (!InitSemaphore(n)) { return false; }
			}

			for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
				VkCommandBufferAllocateInfo allocate_info = {};
				allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				allocate_info.commandPool = commandPools[i];
				allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				allocate_info.commandBufferCount = 1;

				if (vkAllocateCommandBuffers(device, &allocate_info, &commandBuffers[i]) != VK_SUCCESS) {
					LOG_ERROR("[vulkan] allocate command buffers ", i);
					return false;
				}
			}

			return true;
		}

		bool GraphicsVulkan::InitSemaphore(VkSemaphore& _semaphore) {
			VkSemaphoreCreateInfo sem_info = {};
			sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			if (vkCreateSemaphore(device, &sem_info, nullptr, &_semaphore) != VK_SUCCESS) {
				LOG_ERROR("[vulkan] create semaphore");
				return false;
			}
			return true;
		}

		bool GraphicsVulkan::InitImgui() {
			VkDescriptorPoolSize pool_sizes[] =
			{
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
			};

			VkDescriptorPoolCreateInfo pool_info = {};
			pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
			pool_info.maxSets = 1;
			pool_info.poolSizeCount = (u32)IM_ARRAYSIZE(pool_sizes);
			pool_info.pPoolSizes = pool_sizes;
			if (vkCreateDescriptorPool(device, &pool_info, nullptr, &imguiDescriptorPool) != VK_SUCCESS) {
				LOG_ERROR("[vulkan] create imgui descriptor pool");
				return false;
			}

			ImGui_ImplSDL2_InitForVulkan(window);
			ImGui_ImplVulkan_InitInfo init_info = {};
			init_info.Instance = vulkanInstance;
			init_info.PhysicalDevice = physicalDevice;
			init_info.Device = device;
			init_info.QueueFamily = familyIndex;
			init_info.Queue = queue;
			init_info.DescriptorPool = imguiDescriptorPool;
			init_info.MinImageCount = 2;
			init_info.ImageCount = (u32)images.size();
			init_info.MSAASamples = sample_count;
			if (!ImGui_ImplVulkan_Init(&init_info, renderPass)) {
				LOG_ERROR("[vulkan] init imgui");
				return false;
			}

			// Upload Fonts
			{
				// font loading and atlas rebuild before font texture upload to GPU
				ImGuiIO& io = ImGui::GetIO();
				std::string main_font = Config::FONT_FOLDER + Config::FONT_MAIN;
				const char* font = main_font.c_str();
				if (fonts.size() == 0) { fonts.emplace_back(); }

				io.Fonts->AddFontDefault();
				fonts[0] = io.Fonts->AddFontFromFileTTF(font, 13.f, NULL, io.Fonts->GetGlyphRangesDefault());
				IM_ASSERT(fonts[0] != nullptr);

				io.Fonts->Build();

				if (vkResetCommandPool(device, commandPools[0], 0) != VK_SUCCESS) {
					LOG_ERROR("[vulkan] imgui reset command pool");
					return false;
				}

				// fill command buffer and submit to queue
				VkCommandBufferBeginInfo begin_info = {};
				begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
				if (vkBeginCommandBuffer(commandBuffers[0], &begin_info) != VK_SUCCESS) {
					LOG_ERROR("[vulkan] imgui begin command buffer");
					return false;
				}
				{
					ImGui_ImplVulkan_CreateFontsTexture(commandBuffers[0]);
				}
				if (vkEndCommandBuffer(commandBuffers[0]) != VK_SUCCESS) {
					LOG_ERROR("[vulkan] imgui end command buffer");
					return false;
				}

				VkSubmitInfo end_info = {};
				end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				end_info.commandBufferCount = 1;
				end_info.pCommandBuffers = &commandBuffers[0];
				if (vkQueueSubmit(queue, 1, &end_info, VK_NULL_HANDLE) != VK_SUCCESS) {
					LOG_ERROR("[vulkan] imgui queue submit");
					return false;
				}

				WaitIdle();
				ImGui_ImplVulkan_DestroyFontUploadObjects();
			}

			LOG_INFO("[vulkan] imgui initialized");
			return true;
		}

		bool GraphicsVulkan::InitShaderModule(const vector<char>& _byte_code, VkShaderModule& _shader) {
			auto byte_code = vector<char>(_byte_code);

			VkShaderModuleCreateInfo shader_info = {};
			shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			shader_info.pCode = (u32*)byte_code.data();
			shader_info.codeSize = byte_code.size();

			if (vkCreateShaderModule(device, &shader_info, nullptr, &_shader) != VK_SUCCESS) {
				LOG_ERROR("[vulkan] create shader module");
				return false;
			}

			return true;
		}

		bool GraphicsVulkan::InitPipeline(VkShaderModule& _vertex_shader, VkShaderModule& _fragment_shader, VkPipelineLayout& _layout, VkPipeline& _pipeline, VulkanPipelineBufferInfo& _info, vector<VkDescriptorSetLayout>& _set_layouts, std::vector<VkPushConstantRange>& _push_constants) {
			auto shader_stages = vector<VkPipelineShaderStageCreateInfo>(2);
			shader_stages[0] = {};
			shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
			shader_stages[0].module = _vertex_shader;
			shader_stages[0].pName = "main";
			shader_stages[1] = {};
			shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			shader_stages[1].module = _fragment_shader;
			shader_stages[1].pName = "main";

			VkPipelineVertexInputStateCreateInfo vertex_input_state = {};
			vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertex_input_state.vertexAttributeDescriptionCount = (u32)_info.attrDesc.size();
			vertex_input_state.pVertexAttributeDescriptions = _info.attrDesc.data();
			vertex_input_state.vertexBindingDescriptionCount = (u32)_info.bindDesc.size();
			vertex_input_state.pVertexBindingDescriptions = _info.bindDesc.data();

			VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {};
			input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

			VkPipelineViewportStateCreateInfo viewport_state = {};
			viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewport_state.viewportCount = 1;																// currently only one viewport
			viewport_state.pViewports = nullptr;
			viewport_state.scissorCount = 1;
			viewport_state.pScissors = nullptr;

			VkPipelineRasterizationStateCreateInfo rasterization_state = {};
			rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterization_state.lineWidth = 1.0f;

			VkPipelineMultisampleStateCreateInfo multisample_state = {};
			multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisample_state.rasterizationSamples = sample_count;

			VkPipelineColorBlendAttachmentState color_blend_attachment = {};
			color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;			// color mask
			color_blend_attachment.blendEnable = VK_TRUE;
			color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
			color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

			VkPipelineColorBlendStateCreateInfo color_blend_state = {};
			color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			color_blend_state.attachmentCount = 1;
			color_blend_state.pAttachments = &color_blend_attachment;

			VkPipelineLayoutCreateInfo layout_info = {};
			layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			layout_info.setLayoutCount = (u32)_set_layouts.size();
			layout_info.pSetLayouts = _set_layouts.data();
			layout_info.pushConstantRangeCount = (u32)_push_constants.size();
			layout_info.pPushConstantRanges = _push_constants.data();
			if (vkCreatePipelineLayout(device, &layout_info, nullptr, &_layout) != VK_SUCCESS) {
				LOG_ERROR("[vulkan] create pipeline layout");
			}

			VkDynamicState states[2] = {
				VK_DYNAMIC_STATE_VIEWPORT,
				VK_DYNAMIC_STATE_SCISSOR
			};
			VkPipelineDynamicStateCreateInfo dynamic_state = {};
			dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamic_state.dynamicStateCount = 2;
			dynamic_state.pDynamicStates = states;

			VkGraphicsPipelineCreateInfo pipeline_info = {};
			pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipeline_info.stageCount = (u32)shader_stages.size();
			pipeline_info.pStages = shader_stages.data();
			pipeline_info.pVertexInputState = &vertex_input_state;
			pipeline_info.pInputAssemblyState = &input_assembly_state;
			pipeline_info.pViewportState = &viewport_state;
			pipeline_info.pRasterizationState = &rasterization_state;
			pipeline_info.pMultisampleState = &multisample_state;
			pipeline_info.pColorBlendState = &color_blend_state;
			pipeline_info.layout = _layout;
			pipeline_info.renderPass = renderPass;					// currently only one render pass, postprocessing probably more or multiple subpasses
			pipeline_info.subpass = 0;
			pipeline_info.pDynamicState = &dynamic_state;
			if (vkCreateGraphicsPipelines(device, 0, 1, &pipeline_info, nullptr, &_pipeline) != VK_SUCCESS) {
				LOG_ERROR("[vulkan] create graphics pipeline");
			}

			vkDestroyShaderModule(device, _vertex_shader, nullptr);
			vkDestroyShaderModule(device, _fragment_shader, nullptr);

			return true;
		}

		bool GraphicsVulkan::InitBuffer(vulkan_buffer& _buffer, u64 _size, VkBufferUsageFlags _usage, VkMemoryPropertyFlags _memory_properties) {
			VkBufferCreateInfo create_info = {};
			create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			create_info.size = _size;
			create_info.usage = _usage;
			if (vkCreateBuffer(device, &create_info, nullptr, &_buffer.buffer) != VK_SUCCESS) {
				LOG_ERROR("[vulkan] create buffer");
				return false;
			}

			VkMemoryRequirements mem_requirements;
			vkGetBufferMemoryRequirements(device, _buffer.buffer, &mem_requirements);

			uint32_t mem_index = FindMemoryTypes(mem_requirements.memoryTypeBits, _memory_properties);
			if (mem_index == UINT32_MAX) {
				LOG_ERROR("[vulkan] no matching memory types found: ", std::format("{:x}", mem_requirements.memoryTypeBits));
				return false;
			}

			VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
			alloc_info.allocationSize = mem_requirements.size;
			alloc_info.memoryTypeIndex = mem_index;

			if (vkAllocateMemory(device, &alloc_info, nullptr, &_buffer.memory) != VK_SUCCESS) {
				LOG_ERROR("[vulkan] memory allocation");
				return false;
			}

			if (vkBindBufferMemory(device, _buffer.buffer, _buffer.memory, 0) != VK_SUCCESS) {
				LOG_ERROR("[vulkan] binding memory buffer");
				return false;
			}
			return true;
		}

		bool GraphicsVulkan::LoadBuffer(vulkan_buffer& _buffer, void* _data, size_t _size) {
			if (resizableBar) {
				void* data;
				if (vkMapMemory(device, _buffer.memory, 0, _size, 0, &data) != VK_SUCCESS) {
					return false;
				}
				memcpy(data, _data, _size);
			} else {
				vulkan_buffer staging_buffer = {};
				if (!InitBuffer(staging_buffer, _size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
					LOG_ERROR("[vulkan] init staging buffer");
					return false;
				}
				void* data;
				if (vkMapMemory(device, staging_buffer.memory, 0, _size, 0, &data) != VK_SUCCESS) {
					LOG_ERROR("[vulkan] map memory for staging buffer");
					return false;
				}
				memcpy(data, _data, _size);
				vkUnmapMemory(device, staging_buffer.memory);

				VkCommandPool cmd_pool;
				VkCommandBuffer cmd_buffer;

				VkCommandPoolCreateInfo create_info = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
				create_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
				create_info.queueFamilyIndex = familyIndex;
				if (vkCreateCommandPool(device, &create_info, nullptr, &cmd_pool) != VK_SUCCESS) {
					LOG_ERROR("[vulkan] init command pool for staging buffer");
					return false;
				}

				VkCommandBufferAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
				alloc_info.commandPool = cmd_pool;
				alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				alloc_info.commandBufferCount = 1;
				if (vkAllocateCommandBuffers(device, &alloc_info, &cmd_buffer) != VK_SUCCESS) {
					LOG_ERROR("[vulkan] allocate command buffer for staging buffer");
				}

				VkCommandBufferBeginInfo begin_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
				begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
				if (vkBeginCommandBuffer(cmd_buffer, &begin_info) != VK_SUCCESS) {
					LOG_ERROR("[vulkan] begin command buffer for staging buffer");
					return false;
				}
				{
					VkBufferCopy region = { 0, 0, _size };
					vkCmdCopyBuffer(cmd_buffer, staging_buffer.buffer, _buffer.buffer, 1, &region);
				}
				if (vkEndCommandBuffer(cmd_buffer) != VK_SUCCESS) {
					LOG_ERROR("[vulkan] end command buffer for staging buffer");
					return false;
				}

				VkSubmitInfo submit_info = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
				submit_info.commandBufferCount = 1;
				submit_info.pCommandBuffers = &cmd_buffer;
				if (vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE) != VK_SUCCESS) {
					LOG_ERROR("[vulkan] submit command buffer for staging buffer to queue");
					return false;
				}
				if (vkQueueWaitIdle(queue) != VK_SUCCESS) {
					LOG_ERROR("[vulkan] process command buffer for staging buffer");
					return false;
				}

				vkDestroyCommandPool(device, cmd_pool, nullptr);
				DestroyBuffer(staging_buffer);
			}
			return true;
		}

		bool GraphicsVulkan::InitImage(vulkan_image& _image, u32 _width, u32 _height, VkFormat _format, VkImageUsageFlags _usage, VkImageTiling _tiling) {
			VkImageCreateInfo image_info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
			image_info.imageType = VK_IMAGE_TYPE_2D;
			image_info.extent.width = _width;
			image_info.extent.height = _height;
			image_info.extent.depth = 1;
			image_info.mipLevels = 1;
			image_info.arrayLayers = 1;
			image_info.format = _format;
			image_info.tiling = _tiling;
			image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			image_info.usage = _usage;
			image_info.samples = VK_SAMPLE_COUNT_1_BIT;
			image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			if (vkCreateImage(device, &image_info, nullptr, &_image.image) != VK_SUCCESS) {
				LOG_ERROR("[vulkan] create image");
				return false;
			}

			VkMemoryRequirements mem_requ;
			vkGetImageMemoryRequirements(device, _image.image, &mem_requ);
			VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
			alloc_info.allocationSize = mem_requ.size;
			alloc_info.memoryTypeIndex = FindMemoryTypes(mem_requ.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			if (vkAllocateMemory(device, &alloc_info, nullptr, &_image.memory) != VK_SUCCESS) {
				LOG_ERROR("[vulkan] allocate image memory");
			}
			if (vkBindImageMemory(device, _image.image, _image.memory, 0) != VK_SUCCESS) {
				LOG_ERROR("[vulkan] bind image memory");
			}

			VkImageViewCreateInfo create_info = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
			create_info.image = _image.image;
			create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
			create_info.format = _format;
			create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			create_info.subresourceRange.levelCount = 1;
			create_info.subresourceRange.layerCount = 1;
			if (vkCreateImageView(device, &create_info, nullptr, &_image.image_view) != VK_SUCCESS) {
				LOG_ERROR("[vulkan] create image view");
				return false;
			}

			return true;
		}

		void GraphicsVulkan::RebuildSwapchain() {
			WaitIdle();

			DestroyFrameBuffers();
			//DestroyRenderPass();
	
			oldSwapchain = swapchain;
			InitSwapchain(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
			DestroySwapchain(true);

			//InitRenderPass();
			InitFrameBuffers();
		}

		void GraphicsVulkan::DestroySwapchain(const bool& _rebuild) {
			WaitIdle();
			if (_rebuild) {
				vkDestroySwapchainKHR(device, oldSwapchain, nullptr);
			} else {
				for (auto& n : imageViews) {
					vkDestroyImageView(device, n, nullptr);
				}
				vkDestroySwapchainKHR(device, swapchain, nullptr);
			}
		}

		void GraphicsVulkan::DestroySurface() {
			WaitIdle();
			vkDestroySurfaceKHR(vulkanInstance, surface, nullptr);
		}

		void GraphicsVulkan::DestroyFrameBuffers() {
			WaitIdle();
			for (auto& n : frameBuffers) {
				vkDestroyFramebuffer(device, n, nullptr);
			}
			frameBuffers.clear();
		}

		void GraphicsVulkan::DestroyRenderPass() {
			WaitIdle();
			vkDestroyRenderPass(device, renderPass, nullptr);
		}

		void GraphicsVulkan::DestroyCommandBuffer() {
			WaitIdle();
			for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
				vkDestroyCommandPool(device, commandPools[i], nullptr);
			}
			for (auto& n : renderFences) {
				vkDestroyFence(device, n, nullptr);
			}

			for (auto& n : acquireSemaphores) {
				DestroySemaphore(n);
			}
			for (auto& n : releaseSemaphores) {
				DestroySemaphore(n);
			}
		}

		void GraphicsVulkan::DestroyImgui() {
			WaitIdle();
			ImGui_ImplVulkan_Shutdown();
			vkDestroyDescriptorPool(device, imguiDescriptorPool, nullptr);
		}

		void GraphicsVulkan::DestroyPipelines() {
			WaitIdle();
			for (const auto& [layout, pipeline] : pipelines) {
				vkDestroyPipeline(device, pipeline, nullptr);
				vkDestroyPipelineLayout(device, layout, nullptr);
			}
			pipelines.clear();
		}

		void GraphicsVulkan::DestroyBuffer(vulkan_buffer& _buffer) {
			WaitIdle();
			vkDestroyBuffer(device, _buffer.buffer, nullptr);
			vkFreeMemory(device, _buffer.memory, nullptr);
		}

		void GraphicsVulkan::DestroyImage(vulkan_image& _image) {
			WaitIdle();
			vkDestroyImageView(device, _image.image_view, nullptr);
			vkDestroyImage(device, _image.image, nullptr);
			vkFreeMemory(device, _image.memory, nullptr);
		}

		void GraphicsVulkan::DestroySemaphore(VkSemaphore& _semaphore) {
			vkDestroySemaphore(device, _semaphore, nullptr);
		}

		bool GraphicsVulkan::InitTex2dSampler() {
			VkSamplerCreateInfo create_info = {};
			create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			create_info.magFilter = VK_FILTER_NEAREST;
			create_info.minFilter = VK_FILTER_NEAREST;
			create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			create_info.addressModeV = create_info.addressModeU;
			create_info.addressModeW = create_info.addressModeU;
			create_info.mipLodBias = .0f;
			create_info.maxAnisotropy = 1.0f;
			create_info.minLod = .0f;
			create_info.maxLod = .0f;

			if (vkCreateSampler(device, &create_info, nullptr, &tex2dData.sampler) != VK_SUCCESS) {
				LOG_ERROR("[vulkan] create sampler for Texture2d");
				return false;
			}

			return true;
		}

		bool GraphicsVulkan::InitTex2dDescriptorSets() {
			{
				VkDescriptorPoolSize poolSizes[] = {
					{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
				};
				VkDescriptorPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
				createInfo.maxSets = 1;
				createInfo.poolSizeCount = sizeof(poolSizes) / sizeof(poolSizes[0]);
				createInfo.pPoolSizes = poolSizes;
				if (vkCreateDescriptorPool(device, &createInfo, nullptr, &tex2dData.descriptor_pool) != VK_SUCCESS) {
					LOG_ERROR("[vulkan] create descriptor pool");
					return false;
				}
			}

			{
				// combined: texture and sampler at the same time
				VkDescriptorSetLayoutBinding bindings[] = {
					{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
				};
				VkDescriptorSetLayoutCreateInfo createInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
				createInfo.bindingCount = sizeof(bindings) / sizeof(bindings[0]);
				createInfo.pBindings = bindings;
				if (vkCreateDescriptorSetLayout(device, &createInfo, nullptr, tex2dData.descriptor_set_layout.data()) != VK_SUCCESS) {
					LOG_ERROR("[vulkan] create descriptor pool layout");
					return false;
				}

				VkDescriptorSetAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
				allocateInfo.descriptorPool = tex2dData.descriptor_pool;
				allocateInfo.descriptorSetCount = (u32)tex2dData.descriptor_set_layout.size();
				allocateInfo.pSetLayouts = tex2dData.descriptor_set_layout.data();
				if (vkAllocateDescriptorSets(device, &allocateInfo, &tex2dData.descriptor_set) != VK_SUCCESS) {
					LOG_ERROR("[vulkan] allocate descriptor sets");
					return false;
				}

				// info for descriptor of texture2d to sample from -> fragment shader
				VkDescriptorImageInfo imageInfo = { tex2dData.sampler, tex2dData.image.image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
				auto descriptorWrites = std::vector<VkWriteDescriptorSet>(1);
				descriptorWrites[0] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
				descriptorWrites[0].dstSet = tex2dData.descriptor_set;
				descriptorWrites[0].dstBinding = 0;
				descriptorWrites[0].descriptorCount = 1;
				descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				descriptorWrites[0].pImageInfo = &imageInfo;
				vkUpdateDescriptorSets(device, (u32)descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
			}

			return true;
		}

		bool GraphicsVulkan::InitTex2dBuffers() {
			// staging buffer for texture upload
			tex2dData.size = virtGraphicsInfo.lcd_width * virtGraphicsInfo.lcd_height * TEX2D_CHANNELS;
			for (int i = 0; i < FRAMES_IN_FLIGHT_2D; i++) {
				tex2dData.staging_buffer.emplace_back();
				if (!InitBuffer(tex2dData.staging_buffer[i], tex2dData.size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
					LOG_ERROR("[vulkan] create staging buffer for main texture");
					return false;
				}
			}

			// image buffer for shader usage
			if (!InitImage(tex2dData.image, virtGraphicsInfo.lcd_width, virtGraphicsInfo.lcd_height, tex2dData.format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, /*resizableBar ? VK_IMAGE_TILING_LINEAR :*/ VK_IMAGE_TILING_OPTIMAL)) {
				LOG_ERROR("[vulkan] create target 2d texture for virtual hardware");
				return false;
			}

			tex2dData.mapped_image_data = std::vector<void*>(FRAMES_IN_FLIGHT_2D);
			for (int i = 0; auto & n : tex2dData.staging_buffer) {
				if (vkMapMemory(device, n.memory, 0, tex2dData.size, 0, &tex2dData.mapped_image_data[i]) != VK_SUCCESS) {
					LOG_ERROR("[vulkan] map image memory");
					return false;
				}
				i++;
			}

			return true;
		}

		void GraphicsVulkan::DestroyTex2dSampler() {
			WaitIdle();
			vkDestroySampler(device, tex2dData.sampler, nullptr);
		}

		bool GraphicsVulkan::InitTex2dPipeline() {
			VkShaderModule vertex_shader;
			VkShaderModule fragment_shader;

			auto vertex_shader_data = vector<char>(tex2dVertShader.data(), tex2dVertShader.data() + tex2dVertShader.size());
			auto fragment_shader_data = vector<char>(tex2dFragShader.data(), tex2dFragShader.data() + tex2dFragShader.size());

			if (InitShaderModule(vertex_shader_data, vertex_shader) && InitShaderModule(fragment_shader_data, fragment_shader)) {
				VulkanPipelineBufferInfo buffer_info = {};
				VkVertexInputBindingDescription bind_desc = {};
				bind_desc.binding = 0;
				bind_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
				bind_desc.stride = sizeof(float) * 4;
				buffer_info.bindDesc.push_back(bind_desc);

				auto attr_desc = vector<VkVertexInputAttributeDescription>(2);
				attr_desc[0].binding = 0;							// vertex data
				attr_desc[0].location = 0;
				attr_desc[0].format = VK_FORMAT_R32G32_SFLOAT;
				attr_desc[0].offset = 0;
				attr_desc[1].binding = 0;
				attr_desc[1].location = 1;
				attr_desc[1].format = VK_FORMAT_R32G32_SFLOAT;
				attr_desc[1].offset = sizeof(float) * 2;
				buffer_info.attrDesc = attr_desc;

				auto push_constants = vector<VkPushConstantRange>(1);
				push_constants[0].offset = 0;
				push_constants[0].size = sizeof(glm::mat4);
				push_constants[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

				InitPipeline(vertex_shader, fragment_shader, tex2dData.pipeline_layout, tex2dData.pipeline, buffer_info, tex2dData.descriptor_set_layout, push_constants);

				return true;
			} else {
				return false;
			}
		}

		void GraphicsVulkan::DestroyTex2dPipeline() {
			WaitIdle();
			vkDestroyPipeline(device, tex2dData.pipeline, nullptr);
			vkDestroyPipelineLayout(device, tex2dData.pipeline_layout, nullptr);
		}

		void GraphicsVulkan::WaitIdle() {
			if (vkDeviceWaitIdle(device) != VK_SUCCESS) {
				LOG_ERROR("[vulkan] GPU wait idle");
			}
		}

		void GraphicsVulkan::NextFrameImGui() const {
			ImGui_ImplVulkan_NewFrame();
		}

		void GraphicsVulkan::EnumerateShaders() {
			LOG_INFO("[vulkan] enumerating shader source files");

			auto enumeratedShaderFiles = vector<string>();

			FileIO::get_files_in_path(enumeratedShaderFiles, Config::SHADER_FOLDER);
			shaderSourceFiles = vector<pair<string, string>>();

			for (const auto& n : enumeratedShaderFiles) {
				const auto file_1 = Helpers::split_string(n, "/").back();

				if (SHADER_TYPES.at(VERT_EXT) == SHADER_TYPES.at(Helpers::split_string(file_1, ".").back())) {

					for (const auto& m : enumeratedShaderFiles) {
						const auto file_2 = Helpers::split_string(m, "/").back();

						if (Helpers::split_string(file_1, ".").front().compare(Helpers::split_string(file_2, ".").front()) == 0 &&
							SHADER_TYPES.at(FRAG_EXT) == SHADER_TYPES.at(Helpers::split_string(file_2, ".").back())) {
							shaderSourceFiles.emplace_back(n, m);
						}
					}
				}
			}
			LOG_INFO("[vulkan] ", shaderSourceFiles.size(), " shader(s) found");

			if (shaderSourceFiles.empty()) {
				shaderCompilationFinished = true;
				return;
			} else {
				shadersCompiled = 0;
				shadersTotal = (int)(shaderSourceFiles.size());
				shaderCompilationFinished = false;

				compiler = shaderc_compiler_initialize();
				options = shaderc_compile_options_initialize();
				shaderc_compile_options_set_auto_map_locations(options, true);
			}
		}

		void GraphicsVulkan::CompileNextShader() {
			bool compiled = true;
			vector<char> vertex_byte_code;
			if (!compile_shader(vertex_byte_code, shaderSourceFiles[shadersCompiled].first, compiler, options)) {
				compiled = false;
			}
			vector<char> fragment_byte_code;
			if (!compile_shader(fragment_byte_code, shaderSourceFiles[shadersCompiled].second, compiler, options)) {
				compiled = false;
			}

			if (compiled) {
				VkShaderModule vertex_shader;
				VkShaderModule fragment_shader;

				if (InitShaderModule(vertex_byte_code, vertex_shader) && InitShaderModule(fragment_byte_code, fragment_shader)) {
					pipelines.emplace_back();
					auto& [layout, pipeline] = pipelines.back();
					//InitPipeline(vertex_shader, fragment_shader, layout, pipeline, buffer_info);
					// only used while no pipelines are created, remove destroy calls later
					vkDestroyShaderModule(device, vertex_shader, nullptr);
					vkDestroyShaderModule(device, fragment_shader, nullptr);
				}
			}

			shadersCompiled++;
			if (shadersCompiled == shadersTotal) {
				shaderCompilationFinished = true;

				shaderc_compiler_release(compiler);
				shaderc_compile_options_release(options);
			}
		}

		u32 GraphicsVulkan::FindMemoryTypes(u32 _type_filter, VkMemoryPropertyFlags _mem_properties) {
			for (uint32_t i = 0; i < devMemProps.memoryTypeCount; i++) {
				if (_type_filter & (1 << i)) {
					if ((devMemProps.memoryTypes[i].propertyFlags & _mem_properties) == _mem_properties) {
						return i;
					}
				}
			}
			LOG_ERROR("[vulkan] memory type not found");
			return UINT32_MAX;
		}

		void GraphicsVulkan::DetectResizableBar() {
			resizableBar = false;
			VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

			for (uint32_t i = 0; i < devMemProps.memoryTypeCount; i++) {
				if (float heap_size = devMemProps.memoryHeaps[devMemProps.memoryTypes[i].heapIndex].size / (float)pow(10, 9);
					(devMemProps.memoryTypes[i].propertyFlags & flags) == flags && heap_size > .5f) {
					resizableBar = true;
					LOG_INFO(std::format("[vulkan] resizable bar enabled (Heap{:d}:{:.2f}GB)", devMemProps.memoryTypes[i].heapIndex, heap_size));
				}
			}

			bufferUsageFlags = resizableBar ? 0 : VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | (resizableBar ? (VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) : 0);

			if (!resizableBar) { LOG_INFO("[vulkan] resizable bar disabled"); }
		}

		#ifdef VK_DEBUG_CALLBACK
		VkBool32 VKAPI_CALL debug_report_callback(VkDebugUtilsMessageSeverityFlagBitsEXT _severity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* _callback_data, void* userData) {
			if (_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
				LOG_ERROR(_callback_data->pMessage);
			} else {
				LOG_WARN(_callback_data->pMessage);
			}
			return VK_FALSE;
		}

		VkDebugUtilsMessengerEXT GraphicsVulkan::RegisterDebugCallback() {
			PFN_vkCreateDebugUtilsMessengerEXT pfnCreateDebutUtilsMessengerEXT;
			pfnCreateDebutUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vulkanInstance, "vkCreateDebugUtilsMessengerEXT");

			VkDebugUtilsMessengerCreateInfoEXT callbackInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
			callbackInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
			callbackInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
			callbackInfo.pfnUserCallback = debug_report_callback;

			VkDebugUtilsMessengerEXT callback = nullptr;
			if (pfnCreateDebutUtilsMessengerEXT(vulkanInstance, &callbackInfo, nullptr, &callback) != VK_SUCCESS) {
				LOG_ERROR("[vulkan] create callback messenger");
				return nullptr;
			}

			return callback;
		}
		#endif

		bool compile_shader(vector<char>& _byte_code, const string& _shader_source_file, const shaderc_compiler_t& _compiler, const shaderc_compile_options_t& _options) {
			auto source_text_vec = vector<char>();
			if (!FileIO::read_data(source_text_vec, _shader_source_file)) {
				LOG_ERROR("[vulkan] read shader source ", _shader_source_file);
				return false;
			}

			size_t source_size = source_text_vec.size();
			shaderc_shader_kind type = SHADER_TYPES.at(Helpers::split_string(_shader_source_file, ".").back());
			string file_name_str = Helpers::split_string(_shader_source_file, "/").back();
			const char* file_name = file_name_str.c_str();

			shaderc_compilation_result_t result = shaderc_compile_into_spv(_compiler, source_text_vec.data(), source_size, type, file_name, "main", _options);

			size_t error_num = shaderc_result_get_num_errors(result);
			//size_t warning_num = shaderc_result_get_num_warnings(result);
			if (error_num > 0) {
				const char* error = shaderc_result_get_error_message(result);
				LOG_ERROR("[vulkan] compilation of ", _shader_source_file, ": ", error);
				shaderc_result_release(result);
				return false;
			}

			size_t size = shaderc_result_get_length(result);
			const char* byte_code = shaderc_result_get_bytes(result);

			_byte_code = vector<char>(byte_code, byte_code + size);

			// uncomment this section to write the compiled bytecode to a *.spv file in ./shader/spir_v/
			auto byte_code_vec = vector<char>(byte_code, byte_code + size);
			auto file_name_parts = Helpers::split_string(file_name_str, ".");
			string out_file_path = Config::SPIR_V_FOLDER + file_name_parts.front() + "_" + file_name_parts.back() + "." + SPIRV_EXT;
			FileIO::write_data(byte_code_vec, out_file_path, true);
			shaderc_result_release(result);

			LOG_INFO("[vulkan] ", file_name_str, " compiled");
			return true;
		}
	}
}