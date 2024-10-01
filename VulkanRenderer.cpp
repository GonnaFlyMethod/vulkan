#include "VulkanRenderer.h"


int VulkanRenderer::init(GLFWwindow * newWindow) {
	window = newWindow;

	try {
		createInstance();
		createSurface();
		getPhysicalDevice();
		createLogicalDevice();
		createSwapChain();
		createRenderPass();
		createGraphicsPipeline();
		createFramebuffers();
		createCommandPool();

		// Creating mesh

		// Vertex data
		std::vector<Vertex> meshVertices = {

			// 1st triangle
			{
				{-0.9, -0.4, 0.0},	// Vertex position 
				{0.0, 0.0, 1.0}		// Vertex color
			},	// 0
			{
				{-0.1, -0.4, 0.0},	// Vertex position
				{1.0, 0.0, 0.0}		// Vertex color
			},	// 1
			{
				{-0.1, 0.4, 0.0},	// Vertex position
				{1.0, 0.0, 0.0}		// Vertex color
			},	// 2
			{
				{-0.9, 0.4, 0.0},	// Vertex position
				{0.0, 0.0, 1.0}		// Vertex color
			},	// 3

		};

		std::vector<Vertex> meshVertices2 = {

			// 1st triangle
			{
				{0.1, -0.4, 0.0},	// Vertex position 
				{0.0, 0.0, 1.0}		// Vertex color
			},	// 0
			{
				{0.9, -0.4, 0.0},	// Vertex position
				{1.0, 0.0, 0.0}		// Vertex color
			},	// 1
			{
				{0.9, 0.4, 0.0},	// Vertex position
				{1.0, 0.0, 0.0}		// Vertex color
			},	// 2
			{
				{0.1, 0.4, 0.0},	// Vertex position
				{0.0, 0.0, 1.0}		// Vertex color
			},	// 3
		};

		// Index data
		std::vector<uint32_t> meshIndices = {
			0, 1, 2,		// 1st triangle
			2 ,3, 0			// 2nd triangle
		};

		Mesh firstMesh = Mesh(
			mainDevice.physicalDevice,
			mainDevice.logicalDevice,
			graphicsQueue,
			graphicsCommandPool,
			&meshVertices, 
			&meshIndices);

		Mesh secondMesh = Mesh(
			mainDevice.physicalDevice,
			mainDevice.logicalDevice,
			graphicsQueue,
			graphicsCommandPool,
			&meshVertices2,
			&meshIndices);

		meshList.push_back(firstMesh);
		meshList.push_back(secondMesh);

		createCommandBuffers();
		recordCommands();
		createSynchronisation();
	}
	catch(const std::runtime_error &e){
		printf("ERROR: %s\n", e.what());

		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

void VulkanRenderer::draw()
{
	// Wait for given fence to signal (open) from last draw before continuing
	vkWaitForFences(
		mainDevice.logicalDevice, 1, &drawFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());

	// Manually reset (close) fences
	vkResetFences(mainDevice.logicalDevice, 1, &drawFences[currentFrame]);

	// 1. Get next available image to draw to and set something to signal when we're finished with the image (a semaphore)
	// Get index of next image to be drawn to, and signal semaphore when ready to be drawn to
	uint32_t imageIndex;
	
	vkAcquireNextImageKHR(
		mainDevice.logicalDevice, 
		swapchain, 
		std::numeric_limits<uint64_t>::max(), 
		imageAvailable[currentFrame], VK_NULL_HANDLE, &imageIndex);

	// 2. Submit command buffer to queue for execution, making sure it waits for the image to be signaled as avaiable for drawing

	// and signals when it has finished rendering
	// -- SUBMIT COMMAND BUFFER TO RENDER -- 
	// Queue submission information
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;							// Number of semaphores to wait on
	submitInfo.pWaitSemaphores = &imageAvailable[currentFrame];				// List of semaphores to wait on
	
	VkPipelineStageFlags waitStage[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.pWaitDstStageMask = waitStage;
	submitInfo.commandBufferCount = 1;							// Number of command buffers to submit
	submitInfo.pCommandBuffers = &commandBuffers[imageIndex];	// Command buffer to submit
	submitInfo.signalSemaphoreCount = 1;						// Number of semaphores to signal
	submitInfo.pSignalSemaphores = &renderFinished[currentFrame];				// Semaphore to signal when command buffer finishes

	// Submitting command buffer to queue
	VkResult result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, drawFences[currentFrame]);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("error occurred while submitting command buffer to queue");
	}

	// 3. Present image to screen when it has signalled finished rendering

	// -- PRESENT RENDERED IMAGE TO SCREEN
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;							// Number of semaphores to wait on
	presentInfo.pWaitSemaphores = &renderFinished[currentFrame];				// Semaphores to wait on
	presentInfo.swapchainCount = 1;								// Number of swapchains to present to
	presentInfo.pSwapchains = &swapchain;						// Swapchains to present images to
	presentInfo.pImageIndices = &imageIndex;					// Index of images in swapchains to present

	result = vkQueuePresentKHR(presentationQueue, &presentInfo);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("error occurred while presenting an image");
	}

	currentFrame = (currentFrame + 1) % MAX_FRAME_DRAWS;
}

