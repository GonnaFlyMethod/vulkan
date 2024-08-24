#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>


class VulkanRenderer
{
public:
	int init(GLFWwindow* newWindow);

private:
	GLFWwindow *window;

	// Vulkan Components
	VkInstance instance;

	// Vulkan functions
	void createInstance();
	bool checkInstanceExtensionsSupport(std::vector<const char*>* checkExtensions);
};

