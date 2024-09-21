#include "VulkanRenderer.h"


int VulkanRenderer::init(GLFWwindow * newWindow) {
	window = newWindow;

	try {
		createInstance();
		createSurface();
		getPhysicalDevice();
		createLogicalDevice();
		createSwapChain();
		createGraphicsPipeline();
	}
	catch(const std::runtime_error &e){
		printf("ERROR: %s\n", e.what());

		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

void VulkanRenderer::cleanup()
{
	for (auto image : swapchainImages) {
		vkDestroyImageView(mainDevice.logicalDevice, image.imageView, nullptr);
	}

	vkDestroySwapchainKHR(mainDevice.logicalDevice, swapchain, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyDevice(mainDevice.logicalDevice, nullptr);
	vkDestroyInstance(instance, nullptr);
}

void VulkanRenderer::createInstance() {
	// Information about the application itself
	// Only for developer's convenience
	VkApplicationInfo appInfo = {};
	
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan App"; // Custom name of the application
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0); // Custom Engine version
	appInfo.apiVersion = VK_API_VERSION_1_3; // Vulkan version

	VkInstanceCreateInfo createInfo = {};

	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	// Create list to instance extenstions
	std::vector<const char*> instanceExtensions = std::vector<const char *>();
	
	// Set up extensions Instance will use
	uint32_t glfwExtensionsCount = 0; // GLFW may require multiple extensions
	const char** glfwExtensions; // Extensions
	
	// Get GLFW Extensions
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);
	
	// Add GLFW extensions to list of extensions

	for (size_t i = 0; i < glfwExtensionsCount; i++) {
		instanceExtensions.push_back(glfwExtensions[i]);
	}

	bool supportedExtensionsResult = checkInstanceExtensionsSupport(&instanceExtensions);

	// Check Instance extensions
	if (!supportedExtensionsResult) {
		throw std::runtime_error("vk instance does not support required extensions");
	}

	createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
	createInfo.ppEnabledExtensionNames = instanceExtensions.data();

	// Setting validaition layers that Instance will use
	createInfo.enabledLayerCount = 0;
	createInfo.ppEnabledLayerNames = nullptr;

	// Creating instance
	VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
	
	if (result != VK_SUCCESS) {
		throw std::runtime_error("error occurred while creating vulkan instance");
	}
}

void VulkanRenderer::createLogicalDevice()
{
	// TODO: cache index by introducing a property in the class/structure
	QueueFamilyIndices indices = getQueueFamilies(mainDevice.physicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	
	std::set<int> queueFamilyIndeces = { 
		indices.graphicsFamily, indices.presentationFamily
	};

	for (int queueFamilyIndex: queueFamilyIndeces) {
		VkDeviceQueueCreateInfo queueCreateInfo = {};

		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamilyIndex; // Index of a family to create queue from
		queueCreateInfo.queueCount = 1;  // Number of queues to create;

		float priority = 1.0f;

		// Normalized priority for handling multiple queues (1 = highest priority, 0 - lowest)
		queueCreateInfo.pQueuePriorities = &priority;

		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkDeviceCreateInfo deviceCreateInfo = {};

	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());  // Number of Queue create infos

	//  List of queue create infos, so device can create required queues
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data(); 
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());  // Number of enabled logical device extensions
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data(); // List of enabled logical device extensions
	
	// Physical Device Features the Logical Device will be using
	VkPhysicalDeviceFeatures deviceFeatures = {};

	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

	// Create the logical device fo the given physical device
	VkResult result = vkCreateDevice(
		mainDevice.physicalDevice, &deviceCreateInfo, nullptr, &mainDevice.logicalDevice);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create a Logical Device!");
	}


	// From give logical device, of given queue family, of given Queue index (0 since only one queue),
	// place reference in given VkQueue
	vkGetDeviceQueue(mainDevice.logicalDevice, indices.graphicsFamily, 0, &graphicsQueue);
	vkGetDeviceQueue(mainDevice.logicalDevice, indices.presentationFamily, 0, &presentationQueue);
}

void VulkanRenderer::createSurface()
{
	// Creating an info struct, runs the create surface function, retur
	VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
	
	if (result != VK_SUCCESS) {
		throw std::runtime_error("error occurred while creating surface");
	}
}