void VulkanRenderer::cleanup()
{
	// Wait until no actions being run on device before destroying
	vkDeviceWaitIdle(mainDevice.logicalDevice);
	
	for (size_t i = 0; i < meshList.size(); i++) {
		meshList[i].destroyBuffers();
	}

	for (size_t i = 0; i < MAX_FRAME_DRAWS;i++) {
		vkDestroySemaphore(mainDevice.logicalDevice, renderFinished[i], nullptr);
		vkDestroySemaphore(mainDevice.logicalDevice, imageAvailable[i], nullptr);
		vkDestroyFence(mainDevice.logicalDevice, drawFences[i], nullptr);
	}

	vkDestroyCommandPool(mainDevice.logicalDevice, graphicsCommandPool, nullptr);

	for (auto framebuffer : swapchainFrameBuffers) {
		vkDestroyFramebuffer(mainDevice.logicalDevice, framebuffer, nullptr);
	}

	vkDestroyPipeline(mainDevice.logicalDevice, graphicsPipeline, nullptr);

	vkDestroyPipelineLayout(mainDevice.logicalDevice, pipelineLayout, nullptr);
	vkDestroyRenderPass(mainDevice.logicalDevice, renderPass, nullptr);


	for (auto image : swapchainImages) {
		vkDestroyImageView(mainDevice.logicalDevice, image.imageView, nullptr);
	}

	vkDestroySwapchainKHR(mainDevice.logicalDevice, swapchain, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyDevice(mainDevice.logicalDevice, nullptr);
	vkDestroyInstance(instance, nullptr);
}

VulkanRenderer::VulkanRenderer()
{
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

void VulkanRenderer::createRenderPass()
{
	// Color attachment of render pass
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = swapchainImageFormat;						// Format to use for attachment
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;					// Number of samples to write for multisampling
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;				// Describes what to do with attachment before rendering
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;				// Describes what to do with attachment after rendering
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;	// Describes what to do with stencil before rendering
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;	// Describes what to do with stencil after rendering

	// Framebuffer data will be stored as an image, but images can be given different data layouts
	// to give optimal use for cirtain operations
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;			// Image data layout before render pass starts
	
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;		// Image data layout after render pass (to change to)

	// Attachment reference uses an attachment index that refers to index in the attachment list passed to renderPassCreateInfo
	VkAttachmentReference colorAttachmentReference = {};
	colorAttachmentReference.attachment = 0;
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// Information about a particular subpass the Render Pass is using
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;		// Pipeline type subpass is to be bound to
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentReference;

	// Need to determine when layout transitions occur using subpass dependencies
	std::array<VkSubpassDependency, 2> subpassDependencies;

	// Coversion from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	// Transition But must happen before...
	subpassDependencies[0].dstSubpass = 0;
	subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependencies[0].dependencyFlags = 0;

	//  and must happen after...
	subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;						// Subpass index (VK_SUBPASS_EXTERNAL = Special value meaning outside of renderpass)
	subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;		// Pipeline stage
	subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;				// Stage access mask (memory access)

	
	// Coversion from VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	// Transition must happen before...
	subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	subpassDependencies[1].dependencyFlags = 0;
	
	// and must happen after...
	subpassDependencies[1].srcSubpass = 0;						// Subpass index (VK_SUBPASS_EXTERNAL = Special value meaning outside of renderpass)
	subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;		// Pipeline stage
	subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;				// Stage access mask (memory access)

	// Create info for render Pass

	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &colorAttachment;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;
	renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
	renderPassCreateInfo.pDependencies = subpassDependencies.data();

	VkResult result = vkCreateRenderPass(
		mainDevice.logicalDevice, &renderPassCreateInfo, nullptr, &renderPass);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("error occurred while creating render pass");
	}
}

void VulkanRenderer::createGraphicsPipeline()
{
	// Read in SPIR-V code of shaders
	auto vertexShaderCode = readFile("Shaders/vert.spv");
	auto fragmentShaderCode = readFile("Shaders/frag.spv");

	// Create Shader Modules
	VkShaderModule vertexShaderModule = createShaderModule(vertexShaderCode);
	VkShaderModule fragmentShaderModule = createShaderModule(fragmentShaderCode);

	// -- SHADER STAGE CREATION INFORMATION --
	// Vertex Stage creation information
	VkPipelineShaderStageCreateInfo vertexShaderCreateInfo = {};
	vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;				// Shader Stage name
	vertexShaderCreateInfo.module = vertexShaderModule;						// Shader module to be used by stage
	vertexShaderCreateInfo.pName = "main";									// Entry point in to shader

	// Fragment Stage creation information
	VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo = {};
	fragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;				// Shader Stage name
	fragmentShaderCreateInfo.module = fragmentShaderModule;						// Shader module to be used by stage
	fragmentShaderCreateInfo.pName = "main";									// Entry point in to shader

	// Put shader stage creation info in to array
	// Graphics Pipeline creation info requires array of shader stage creates
	VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderCreateInfo, fragmentShaderCreateInfo };

	// How the data for a signle vertex (including info such as position, color, texture coords, normals etc.)
	// is as whole
	VkVertexInputBindingDescription bindingDescription = {};
	bindingDescription.binding = 0;										// Can bind multiple streams of data, this defines which one
	bindingDescription.stride = sizeof(Vertex);							// Size of single vertex object
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;			// How to move between data after each vertex
																		// VK_VERTEX_INPUT_RATE_VERTEX		: Move on to the next vertex
																		// VK_VERTEX_INPUT_RATE_INSTANCE	: Move to a vertex for the next instance
	// How the data for an attribe us defined within a vertex
	std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions;

	// Position attribute
	attributeDescriptions[0].binding = 0;								// Which binding the data is at (should be same as above)
	attributeDescriptions[0].location = 0;								// Location in shader where data will be read from
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;		// Format the data will take (also helps define size of data)
	attributeDescriptions[0].offset = offsetof(Vertex, pos);			// Where this attribute is defined in the data for a single vertex

	// Color attribute
	attributeDescriptions[1].binding = 0;								
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex, col);
	
	// -- VERTEX INPUT --
	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
	vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
	vertexInputCreateInfo.pVertexBindingDescriptions = &bindingDescription;												// List of Vertex Binding Descriptions (data spacing/stride information)
	vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data();														// List of Vertex Attribute Descriptions (data format and where to bind to/from)


	// -- INPUT ASSEMBLY --
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;		// Primitive type to assemble vertices as
	inputAssembly.primitiveRestartEnable = VK_FALSE;					// Allow overriding of "strip" topology to start new primitives


	// -- VIEWPORT & SCISSOR --
	// Create a viewport info struct
	VkViewport viewport = {};
	viewport.x = 0.0f;									// x start coordinate
	viewport.y = 0.0f;									// y start coordinate
	viewport.width = (float)swapchainExtent.width;		// width of viewport
	viewport.height = (float)swapchainExtent.height;	// height of viewport
	viewport.minDepth = 0.0f;							// min framebuffer depth
	viewport.maxDepth = 1.0f;							// max framebuffer depth

	// Create a scissor info struct
	VkRect2D scissor = {};
	scissor.offset = { 0,0 };							// Offset to use region from
	scissor.extent = swapchainExtent;					// Extent to describe region to use, starting at offset

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.pViewports = &viewport;
	viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.pScissors = &scissor;


	// -- DYNAMIC STATES --
	// Dynamic states to enable
	//std::vector<VkDynamicState> dynamicStateEnables;
	//dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);	// Dynamic Viewport : Can resize in command buffer with vkCmdSetViewport(commandbuffer, 0, 1, &viewport);
	//dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);	// Dynamic Scissor	: Can resize in command buffer with vkCmdSetScissor(commandbuffer, 0, 1, &scissor);

	//// Dynamic State creation info
	//VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
	//dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	//dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
	//dynamicStateCreateInfo.pDynamicStates = dynamicStateEnables.data();


	// -- RASTERIZER --
	VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
	rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerCreateInfo.depthClampEnable = VK_FALSE;			// Change if fragments beyond near/far planes are clipped (default) or clamped to plane
	rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;	// Whether to discard data and skip rasterizer. Never creates fragments, only suitable for pipeline without framebuffer output
	rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;	// How to handle filling points between vertices
	rasterizerCreateInfo.lineWidth = 1.0f;						// How thick lines should be when drawn
	rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;		// Which face of a tri to cull
	rasterizerCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;	// Winding to determine which side is front
	rasterizerCreateInfo.depthBiasEnable = VK_FALSE;			// Whether to add depth bias to fragments (good for stopping "shadow acne" in shadow mapping)


	// -- MULTISAMPLING --
	VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo = {};
	multisamplingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisamplingCreateInfo.sampleShadingEnable = VK_FALSE;					// Enable multisample shading or not
	multisamplingCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;	// Number of samples to use per fragment


	// -- BLENDING --
	// Blending decides how to blend a new colour being written to a fragment, with the old value

	// Blend Attachment State (how blending is handled)
	VkPipelineColorBlendAttachmentState colourState = {};
	colourState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT	// Colours to apply blending to
		| VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colourState.blendEnable = VK_TRUE;													// Enable blending

	// Blending uses equation: (srcColorBlendFactor * new colour) colorBlendOp (dstColorBlendFactor * old colour)
	colourState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colourState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colourState.colorBlendOp = VK_BLEND_OP_ADD;

	// Summarised: (VK_BLEND_FACTOR_SRC_ALPHA * new colour) + (VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA * old colour)
	//			   (new colour alpha * new colour) + ((1 - new colour alpha) * old colour)

	colourState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colourState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colourState.alphaBlendOp = VK_BLEND_OP_ADD;
	// Summarised: (1 * new alpha) + (0 * old alpha) = new alpha

	VkPipelineColorBlendStateCreateInfo colourBlendingCreateInfo = {};
	colourBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colourBlendingCreateInfo.logicOpEnable = VK_FALSE;				// Alternative to calculations is to use logical operations
	colourBlendingCreateInfo.attachmentCount = 1;
	colourBlendingCreateInfo.pAttachments = &colourState;


	// -- PIPELINE LAYOUT (TODO: Apply Future Descriptor Set Layouts) --
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 0;
	pipelineLayoutCreateInfo.pSetLayouts = nullptr;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

	// Create Pipeline Layout
	VkResult result = vkCreatePipelineLayout(mainDevice.logicalDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Pipeline Layout!");
	}


	// -- DEPTH STENCIL TESTING --
	// TODO: Set up depth stencil testing


	// -- GRAPHICS PIPELINE CREATION --
	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.stageCount = 2;									// Number of shader stages
	pipelineCreateInfo.pStages = shaderStages;							// List of shader stages
	pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;		// All the fixed function pipeline states
	pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
	pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	pipelineCreateInfo.pDynamicState = nullptr;
	pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
	pipelineCreateInfo.pMultisampleState = &multisamplingCreateInfo;
	pipelineCreateInfo.pColorBlendState = &colourBlendingCreateInfo;
	pipelineCreateInfo.pDepthStencilState = nullptr;
	pipelineCreateInfo.layout = pipelineLayout;							// Pipeline Layout pipeline should use
	pipelineCreateInfo.renderPass = renderPass;							// Render pass description the pipeline is compatible with
	pipelineCreateInfo.subpass = 0;										// Subpass of render pass to use with pipeline

	// Pipeline Derivatives : Can create multiple pipelines that derive from one another for optimisation
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;	// Existing pipeline to derive from...
	pipelineCreateInfo.basePipelineIndex = -1;				// or index of pipeline being created to derive from (in case creating multiple at once)

	// Create Graphics Pipeline
	result = vkCreateGraphicsPipelines(mainDevice.logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &graphicsPipeline);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Graphics Pipeline!");
	}

	// Destroy Shader Modules, no longer needed after Pipeline created
	vkDestroyShaderModule(mainDevice.logicalDevice, fragmentShaderModule, nullptr);
	vkDestroyShaderModule(mainDevice.logicalDevice, vertexShaderModule, nullptr);
}

