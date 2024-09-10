#include "VulkanRenderer.h"


int VulkanRenderer::init(GLFWwindow * newWindow) {
	window = newWindow;

	try {
		createInstance();
		createSurface();
		getPhysicalDevice();
		createLogicalDevice();
	}
	catch(const std::runtime_error &e){
		printf("ERROR: %s\n", e.what());

		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

void VulkanRenderer::cleanup()
{
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

	return indices.isValid() && extensionsSupported;
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
