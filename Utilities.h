#pragma once

// Indicies (locations) of Queue families

struct QueueFamilyIndices {
	int graphicsFamily = -1; // Location of graphics Queue Family

	bool isValid() {
		return graphicsFamily >= 0;
	}
};