void VulkanRenderer::createSwapChain()
{
	SwapChainDetails swapChainDetails = getSwapChainDetails(mainDevice.physicalDevice);

	VkSurfaceFormatKHR surfaceFormat = chooseBestSurfaceFormat(swapChainDetails.formats);
	VkPresentModeKHR presentMode = chooseBestPresentationMode(swapChainDetails.presentationModes);
	VkExtent2D extent = chooseSwapExtent(swapChainDetails.surfaceCapabilities);
	
	// How many images are in the swap chain. Get one more than the minimum to allow triple buffering
	uint32_t imageCount = swapChainDetails.surfaceCapabilities.minImageCount + 1;

	// If maxImageCount = 0, then it's limitless
	if (swapChainDetails.surfaceCapabilities.maxImageCount > 0 && 
		swapChainDetails.surfaceCapabilities.maxImageCount < imageCount) {
		
		imageCount = swapChainDetails.surfaceCapabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
	swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCreateInfo.surface = surface;														// Swapchain surface
	swapChainCreateInfo.imageFormat = surfaceFormat.format;
	swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapChainCreateInfo.presentMode = presentMode;
	swapChainCreateInfo.imageExtent = extent;													// Frame buffer resolution in swap chain
	swapChainCreateInfo.minImageCount = imageCount;												// Minimum images in swapchain
	swapChainCreateInfo.imageArrayLayers = 1;													// Number of layers for each image in swap chain
	swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;						// Image usage in swap chain
	swapChainCreateInfo.preTransform = swapChainDetails.surfaceCapabilities.currentTransform;	// Transform to perform on swapchain iamges
	swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;						// How to handle to handle blending images with external graphics (e.g. other windows)
	swapChainCreateInfo.clipped = VK_TRUE;														// Whether to clip parts of image not in view (e.g. behind another window, off screen(not in focus), etc.)
	
	QueueFamilyIndices indices = getQueueFamilies(mainDevice.physicalDevice);

	// If Graphics and Presentation families are different, then swapchain must let images be shared between families
	if (indices.graphicsFamily != indices.presentationFamily) {
		
		// Qeueus to share between
		uint32_t queueFamilyIndeces[] = {
			(uint32_t)indices.graphicsFamily,
			(uint32_t)indices.presentationFamily
		};
		
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapChainCreateInfo.queueFamilyIndexCount = 2;						// Number of queues to share images between
		swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndeces;		// Array of queues to share between
	}
	else {
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapChainCreateInfo.queueFamilyIndexCount = 0;
		swapChainCreateInfo.pQueueFamilyIndices = nullptr;
	}

	// If old swap chain been destroyed and this one replaces it, then we can link old one to hand over responsobilities
	swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	VkResult result = vkCreateSwapchainKHR(
		mainDevice.logicalDevice, &swapChainCreateInfo, nullptr, &swapchain);
	
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create swapchain");
	}

	// Store for later reference
	swapchainImageFormat = surfaceFormat.format;
	swapchainExtent = extent;

	// Get Swap chain images

	uint32_t swapchainImageCount;
	vkGetSwapchainImagesKHR(mainDevice.logicalDevice, swapchain, &swapchainImageCount, nullptr);

	std::vector<VkImage> images(swapchainImageCount);

	vkGetSwapchainImagesKHR(mainDevice.logicalDevice, swapchain, &swapchainImageCount, images.data());

	for (VkImage image : images) {
		SwapchainImage swapchainImage = {};
		swapchainImage.image = image;
		swapchainImage.imageView = createImageView(image, swapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
		
		swapchainImages.push_back(swapchainImage);
	}
}

void VulkanRenderer::createGraphicsPipeline()
{
	// Read in SPIR-V code of shaders
	auto vertexShaderCode = readFile("Shaders/vert.spv");
	auto fragmentShaderCode = readFile("Shaders/frag.spv");

	// Build Shader Modules to link to Graphics Pipeline
	VkShaderModule vertexShaderModule = createShaderModule(vertexShaderCode);
	VkShaderModule fragmentShaderModule = createShaderModule(fragmentShaderCode);
	
	// Vertex Stage creation information
	VkPipelineShaderStageCreateInfo vertexShaderCreateInfo = {};
	vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;		// Shader stage name
	vertexShaderCreateInfo.module = vertexShaderModule;					// Shader module to be used by stage
	vertexShaderCreateInfo.pName = "main";								// Entrypoint func's name for the shader

	// Fragment stage creation information
	VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo = {};
	fragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;		// Shader stage name
	fragmentShaderCreateInfo.module = fragmentShaderModule;				// Shader module to be used by stage
	fragmentShaderCreateInfo.pName = "main";							// Entrypoint func's name for the shader

	// Graphics pipeline creation info requires array of shader stage infos
	VkPipelineShaderStageCreateInfo shaderStage[] = {
		vertexShaderCreateInfo, fragmentShaderCreateInfo
	};

	// Creating pipeline


	// Destroy shader modules, no longer needed after pipeline created
	vkDestroyShaderModule(mainDevice.logicalDevice, fragmentShaderModule, nullptr);
	vkDestroyShaderModule(mainDevice.logicalDevice, vertexShaderModule, nullptr);
}

void VulkanRenderer::getPhysicalDevice()
{
	uint32_t deviceCount = 0;

	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	
	if (deviceCount == 0) {
		throw std::runtime_error("Can't find GPU that support Vulkan instance");
	}

	std::vector<VkPhysicalDevice> deviceList(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, deviceList.data());

	for (const auto& device : deviceList) {
		bool isDeviceSuitable = checkDeviceSuitable(device);
		
		if (isDeviceSuitable) {
			mainDevice.physicalDevice = device;
			break;
		}
	}
}

bool VulkanRenderer::checkInstanceExtensionsSupport(std::vector<const char*>* checkExtensions)
{
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	// Create a list of VkExtensionProperties using count
	std::vector<VkExtensionProperties> extensions(extensionCount);

	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
	
	for (const auto& checkExtension : *checkExtensions) {
		bool hasExtension = false;

		for (const auto& extension : extensions) {
			if (strcmp(checkExtension, extension.extensionName)) {
				hasExtension = true;
				break;
			}
		}

		if (!hasExtension) {
			return false;
		}

	}

	return true;
}

bool VulkanRenderer::checkDeviceSuitable(VkPhysicalDevice device)
{
	/*VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);*/

	QueueFamilyIndices indices = getQueueFamilies(device);
	bool extensionsSupported = checkDeviceExtensionSupport(device);

	bool swapChainValid = false;

	if (extensionsSupported) {

		SwapChainDetails swapChainDetails = getSwapChainDetails(device);

		swapChainValid = !swapChainDetails.presentationModes.empty() && !swapChainDetails.formats.empty();
	}

	return indices.isValid() && extensionsSupported && swapChainValid;
}

bool VulkanRenderer::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	if (extensionCount == 0) {
		return false;
	}

	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());

	bool hasExtension = false;

	for (const auto& deviceExtension : deviceExtensions) {

		for (const auto& extension : extensions) {
			if (strcmp(deviceExtension, extension.extensionName)) {
				hasExtension = true;
				
				break;
			}
		}
	}

	return hasExtension;
}

