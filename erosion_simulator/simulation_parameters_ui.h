#pragma once

struct SimulationParametersUI
{
	SimulationParametersUI(bool enableRegenerate)
	{
		showRegenButton = enableRegenerate;
	}

	bool saveHeightMapRequested = false;
	char* fileSaveName = new char(100);

	bool showRegenButton = false;
	bool regenerateHeightMapRequested = false;
};