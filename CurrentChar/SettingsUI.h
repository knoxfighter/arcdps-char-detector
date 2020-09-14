#pragma once

#include "imgui/imgui.h"

#include <vector>

struct SettingsUI
{
private:
	std::vector<char*> sourcesBuffer;
	char tokenBuffer[1024] = {0};

public:
	void draw(const char* title, bool* p_open, ImGuiWindowFlags flags);
};

