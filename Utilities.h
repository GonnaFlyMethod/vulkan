#pragma once

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