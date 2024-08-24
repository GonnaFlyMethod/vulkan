#include "VulkanRenderer.h"


int VulkanRenderer::init(GLFWwindow * newWindow) {
	window = newWindow;

	try {
		createInstance();
	}
	catch(const std::runtime_error &e){
		printf("ERROR: %s\n", e.what());

		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
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
