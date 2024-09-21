#pragma once

#include <fstream>

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

// Indicies (locations) of Queue families

struct QueueFamilyIndices {
	int graphicsFamily = -1; // Location of graphics Queue Family
	int presentationFamily = -1; // Location of presentation queue

	bool isValid() {
		return graphicsFamily >= 0 && presentationFamily >= 0;
	}
};

struct SwapChainDetails {
	VkSurfaceCapabilitiesKHR surfaceCapabilities;  // Surface proporties
	std::vector<VkSurfaceFormatKHR> formats; // Image formats, e.g. RGBA and size of each color in bits
	std::vector<VkPresentModeKHR> presentationModes;  // How images should be presented to the screen
};

struct SwapchainImage {
	VkImage image;
	VkImageView imageView;
};

static std::vector<char> readFile(const std::string& filename) {
	// Open stream from given file
	// std::ios::ate tells to start reading from end of file
	std::ifstream file(filename, std::ios::binary | std::ios::ate);

	// Check if file stream  successfully opened
	if (!file.is_open()) {
		throw std::runtime_error("error occurred while opening a file");
	}

	// Get current read position and use to resize file buffer
	size_t fileSize = (size_t)file.tellg();
	std::vector<char> fileBuffer(fileSize);

	// Move read position (seek to) the start of the file
	file.seekg(0);

	// Read the file data into the buffer (stream "filesize" in total)
	file.read(fileBuffer.data(), fileSize);

	file.close();

	return fileBuffer;
}