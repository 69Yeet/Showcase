#ifndef SHADER_HPP
#define SHADER_HPP

#include <glm/matrix.hpp>

struct ShaderData
{
	glm::mat4 projection;
	glm::mat4 view;
	glm::mat4 model[3];
	glm::vec4 lightPos{ 0.0f, -10.0f, 10.0f, 0.0f };
	uint32_t selected{1};
} shaderData{};

struct ShaderDataBuffer {
	VkDeviceMemory memory{ VK_NULL_HANDLE };
	VkMemoryAllocateInfo allocationInfo{};
	VkBuffer buffer{ VK_NULL_HANDLE };
	VkDeviceAddress deviceAddress{};
};

#endif