void VulkanRenderer::createFramebuffers()
{
	swapchainFrameBuffers.resize(swapchainImages.size());

	for (size_t i = 0; i < swapchainFrameBuffers.size(); i++) {
		std::array<VkImageView, 1> attachments = {swapchainImages[i].imageView};

		VkFramebufferCreateInfo framebufferCreateInfo = {};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = renderPass;											// Render pass layout the framebuffer will be used with
		framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());		// List of attachments (1:1 with render pass)
		framebufferCreateInfo.pAttachments = attachments.data();
		framebufferCreateInfo.width = swapchainExtent.width;									// Framebuffer width
		framebufferCreateInfo.height = swapchainExtent.height;									// Framebuffer height
		framebufferCreateInfo.layers = 1;														// Framebuffer layers
		
		VkResult result = vkCreateFramebuffer(
			mainDevice.logicalDevice, &framebufferCreateInfo, nullptr, &swapchainFrameBuffers[i]);

		if (result != VK_SUCCESS) {
			throw std::runtime_error("error occurred while creating framebuffer");
		}
	}
}

void VulkanRenderer::createCommandPool()
{
	// Get indices of queue families from device
	QueueFamilyIndices queueFamilyIndices = getQueueFamilies(mainDevice.physicalDevice);

	VkCommandPoolCreateInfo poolInfo = {};

	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;	// Command pool will use this graphics family index for commands

	VkResult result = vkCreateCommandPool(mainDevice.logicalDevice, &poolInfo, nullptr, &graphicsCommandPool);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("error occurred while creating graphics command pool");
	}

}

