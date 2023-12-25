#include "VulkanMgr.h"

#include "logger.h"
#include "general_config.h"
#include "helper_functions.h"
#include "data_io.h"

#include <unordered_map>
#include <format>
#include <iostream>

using namespace std;

#define VK_VALIDATION "VK_LAYER_KHRONOS_validation"

const vector<const char*> VK_ENABLED_LAYERS = {
	"VK_LAYER_KHRONOS_validation"
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
	-1.0f, -1.0f,
	3.0f, -1.0f,
	-1.0f, 3.0f,
};

u32 indexData[] = {
	0, 1, 2
};

// main vertex and fragment shader for rendering as byte code
// created with ./shader/spir_v/print_array.py
const vector<u8> tex2dVertShader = {
		0x03, 0x02, 0x23, 0x07, 0x00, 0x00, 0x01, 0x00, 0x0b, 0x00, 0x0d, 0x00, 0x29, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x06, 0x00,
		0x01, 0x00, 0x00, 0x00, 0x47, 0x4c, 0x53, 0x4c, 0x2e, 0x73, 0x74, 0x64, 0x2e, 0x34, 0x35, 0x30,
		0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
		0x0f, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x6d, 0x61, 0x69, 0x6e,
		0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x1d, 0x00, 0x00, 0x00,
		0x20, 0x00, 0x00, 0x00, 0x03, 0x00, 0x03, 0x00, 0x02, 0x00, 0x00, 0x00, 0xc2, 0x01, 0x00, 0x00,
		0x04, 0x00, 0x0a, 0x00, 0x47, 0x4c, 0x5f, 0x47, 0x4f, 0x4f, 0x47, 0x4c, 0x45, 0x5f, 0x63, 0x70,
		0x70, 0x5f, 0x73, 0x74, 0x79, 0x6c, 0x65, 0x5f, 0x6c, 0x69, 0x6e, 0x65, 0x5f, 0x64, 0x69, 0x72,
		0x65, 0x63, 0x74, 0x69, 0x76, 0x65, 0x00, 0x00, 0x04, 0x00, 0x08, 0x00, 0x47, 0x4c, 0x5f, 0x47,
		0x4f, 0x4f, 0x47, 0x4c, 0x45, 0x5f, 0x69, 0x6e, 0x63, 0x6c, 0x75, 0x64, 0x65, 0x5f, 0x64, 0x69,
		0x72, 0x65, 0x63, 0x74, 0x69, 0x76, 0x65, 0x00, 0x05, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00,
		0x6d, 0x61, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x03, 0x00, 0x09, 0x00, 0x00, 0x00,
		0x75, 0x76, 0x00, 0x00, 0x05, 0x00, 0x06, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x67, 0x6c, 0x5f, 0x56,
		0x65, 0x72, 0x74, 0x65, 0x78, 0x49, 0x6e, 0x64, 0x65, 0x78, 0x00, 0x00, 0x05, 0x00, 0x06, 0x00,
		0x1b, 0x00, 0x00, 0x00, 0x67, 0x6c, 0x5f, 0x50, 0x65, 0x72, 0x56, 0x65, 0x72, 0x74, 0x65, 0x78,
		0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x06, 0x00, 0x1b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x67, 0x6c, 0x5f, 0x50, 0x6f, 0x73, 0x69, 0x74, 0x69, 0x6f, 0x6e, 0x00, 0x06, 0x00, 0x07, 0x00,
		0x1b, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x67, 0x6c, 0x5f, 0x50, 0x6f, 0x69, 0x6e, 0x74,
		0x53, 0x69, 0x7a, 0x65, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x07, 0x00, 0x1b, 0x00, 0x00, 0x00,
		0x02, 0x00, 0x00, 0x00, 0x67, 0x6c, 0x5f, 0x43, 0x6c, 0x69, 0x70, 0x44, 0x69, 0x73, 0x74, 0x61,
		0x6e, 0x63, 0x65, 0x00, 0x06, 0x00, 0x07, 0x00, 0x1b, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
		0x67, 0x6c, 0x5f, 0x43, 0x75, 0x6c, 0x6c, 0x44, 0x69, 0x73, 0x74, 0x61, 0x6e, 0x63, 0x65, 0x00,
		0x05, 0x00, 0x03, 0x00, 0x1d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x05, 0x00,
		0x20, 0x00, 0x00, 0x00, 0x69, 0x6e, 0x5f, 0x70, 0x6f, 0x73, 0x69, 0x74, 0x69, 0x6f, 0x6e, 0x00,
		0x47, 0x00, 0x04, 0x00, 0x09, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x47, 0x00, 0x04, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x2a, 0x00, 0x00, 0x00,
		0x48, 0x00, 0x05, 0x00, 0x1b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00, 0x1b, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
		0x0b, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00, 0x1b, 0x00, 0x00, 0x00,
		0x02, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00,
		0x1b, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
		0x47, 0x00, 0x03, 0x00, 0x1b, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00,
		0x20, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0x00, 0x02, 0x00,
		0x02, 0x00, 0x00, 0x00, 0x21, 0x00, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
		0x16, 0x00, 0x03, 0x00, 0x06, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00,
		0x07, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00,
		0x08, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00,
		0x08, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x15, 0x00, 0x04, 0x00,
		0x0a, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00,
		0x0b, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00,
		0x0b, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00,
		0x0a, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00,
		0x0a, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00,
		0x17, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x15, 0x00, 0x04, 0x00,
		0x18, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00,
		0x18, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x04, 0x00,
		0x1a, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x06, 0x00,
		0x1b, 0x00, 0x00, 0x00, 0x17, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x1a, 0x00, 0x00, 0x00,
		0x1a, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
		0x1b, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x1d, 0x00, 0x00, 0x00,
		0x03, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
		0x07, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
		0x01, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x06, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x06, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x80, 0x3f, 0x20, 0x00, 0x04, 0x00, 0x27, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
		0x17, 0x00, 0x00, 0x00, 0x36, 0x00, 0x05, 0x00, 0x02, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x05, 0x00, 0x00, 0x00,
		0x3d, 0x00, 0x04, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00,
		0xc4, 0x00, 0x05, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00,
		0x0e, 0x00, 0x00, 0x00, 0xc7, 0x00, 0x05, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00,
		0x0f, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x6f, 0x00, 0x04, 0x00, 0x06, 0x00, 0x00, 0x00,
		0x12, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x0a, 0x00, 0x00, 0x00,
		0x13, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0xc7, 0x00, 0x05, 0x00, 0x0a, 0x00, 0x00, 0x00,
		0x14, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x6f, 0x00, 0x04, 0x00,
		0x06, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00,
		0x07, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00,
		0x3e, 0x00, 0x03, 0x00, 0x09, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00,
		0x07, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x51, 0x00, 0x05, 0x00,
		0x06, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x51, 0x00, 0x05, 0x00, 0x06, 0x00, 0x00, 0x00, 0x25, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00,
		0x01, 0x00, 0x00, 0x00, 0x50, 0x00, 0x07, 0x00, 0x17, 0x00, 0x00, 0x00, 0x26, 0x00, 0x00, 0x00,
		0x24, 0x00, 0x00, 0x00, 0x25, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00,
		0x41, 0x00, 0x05, 0x00, 0x27, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x1d, 0x00, 0x00, 0x00,
		0x1e, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00, 0x28, 0x00, 0x00, 0x00, 0x26, 0x00, 0x00, 0x00,
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
		0x72, 0x65, 0x32, 0x64, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x03, 0x00, 0x11, 0x00, 0x00, 0x00,
		0x75, 0x76, 0x00, 0x00, 0x05, 0x00, 0x05, 0x00, 0x15, 0x00, 0x00, 0x00, 0x6f, 0x75, 0x74, 0x5f,
		0x63, 0x6f, 0x6c, 0x6f, 0x72, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x0d, 0x00, 0x00, 0x00,
		0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x0d, 0x00, 0x00, 0x00,
		0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x11, 0x00, 0x00, 0x00,
		0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x15, 0x00, 0x00, 0x00,
		0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0x00, 0x02, 0x00, 0x02, 0x00, 0x00, 0x00,
		0x21, 0x00, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x16, 0x00, 0x03, 0x00,
		0x06, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00, 0x07, 0x00, 0x00, 0x00,
		0x06, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x08, 0x00, 0x00, 0x00,
		0x07, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x19, 0x00, 0x09, 0x00, 0x0a, 0x00, 0x00, 0x00,
		0x06, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x03, 0x00,
		0x0b, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x0c, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x0c, 0x00, 0x00, 0x00,
		0x0d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00, 0x0f, 0x00, 0x00, 0x00,
		0x06, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x00,
		0x01, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x00,
		0x11, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x14, 0x00, 0x00, 0x00,
		0x03, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x14, 0x00, 0x00, 0x00,
		0x15, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x36, 0x00, 0x05, 0x00, 0x02, 0x00, 0x00, 0x00,
		0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00,
		0x05, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x08, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00,
		0x07, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00,
		0x0d, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00,
		0x11, 0x00, 0x00, 0x00, 0x57, 0x00, 0x05, 0x00, 0x07, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00,
		0x0e, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00, 0x09, 0x00, 0x00, 0x00,
		0x13, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x07, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00,
		0x09, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00, 0x15, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00,
		0xfd, 0x00, 0x01, 0x00, 0x38, 0x00, 0x01, 0x00
};