QueueFamilyIndices VulkanRenderer::getQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
	
	std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamilyCount);

	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyList.data());

	int i = 0;
	
	for (const auto &queueFamily: queueFamilyList) {
		
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
		}

		// Check of Queue family supports presentation
		VkBool32 presentationSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentationSupport);

		// The index of the presentation queue can be equal to the index of graphics queue
		if (queueFamily.queueCount > 0 && presentationSupport) {
			indices.presentationFamily = i;
		}

		if (indices.isValid()) {
			break;
		}

		i++;
	}

	return indices;
}

SwapChainDetails VulkanRenderer::getSwapChainDetails(VkPhysicalDevice device)
{
	SwapChainDetails swapChainDetails{};

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
		device, surface, &swapChainDetails.surfaceCapabilities);

	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	if (formatCount != 0) {
		swapChainDetails.formats.resize(formatCount);
		
		vkGetPhysicalDeviceSurfaceFormatsKHR(
			device, surface, &formatCount, swapChainDetails.formats.data());
	}

	uint32_t presentationCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationCount, nullptr);

	if (presentationCount != 0) {
		swapChainDetails.presentationModes.resize(presentationCount);
		
		vkGetPhysicalDeviceSurfacePresentModesKHR(
			device, surface, &presentationCount, swapChainDetails.presentationModes.data());
	}


	return swapChainDetails;
}

