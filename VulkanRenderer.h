#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>
#include <set>
#include <algorithm>

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

	// Vulkan compinents
	
	// Main
	struct {
		VkPhysicalDevice physicalDevice;
		VkDevice logicalDevice;
	} mainDevice;

	VkQueue graphicsQueue;
	VkQueue presentationQueue;

	VkSurfaceKHR surface;

	VkSwapchainKHR swapchain;

	std::vector<SwapchainImage> swapchainImages;

	// Utility
	VkFormat swapchainImageFormat;
	VkExtent2D swapchainExtent;

	// Vulkan functions
	void createInstance();
	void createDebugCallback();
	void createLogicalDevice();
	void createSurface();
	void createSwapChain();
	void createGraphicsPipeline();

	void getPhysicalDevice();

	bool checkInstanceExtensionsSupport(std::vector<const char*>* checkExtensions);
	
	bool checkDeviceSuitable(VkPhysicalDevice device);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);

	QueueFamilyIndices getQueueFamilies(VkPhysicalDevice device);
	SwapChainDetails getSwapChainDetails(VkPhysicalDevice device);

	VkSurfaceFormatKHR chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &formats);
	VkPresentModeKHR chooseBestPresentationMode(const std::vector<VkPresentModeKHR> presentationModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities);

	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

	VkShaderModule createShaderModule(const std::vector<char> &code);
};

