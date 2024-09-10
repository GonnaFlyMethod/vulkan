#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>
#include <set>

#include "Utilities.h"


class VulkanRenderer
{
public:
	int init(GLFWwindow* newWindow);
	void cleanup();

private:
	GLFWwindow *window;

	// Vulkan Components
	VkInstance instance;

	struct {
		VkPhysicalDevice physicalDevice;
		VkDevice logicalDevice;
	} mainDevice;

	VkQueue graphicsQueue;
	VkQueue presentationQueue;

	VkSurfaceKHR surface;

	// Vulkan functions
	void createInstance();
	void createLogicalDevice();
	void createSurface();

	void getPhysicalDevice();

	bool checkInstanceExtensionsSupport(std::vector<const char*>* checkExtensions);
	
	bool checkDeviceSuitable(VkPhysicalDevice device);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);

	QueueFamilyIndices getQueueFamilies(VkPhysicalDevice device);
	SwapChainDetails getSwapChainDetails(VkPhysicalDevice device);
};