VkSurfaceFormatKHR VulkanRenderer::chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
{
	// VK_FORMAT_UNDEFINED means that all formats are available
	
	if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED) {
		VkSurfaceFormatKHR surfaceFormat{};

		surfaceFormat.format = VK_FORMAT_R8G8B8A8_UNORM;
		surfaceFormat.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
		
		return surfaceFormat;
	}

	for (const auto& format : formats) {
		bool isBestFormat = format.format == VK_FORMAT_R8G8B8A8_UNORM
			|| format.format == VK_FORMAT_B8G8R8A8_UNORM; // Backup

		if (isBestFormat && format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) {
			return format;
		}
	}

	return formats[0];
}

VkPresentModeKHR VulkanRenderer::chooseBestPresentationMode(const std::vector<VkPresentModeKHR> presentationModes)
{
	for (const auto& presentationMode : presentationModes) {
		if (presentationMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return presentationMode;
		}
	}

	// Safe back up
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanRenderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities)
{
	// If the width is the max uint32_t then it's undefined and we just set the resolution of the current
	// framebuffer from glfw

	if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return surfaceCapabilities.currentExtent;
	}

	int width{}, height{};
	
	glfwGetFramebufferSize(window, &width, &height);

	VkExtent2D extent = {};
	extent.width = static_cast<uint32_t>(width);
	extent.height = static_cast<uint32_t>(height);
		
	uint32_t maxThresholdWidth = std::min(
		surfaceCapabilities.maxImageExtent.width, extent.width);

	uint32_t minThresholdWidth = std::max(
		surfaceCapabilities.minImageExtent.width, maxThresholdWidth);
		
	extent.width = minThresholdWidth;

	uint32_t maxThresholdHeight = std::min(
		surfaceCapabilities.maxImageExtent.height, extent.height);

	uint32_t minThresholdHeight = std::max(
		surfaceCapabilities.minImageExtent.height, maxThresholdHeight);

	extent.height = minThresholdHeight;

	return extent;
}

VkImageView VulkanRenderer::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
	VkImageViewCreateInfo viewCreateInfo = {};
	viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewCreateInfo.image = image;									// Image to create view for
	viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;				// Type of image (1D, 2D, 3D etc...)
	viewCreateInfo.format = format;									// Format of iamge data
	viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;	// Allows remapping of RGBA components to other rgba values
	viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	// Subresources allow the view to view only a prt of an image
	viewCreateInfo.subresourceRange.aspectMask = aspectFlags;		// Which aspect of image to view( e.g. COLOR_BIT for viewing color)
	viewCreateInfo.subresourceRange.baseMipLevel = 0;				// Start mipmap level to view from
	viewCreateInfo.subresourceRange.levelCount = 1;					// Number of mipmap levels to view
	viewCreateInfo.subresourceRange.baseArrayLayer = 0;				// Start array level to view from
	viewCreateInfo.subresourceRange.layerCount = 1;					// Number of array levels to view

	VkImageView imageView;

	VkResult result = vkCreateImageView(mainDevice.logicalDevice, &viewCreateInfo, nullptr, &imageView);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("error occurred while creating image view");
	}


	return imageView;
}

VkShaderModule VulkanRenderer::createShaderModule(const std::vector<char>& code)
{
	// Shader module creation information
	VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
	
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.codeSize = code.size();										// Size of code
	shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());		// Pointer to code

	VkShaderModule shaderModule;
	
	VkResult result = vkCreateShaderModule(
		mainDevice.logicalDevice, &shaderModuleCreateInfo, nullptr, &shaderModule);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("error occurred while creating shader module");
	}

	return shaderModule;
}