void VulkanRenderer::createCommandBuffers()
{
	commandBuffers.resize(swapchainFrameBuffers.size());

	VkCommandBufferAllocateInfo commandBufferAllocInfo = {};

	commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocInfo.commandPool = graphicsCommandPool;
	commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;	// VK_COMMAND_BUFFER_LEVEL_PRIMARY - Commands are executed by queue directly
																	// VK_COMMAND_BUFFER_LEVEL_SECONDARY - command buffer is not called directly by queue, 
																	//	but rather called from other buffers via "vkCmdExecuteCommands" when recording commands in primary buffer
	commandBufferAllocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

	// Allocate command buffers and place handles in array of buffers
	VkResult result = vkAllocateCommandBuffers(
		mainDevice.logicalDevice, &commandBufferAllocInfo, commandBuffers.data());

	if (result != VK_SUCCESS) {
		throw std::runtime_error("error occured when allocating command buffers");
	}
}
void VulkanRenderer::createSynchronisation()
{
	imageAvailable.resize(MAX_FRAME_DRAWS);
	renderFinished.resize(MAX_FRAME_DRAWS);
	drawFences.resize(MAX_FRAME_DRAWS);

	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAME_DRAWS; i++) {
		VkResult result = vkCreateSemaphore(mainDevice.logicalDevice, &semaphoreCreateInfo, nullptr, &imageAvailable[i]);
		VkResult result1 = vkCreateSemaphore(
			mainDevice.logicalDevice, &semaphoreCreateInfo, nullptr, &renderFinished[i]);

		VkResult result2 = vkCreateFence(
			mainDevice.logicalDevice, &fenceCreateInfo, nullptr, &drawFences[i]);

		if (result != VK_SUCCESS || result1 != VK_SUCCESS || result2 != VK_SUCCESS) {
			throw std::runtime_error("error occurred when creating synchraziation mechanisms!");
		}
	}

	

}
void VulkanRenderer::recordCommands()
{
	// Information about how to begin each command buffer
	VkCommandBufferBeginInfo bufferBeginInfo = { };
	bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	// Information about how to begin a render pass (only needed for graphical application)
	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = renderPass;								// Render pass to begin
	renderPassBeginInfo.renderArea.offset = { 0,0 };							// Start point of render pass in pix
	renderPassBeginInfo.renderArea.extent = swapchainExtent;					// Size of region to run render pass on (starting at offset)
	
	VkClearValue clearValues[] = {
		{1.0f, 1.0f, 1.0f, 1.0f}		// Color of background, basically 
	};
	renderPassBeginInfo.pClearValues = clearValues;								// List of clear values (TODO: depth attachment clear value)
	renderPassBeginInfo.clearValueCount = 1;

	for (size_t i = 0; i < commandBuffers.size(); i++) {
		renderPassBeginInfo.framebuffer = swapchainFrameBuffers[i];
		
		// Start recording commands to command buffer
		VkResult result = vkBeginCommandBuffer(commandBuffers[i], &bufferBeginInfo);
		if (result != VK_SUCCESS) {
			throw std::runtime_error("error occurred when setting up command buffer");
		}

		vkCmdBeginRenderPass(commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		 
		vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

		for (size_t j = 0; j < meshList.size(); j++) {
			VkBuffer vertexBuffers[] = { meshList[j].getVertexBuffer()};													// Buffers to bind};
			VkDeviceSize offsets[] = { 0 };													// Offsets into buffers being bound

			// Bind mesh index data
			vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);		// Command to bind vertex buffer before drawing

			// Bind mesh vertex data
			vkCmdBindIndexBuffer(
				commandBuffers[i],
				meshList[j].getIndexBuffer(),
				0,
				VK_INDEX_TYPE_UINT32);

			vkCmdDrawIndexed(commandBuffers[i], meshList[j].getIndexCount(), 1, 0, 0, 0);
		}

		vkCmdEndRenderPass(commandBuffers[i]);

		result = vkEndCommandBuffer(commandBuffers[i]);
		if (result != VK_SUCCESS) {
			throw std::runtime_error("error occurred when ending command buffer");
		}
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