bool compile_shader(vector<char>& _byte_code, const string& _shader_source_file, const shaderc_compiler_t& _compiler, const shaderc_compile_options_t& _options);
#ifdef VK_DEBUG_CALLBACK
VkBool32 VKAPI_CALL debug_report_callback(VkDebugUtilsMessageSeverityFlagBitsEXT _severity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* _callback_data, void* userData);
#endif

VulkanMgr* VulkanMgr::instance = nullptr;

VulkanMgr* VulkanMgr::getInstance(SDL_Window* _window, graphics_information& _graphics_info, game_status& _game_stat) {
	if (instance != nullptr) {
		instance->resetInstance();
	}
	instance = new VulkanMgr(_window, _graphics_info, _game_stat);
	return instance;
}

void VulkanMgr::resetInstance() {
	if (instance != nullptr) {
		instance->StopGraphics();
		instance->ExitGraphics();
		delete instance;
		instance = nullptr;
	}
}

bool VulkanMgr::InitGraphics() {
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

bool VulkanMgr::ExitGraphics() {
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

bool VulkanMgr::StartGraphics() {
	if (!InitSurface()) {
		return false;
	}
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

	bindPipelines = &VulkanMgr::BindPipelinesDummy;
	rebuildFunction = &VulkanMgr::RebuildDummy;
	updateFunction = &VulkanMgr::UpdateDummy;

	return true;
}

void VulkanMgr::StopGraphics() {
	if (gameStat.game_running) {
		if (graphicsInfo.is2d) {
			DestroyTex2dRenderTarget();
		}
	}
	DestroyCommandBuffer();
	DestroyFrameBuffers();
	DestroyPipelines();
	DestroyRenderPass();
	DestroySwapchain(false);
	DestroySurface();
}

bool VulkanMgr::Init2dGraphicsBackend() {
	if (InitTex2dRenderTarget()) {
		UpdateGraphicsInfo();
		UpdateTex2d();
		return true;
	}
	else {
		return false;
	}
}

void VulkanMgr::Destroy2dGraphicsBackend() {
	DestroyTex2dRenderTarget();

	updateFunction = &VulkanMgr::UpdateDummy;
	rebuildFunction = &VulkanMgr::RebuildDummy;

	LOG_INFO("[vulkan] 2d graphics backend stopped");
}

void VulkanMgr::UpdateGraphicsInfo() {
	graphicsInfo.image_data = vector<u8>(currentSize);

	float aspect_ratio = (float)graphicsInfo.win_width / graphicsInfo.win_height;

	if (aspect_ratio > graphicsInfo.ascpect_ratio) {
		graphicsInfo.y_offset = 0;

		graphicsInfo.texels_per_pixel = graphicsInfo.win_height / graphicsInfo.y_;

		graphicsInfo.x_offset = (graphicsInfo.win_width - (graphicsInfo.texels_per_pixel * graphicsInfo.x_)) / 2;
	}
	else {
		graphicsInfo.x_offset = 0;

		graphicsInfo.texels_per_pixel = graphicsInfo.win_width / graphicsInfo.x_;

		graphicsInfo.y_offset = (graphicsInfo.win_height - (graphicsInfo.texels_per_pixel * graphicsInfo.y_)) / 2;
	}

	LOG_WARN("Aspect ratio: ", aspect_ratio);
	LOG_WARN("Win X: ", graphicsInfo.win_width, "; LCD X Offset: ", graphicsInfo.x_offset, "; LCD X Pixels: ", graphicsInfo.x_ * graphicsInfo.texels_per_pixel);
	LOG_WARN("Win Y: ", graphicsInfo.win_height, "; LCD Y Offset: ", graphicsInfo.y_offset, "; LCD Y Pixels: ", graphicsInfo.y_ * graphicsInfo.texels_per_pixel);
}

void VulkanMgr::RenderFrame() {
	u32 image_index = 0;
	static u32 frame_index = 0;

	if (VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, acquireSemaphore, nullptr, &image_index); result != VK_SUCCESS) {
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
			(this->*rebuildFunction)();
		}
		else {
			LOG_ERROR("[vulkan] acquire image from swapchain");
		}
		return;
	}

	if (vkResetCommandPool(device, commandPools[frame_index], 0) != VK_SUCCESS) {
		LOG_ERROR("[vulkan] reset command pool");
	}

	VkCommandBufferBeginInfo commandbuffer_info = {};
	commandbuffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandbuffer_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	{
		VkCommandBuffer commandBuffer = commandBuffers[frame_index];
		if (vkBeginCommandBuffer(commandBuffer, &commandbuffer_info) != VK_SUCCESS) {
			LOG_ERROR("[vulkan] begin command buffer");
		}

		// render commands
		VkRenderPassBeginInfo begin_info = {};
		begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		begin_info.renderPass = renderPass;
		begin_info.framebuffer = frameBuffers[image_index];
		begin_info.renderArea = { {0, 0}, {graphicsInfo.win_width, graphicsInfo.win_height} };
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

	if (vkWaitForFences(device, 1, &renderFence, VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
		LOG_ERROR("[vulkan] wait for fences");
	}
	if (vkResetFences(device, 1, &renderFence) != VK_SUCCESS) {
		LOG_ERROR("[vulkan] reset fences");
	}

	// submit buffer to queue
	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &commandBuffers[frame_index];
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = &acquireSemaphore;
	submit_info.pWaitDstStageMask = &waitFlags;
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = &releaseSemaphore;
	if (vkQueueSubmit(queue, 1, &submit_info, renderFence) != VK_SUCCESS) {
		LOG_ERROR("[vulkan] submit command buffer to queue");
	}

	// present
	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.pSwapchains = &swapchain;
	present_info.swapchainCount = 1;
	present_info.pImageIndices = &image_index;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &releaseSemaphore;
	
	if (VkResult result = vkQueuePresentKHR(queue, &present_info); result != VK_SUCCESS) {
		/*
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
			RebuildSwapchain();
		}
		*/
	}

	++frame_index %= FRAMES_IN_FLIGHT;
}

void VulkanMgr::BindPipelinesDummy(VkCommandBuffer& _command_buffer) {
	return;
}

void VulkanMgr::UpdateDummy() {
	return;
}

void VulkanMgr::RebuildDummy() {
	RebuildSwapchain();
}

void VulkanMgr::BindPipelines2d(VkCommandBuffer& _command_buffer) {
	vkCmdBindPipeline(_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tex2dPipeline);
	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(_command_buffer, 0, 1, &tex2dVertexBuffer.buffer, &offset);
	vkCmdBindIndexBuffer(_command_buffer, tex2dIndexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdBindDescriptorSets(_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tex2dPipelineLayout, 0, 1, &tex2dDescSet, 0, nullptr);
	vkCmdSetViewport(_command_buffer, 0, 1, &viewport);
	vkCmdSetScissor(_command_buffer, 0, 1, &scissor);
	vkCmdDrawIndexed(_command_buffer, sizeof(indexData) / sizeof(u32), 1, 0, 0, 0);
}

void VulkanMgr::UpdateGpuData() {
	(this->*updateFunction)();
}

void VulkanMgr::UpdateTex2d() {
	if (vkWaitForFences(device, 1, &tex2dUpdateFence[tex2dUpdateIndex], VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
		LOG_ERROR("[vulkan] wait for texture2d update fence");
	}
	if (vkResetFences(device, 1, &tex2dUpdateFence[tex2dUpdateIndex]) != VK_SUCCESS) {
		LOG_ERROR("[vulkan] reset texture2d update fence");
	}

	memcpy(mappedImageData[tex2dUpdateIndex], graphicsInfo.image_data.data(), graphicsInfo.image_data.size());
	
	if (vkResetCommandPool(device, tex2dCommandPool[tex2dUpdateIndex], 0) != VK_SUCCESS) {
		LOG_ERROR("[vulkan] reset texture2d command pool");
	}

	VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	if (vkBeginCommandBuffer(tex2dCommandBuffer[tex2dUpdateIndex], &beginInfo) != VK_SUCCESS) {
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
		imageBarrier.image = tex2dImage.image;
		imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBarrier.subresourceRange.levelCount = 1;
		imageBarrier.subresourceRange.layerCount = 1;
		imageBarrier.srcAccessMask = VK_ACCESS_NONE;
		imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		vkCmdPipelineBarrier(tex2dCommandBuffer[tex2dUpdateIndex], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);
	}

	VkBufferImageCopy region = {};
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.layerCount = 1;
	region.imageExtent = { graphicsInfo.win_width, graphicsInfo.win_height, 1 };
	vkCmdCopyBufferToImage(tex2dCommandBuffer[tex2dUpdateIndex], tex2dStagingBuffer[tex2dUpdateIndex].buffer, tex2dImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	{
		VkImageMemoryBarrier imageBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarrier.image = tex2dImage.image;
		imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBarrier.subresourceRange.levelCount = 1;
		imageBarrier.subresourceRange.layerCount = 1;
		imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		vkCmdPipelineBarrier(tex2dCommandBuffer[tex2dUpdateIndex], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);
	}

	if (vkEndCommandBuffer(tex2dCommandBuffer[tex2dUpdateIndex]) != VK_SUCCESS) {
		LOG_ERROR("[vulkan] end command buffer texture2d update");
	}

	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &tex2dCommandBuffer[tex2dUpdateIndex];
	if (vkQueueSubmit(queue, 1, &submitInfo, tex2dUpdateFence[tex2dUpdateIndex]) != VK_SUCCESS) {
		LOG_ERROR("[vulkan] queue submit texture2d update");
	}

	++tex2dUpdateIndex %= 2;
}

void VulkanMgr::Rebuild2d() {
	WaitIdle();

	RebuildSwapchain();

	vkUnmapMemory(device, tex2dStagingBuffer[0].memory);
	vkUnmapMemory(device, tex2dStagingBuffer[1].memory);
	DestroyImage(tex2dImage);
	DestroyBuffer(tex2dStagingBuffer[0]);
	DestroyBuffer(tex2dStagingBuffer[1]);
	InitTex2dBuffers();

	UpdateGraphicsInfo();
	UpdateTex2d();

	vkDestroyDescriptorPool(device, tex2dDescPool, nullptr);
	vkDestroyDescriptorSetLayout(device, tex2dDescSetLayout[0], nullptr);
	InitTex2dDescriptorSets();

	// upload dummy data
	UpdateTex2d();
}

void VulkanMgr::Rebuild3d() {
	RebuildSwapchain();
}

bool VulkanMgr::InitVulkanInstance(std::vector<const char*>& _sdl_extensions) {
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
	vk_app_info.pApplicationName = APP_TITLE.c_str();
	vk_app_info.applicationVersion = VK_MAKE_VERSION(GBX_VERSION_MAJOR, GBX_VERSION_MINOR, GBX_VERSION_PATCH);
	vk_app_info.apiVersion = VK_API_VERSION_1_3;

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
		if (strcmp(n, VK_VALIDATION) == 0) {
			LOG_WARN("[vulkan] ", n, " layer enabled");
		}
		else {
			LOG_INFO("[vulkan] ", n, " layer enabled");
		}
	}
	for (const auto& n : _sdl_extensions) {
		LOG_INFO("[vulkan] ", n, " extension enabled");
	}

	return true;
}

bool VulkanMgr::InitPhysicalDevice() {
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
	}
	else {
		LOG_INFO("[vulkan] fallback to first entry");
		physicalDevice = vk_physical_devices[0];
		vkGetPhysicalDeviceProperties(vk_physical_devices[0], &physicalDeviceProperties);
	}

	SetGPUInfo();
	LOG_INFO("[vulkan] ", physicalDeviceProperties.deviceName, " (", driverVersion, ") selected");

	return true;
}

bool VulkanMgr::InitLogicalDevice(std::vector<const char*>& _device_extensions) {
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

void VulkanMgr::SetGPUInfo() {
	u16 vendor_id = physicalDeviceProperties.vendorID & 0xFFFF;
	vendor = get_vendor(vendor_id);

	u32 driver = physicalDeviceProperties.driverVersion;
	if (vendor_id == ID_NVIDIA) {
		driverVersion = std::format("{}.{}", (driver >> 22) & 0x3ff, (driver >> 14) & 0xff);
	}
	// TODO: doesn't work properly for AMD cards, will get fixed in the future
	else if (vendor_id == ID_AMD) {
		driverVersion = std::format("{}.{}", driver >> 14, driver & 0x3fff);
	}
	else {
		driverVersion = "";
	}
}

bool VulkanMgr::InitSurface() {
	if (!SDL_Vulkan_CreateSurface(window, vulkanInstance, &surface)) {
		LOG_ERROR("[vulkan] ", SDL_GetError());
		return false;
	}
	else {
		return true;
	}
}

bool VulkanMgr::InitSwapchain(VkImageUsageFlags _flags) {
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

	presentMode = VK_PRESENT_MODE_FIFO_KHR;
	minImageCount = 3;

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
	swapchain_info.presentMode = presentMode;								// simple image fifo (vsync)
	swapchain_info.clipped = VK_FALSE;										// draw only visible window areas if true (clipping)
	swapchain_info.oldSwapchain = oldSwapchain;
	swapchain_info.pNext = nullptr;

	if (vkCreateSwapchainKHR(device, &swapchain_info, nullptr, &swapchain) != VK_SUCCESS) {
		LOG_ERROR("[vulkan] couldn't create swapchain");
		return false;
	}

	graphicsInfo.win_width = surface_capabilites.currentExtent.width;
	graphicsInfo.win_height = surface_capabilites.currentExtent.height;
	viewport = { .0f, .0f, (float)graphicsInfo.win_width, (float)graphicsInfo.win_height, .0f, 1.f};
	scissor = { {0, 0}, {graphicsInfo.win_width, graphicsInfo.win_height} };

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

	return true;
}

bool VulkanMgr::InitRenderPass() {
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

bool VulkanMgr::InitFrameBuffers() {
	frameBuffers.clear();
	frameBuffers.resize(images.size());
	for (u32 i = 0; i < images.size(); i++) {
		VkFramebufferCreateInfo framebuffer_info = {};
		framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_info.renderPass = renderPass;
		framebuffer_info.attachmentCount = 1;
		framebuffer_info.pAttachments = &imageViews[i];
		framebuffer_info.width = graphicsInfo.win_width;
		framebuffer_info.height = graphicsInfo.win_height;
		framebuffer_info.layers = 1;

		if (vkCreateFramebuffer(device, &framebuffer_info, nullptr, &frameBuffers[i]) != VK_SUCCESS) {
			LOG_ERROR("[vulkan] create framebuffer ", i);
			return false;
		}
	}

	return true;
}

bool VulkanMgr::InitCommandBuffers() {
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
	if (vkCreateFence(device, &fence_info, nullptr, &renderFence) != VK_SUCCESS) {
		LOG_ERROR("[vulkan] create renderFence");
		return false;
	}

	if (!InitSemaphore(acquireSemaphore) || !InitSemaphore(releaseSemaphore)) {
		return false;
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

bool VulkanMgr::InitSemaphore(VkSemaphore& _semaphore) {
	VkSemaphoreCreateInfo sem_info = {};
	sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	if (vkCreateSemaphore(device, &sem_info, nullptr, &_semaphore) != VK_SUCCESS) {
		LOG_ERROR("[vulkan] create semaphore");
		return false;
	}
	return true;
}

bool VulkanMgr::InitImgui() {
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

bool VulkanMgr::InitShaderModule(const vector<char>& _byte_code, VkShaderModule& _shader) {
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

bool VulkanMgr::InitPipeline(VkShaderModule& _vertex_shader, VkShaderModule& _fragment_shader, VkPipelineLayout& _layout, VkPipeline& _pipeline, VulkanPipelineBufferInfo& _info, vector<VkDescriptorSetLayout>& _set_layouts) {
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
	vertex_input_state.vertexAttributeDescriptionCount = _info.attrDesc.size();
	vertex_input_state.pVertexAttributeDescriptions = _info.attrDesc.data();
	vertex_input_state.vertexBindingDescriptionCount = _info.bindDesc.size();
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
	layout_info.setLayoutCount = _set_layouts.size();
	layout_info.pSetLayouts = _set_layouts.data();
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
	if (vkCreateGraphicsPipelines(device, nullptr, 1, &pipeline_info, nullptr, &_pipeline) != VK_SUCCESS) {
		LOG_ERROR("[vulkan] create graphics pipeline");
	}

	vkDestroyShaderModule(device, _vertex_shader, nullptr);
	vkDestroyShaderModule(device, _fragment_shader, nullptr);

	return true;
}

bool VulkanMgr::InitBuffer(VulkanBuffer& _buffer, u64 _size, VkBufferUsageFlags _usage, VkMemoryPropertyFlags _memory_properties) {
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

bool VulkanMgr::LoadBuffer(VulkanBuffer& _buffer, void* _data, u64 _size) {
	if (resizableBar) {
		void* data;
		if (vkMapMemory(device, _buffer.memory, 0, _size, 0, &data) != VK_SUCCESS) {
			return false;
		}
		memcpy(data, _data, _size);
	}
	else {
		VulkanBuffer staging_buffer = {};
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

bool VulkanMgr::InitImage(VulkanImage& _image, u32 _width, u32 _height, VkFormat _format, VkImageUsageFlags _usage, VkImageTiling _tiling) {
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

void VulkanMgr::RebuildSwapchain() {
	WaitIdle();
	oldSwapchain = swapchain;
	InitSwapchain(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
	DestroySwapchain(true);

	DestroyFrameBuffers();
	DestroyRenderPass();

	InitRenderPass();
	InitFrameBuffers();
}

void VulkanMgr::DestroySwapchain(const bool& _rebuild) {
	WaitIdle();
	if (_rebuild) {
		vkDestroySwapchainKHR(device, oldSwapchain, nullptr);
	}
	else {
		for (auto& n : imageViews) {
			vkDestroyImageView(device, n, nullptr);
		}
		vkDestroySwapchainKHR(device, swapchain, nullptr);
	}
}

void VulkanMgr::DestroySurface() {
	WaitIdle();
	vkDestroySurfaceKHR(vulkanInstance, surface, nullptr);
}

void VulkanMgr::DestroyFrameBuffers() {
	WaitIdle();
	for (auto& n : frameBuffers) {
		vkDestroyFramebuffer(device, n, nullptr);
	}
	frameBuffers.clear();
}

void VulkanMgr::DestroyRenderPass() {
	WaitIdle();
	vkDestroyRenderPass(device, renderPass, nullptr);
}

void VulkanMgr::DestroyCommandBuffer() {
	WaitIdle();
	for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
		vkDestroyCommandPool(device, commandPools[i], nullptr);
	}
	vkDestroyFence(device, renderFence, nullptr);
	DestroySemaphore(acquireSemaphore);
	DestroySemaphore(releaseSemaphore);
}

void VulkanMgr::DestroyImgui() {
	WaitIdle();
	ImGui_ImplVulkan_Shutdown();
	vkDestroyDescriptorPool(device, imguiDescriptorPool, nullptr);
}

void VulkanMgr::DestroyPipelines() {
	WaitIdle();
	for (const auto& [layout, pipeline] : pipelines) {
		vkDestroyPipeline(device, pipeline, nullptr);
		vkDestroyPipelineLayout(device, layout, nullptr);
	}
	pipelines.clear();
}

void VulkanMgr::DestroyBuffer(VulkanBuffer& _buffer) {
	WaitIdle();
	vkDestroyBuffer(device, _buffer.buffer, nullptr);
	vkFreeMemory(device, _buffer.memory, nullptr);
}

void VulkanMgr::DestroyImage(VulkanImage& _image) {
	WaitIdle();
	vkDestroyImageView(device, _image.image_view, nullptr);
	vkDestroyImage(device, _image.image, nullptr);
	vkFreeMemory(device, _image.memory, nullptr);
}

void VulkanMgr::DestroySemaphore(VkSemaphore& _semaphore) {
	vkDestroySemaphore(device, _semaphore, nullptr);
}

bool VulkanMgr::InitTex2dRenderTarget() {
	for (int i = 0; i < tex2dCommandPool.size(); i++) {
		VkCommandPoolCreateInfo cmd_pool_info = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		cmd_pool_info.queueFamilyIndex = familyIndex;
		if (vkCreateCommandPool(device, &cmd_pool_info, nullptr, &tex2dCommandPool[i]) != VK_SUCCESS) {
			LOG_ERROR("[vulkan] create command pool for tex2d update");
		}

		VkCommandBufferAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		alloc_info.commandPool = tex2dCommandPool[i];
		alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		alloc_info.commandBufferCount = 1;
		if (vkAllocateCommandBuffers(device, &alloc_info, &tex2dCommandBuffer[i]) != VK_SUCCESS) {
			LOG_ERROR("[vulkan] init command buffer for tex2d update");
		}
	}

	// vertex buffer for triangle
	if (!InitBuffer(tex2dVertexBuffer, sizeof(vertexData), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | bufferUsageFlags, memoryPropertyFlags)) {
		LOG_ERROR("[vulkan] init vertex buffer");
		return false;
	}
	if (!LoadBuffer(tex2dVertexBuffer, vertexData, sizeof(vertexData))) {
		LOG_ERROR("[vulkan] copy data to vertex buffer");
		return false;
	}

	// index buffer for triangle
	if (!InitBuffer(tex2dIndexBuffer, sizeof(indexData), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | bufferUsageFlags, memoryPropertyFlags)) {
		LOG_ERROR("[vulkan] init index buffer");
		return false;
	}
	if (!LoadBuffer(tex2dIndexBuffer, indexData, sizeof(indexData))) {
		LOG_ERROR("[vulkan] copy data to index buffer");
		return false;
	}

	VkFenceCreateInfo fence_info = {};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	if (vkCreateFence(device, &fence_info, nullptr, &tex2dUpdateFence[0]) != VK_SUCCESS ||
		vkCreateFence(device, &fence_info, nullptr, &tex2dUpdateFence[1]) != VK_SUCCESS) {
		LOG_ERROR("[vulkan] create tex2dUpdateFence");
		return false;
	}

	if (!InitTex2dBuffers()) { return false; }

	if (!InitTex2dSampler()) { return false; }

	if (!InitTex2dDescriptorSets()) { return false; }

	// shader to present 2d texture
	if (!InitTex2dPipeline()) {
		LOG_ERROR("[vulkan] init shader for 2d texture output");
		return false;
	}

	if (graphicsInfo.en2d) {
		bindPipelines = &VulkanMgr::BindPipelines2d;
		rebuildFunction = &VulkanMgr::Rebuild2d;
		updateFunction = &VulkanMgr::UpdateTex2d;
	}
	else {

	}

	LOG_INFO("[vulkan] 2d graphics backend initialized");

	return true;
}

bool VulkanMgr::InitTex2dSampler() {
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

	if (vkCreateSampler(device, &create_info, nullptr, &samplerTex2d) != VK_SUCCESS) {
		LOG_ERROR("[vulkan] create sampler for Texture2d");
		return false;
	}

	return true;
}

bool VulkanMgr::InitTex2dDescriptorSets() {
	{
		VkDescriptorPoolSize poolSizes[] = {
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
		};
		VkDescriptorPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		createInfo.maxSets = 1;
		createInfo.poolSizeCount = sizeof(poolSizes) / sizeof(poolSizes[0]);
		createInfo.pPoolSizes = poolSizes;
		if (vkCreateDescriptorPool(device, &createInfo, nullptr, &tex2dDescPool) != VK_SUCCESS) {
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
		if (vkCreateDescriptorSetLayout(device, &createInfo, nullptr, tex2dDescSetLayout.data()) != VK_SUCCESS) {
			LOG_ERROR("[vulkan] create descriptor pool layout");
			return false;
		}

		VkDescriptorSetAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		allocateInfo.descriptorPool = tex2dDescPool;
		allocateInfo.descriptorSetCount = (u32)tex2dDescSetLayout.size();
		allocateInfo.pSetLayouts = tex2dDescSetLayout.data();
		if (vkAllocateDescriptorSets(device, &allocateInfo, &tex2dDescSet) != VK_SUCCESS) {
			LOG_ERROR("[vulkan] allocate descriptor sets");
			return false;
		}

		// info for descriptor of texture2d to sample from -> fragment shader
		VkDescriptorImageInfo imageInfo = { samplerTex2d, tex2dImage.image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		auto descriptorWrites = std::vector<VkWriteDescriptorSet>(1);
		descriptorWrites[0] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		descriptorWrites[0].dstSet = tex2dDescSet;
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[0].pImageInfo = &imageInfo;
		vkUpdateDescriptorSets(device, (u32)descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	}

	return true;
}

bool VulkanMgr::InitTex2dBuffers() {
	// staging buffer for texture upload
	currentSize = graphicsInfo.win_width * graphicsInfo.win_height * 4;
	for(auto& n : tex2dStagingBuffer){
		if (!InitBuffer(n, currentSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
			LOG_ERROR("[vulkan] create staging buffer for main texture");
			return false;
		}
	}

	// image buffer for shader usage
	if (!InitImage(tex2dImage, graphicsInfo.win_width, graphicsInfo.win_height, tex2dFormat, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, /*resizableBar ? VK_IMAGE_TILING_LINEAR :*/ VK_IMAGE_TILING_OPTIMAL)) {
		LOG_ERROR("[vulkan] create target 2d texture for virtual hardware");
		return false;
	}

	for (int i = 0; auto & n : tex2dStagingBuffer) {
		if (vkMapMemory(device, n.memory, 0, currentSize, 0, &mappedImageData[i]) != VK_SUCCESS) {
			LOG_ERROR("[vulkan] map image memory");
			return false;
		}
		i++;
	}

	return true;
}

void VulkanMgr::DestroyTex2dRenderTarget() {
	WaitIdle();
	DestroyTex2dSampler();
	DestroyTex2dShader();
	vkDestroyDescriptorPool(device, tex2dDescPool, nullptr);
	vkDestroyDescriptorSetLayout(device, tex2dDescSetLayout[0], nullptr);
	for (auto& n : tex2dStagingBuffer) {
		DestroyBuffer(n);
	}
	DestroyImage(tex2dImage);
	DestroyBuffer(tex2dVertexBuffer);
	DestroyBuffer(tex2dIndexBuffer);
	for (auto& n : tex2dCommandPool) {
		vkDestroyCommandPool(device, n, nullptr);
	}
	for (auto& n : tex2dUpdateFence) {
		vkDestroyFence(device, n, nullptr);
	}

	bindPipelines = &VulkanMgr::BindPipelinesDummy;
}

void VulkanMgr::DestroyTex2dSampler() {
	WaitIdle();
	vkDestroySampler(device, samplerTex2d, nullptr);
}

bool VulkanMgr::InitTex2dPipeline() {
	VkShaderModule vertex_shader;
	VkShaderModule fragment_shader;

	auto vertex_shader_data = vector<char>(tex2dVertShader.data(), tex2dVertShader.data() + tex2dVertShader.size());
	auto fragment_shader_data = vector<char>(tex2dFragShader.data(), tex2dFragShader.data() + tex2dFragShader.size());

	if (InitShaderModule(vertex_shader_data, vertex_shader) && InitShaderModule(fragment_shader_data, fragment_shader)) {
		VulkanPipelineBufferInfo buffer_info = {};
		VkVertexInputBindingDescription bind_desc = {};
		bind_desc.binding = 0;
		bind_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		bind_desc.stride = sizeof(float) * 2;
		buffer_info.bindDesc.push_back(bind_desc);

		VkVertexInputAttributeDescription attr_desc[1] = {};
		attr_desc[0].binding = 0;							// vertex data
		attr_desc[0].location = 0;
		attr_desc[0].format = VK_FORMAT_R32G32_SFLOAT;
		attr_desc[0].offset = 0;
		buffer_info.attrDesc = vector<VkVertexInputAttributeDescription>(attr_desc, attr_desc + (sizeof(attr_desc) / sizeof(attr_desc[0])));

		InitPipeline(vertex_shader, fragment_shader, tex2dPipelineLayout, tex2dPipeline, buffer_info, tex2dDescSetLayout);
		return true;
	}
	else {
		return false;
	}
}

void VulkanMgr::DestroyTex2dShader() {
	WaitIdle();
	vkDestroyPipeline(device, tex2dPipeline, nullptr);
	vkDestroyPipelineLayout(device, tex2dPipelineLayout, nullptr);
}

void VulkanMgr::WaitIdle() {
	if (vkDeviceWaitIdle(device) != VK_SUCCESS) {
		LOG_ERROR("[vulkan] GPU wait idle");
	}
}

void VulkanMgr::NextFrameImGui() const {
	ImGui_ImplVulkan_NewFrame();
}

void VulkanMgr::EnumerateShaders() {
	LOG_INFO("[vulkan] enumerating shader source files");

	enumeratedShaderFiles = get_files_in_path(SHADER_FOLDER);
	shaderSourceFiles = vector<pair<string, string>>();

	for (const auto& n : enumeratedShaderFiles) {
		const auto file_1 = split_string(n, "/").back();

		if (SHADER_TYPES.at(VERT_EXT) == SHADER_TYPES.at(split_string(file_1, ".").back())) {

			for (const auto& m : enumeratedShaderFiles) {
				const auto file_2 = split_string(m, "/").back();

				if (split_string(file_1, ".").front().compare(split_string(file_2, ".").front()) == 0 &&
					SHADER_TYPES.at(FRAG_EXT) == SHADER_TYPES.at(split_string(file_2, ".").back())) {
					shaderSourceFiles.emplace_back(n, m);
				}
			}
		}
	}
	LOG_INFO("[vulkan] ", shaderSourceFiles.size(), " shader(s) found");

	if (shaderSourceFiles.empty()) {
		graphicsInfo.shaders_compilation_finished = true;
		return;
	}
	else {
		graphicsInfo.shaders_compiled = 0;
		graphicsInfo.shaders_total = (int)(shaderSourceFiles.size());
		graphicsInfo.shaders_compilation_finished = false;

		compiler = shaderc_compiler_initialize();
		options = shaderc_compile_options_initialize();
		shaderc_compile_options_set_auto_map_locations(options, true);
	}
}

void VulkanMgr::CompileNextShader() {
	bool compiled = true;
	vector<char> vertex_byte_code;
	if(!compile_shader(vertex_byte_code, shaderSourceFiles[graphicsInfo.shaders_compiled].first, compiler, options)){
		compiled = false;
	}
	vector<char> fragment_byte_code;
	if (!compile_shader(fragment_byte_code, shaderSourceFiles[graphicsInfo.shaders_compiled].second, compiler, options)) {
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

	graphicsInfo.shaders_compiled++;
	if (graphicsInfo.shaders_compiled == graphicsInfo.shaders_total) {
		graphicsInfo.shaders_compilation_finished = true;

		shaderc_compiler_release(compiler);
		shaderc_compile_options_release(options);
	}
}

u32 VulkanMgr::FindMemoryTypes(u32 _type_filter, VkMemoryPropertyFlags _mem_properties) {
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

void VulkanMgr::DetectResizableBar() {
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
	}
	else {
		LOG_WARN(_callback_data->pMessage);
	}
	return VK_FALSE;
}

VkDebugUtilsMessengerEXT VulkanMgr::RegisterDebugCallback() {
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
	if (!read_data(source_text_vec, _shader_source_file, false)) {
		LOG_ERROR("[vulkan] read shader source ", _shader_source_file);
		return false;
	}

	size_t source_size = source_text_vec.size();
	shaderc_shader_kind type = SHADER_TYPES.at(split_string(_shader_source_file, ".").back());
	string file_name_str = split_string(_shader_source_file, "/").back();
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
	auto file_name_parts = split_string(file_name_str, ".");
	string out_file_path = SPIR_V_FOLDER + file_name_parts.front() + "_" + file_name_parts.back() + "." + SPIRV_EXT;
	write_data(byte_code_vec, out_file_path, true, true);
	shaderc_result_release(result);

	LOG_INFO("[vulkan] ", file_name_str, " compiled");
	return true;
}