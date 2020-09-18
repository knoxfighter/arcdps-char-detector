#include "SettingsUI.h"

#include "Settings.h"
#include "StreamLabs.h"

#define windowWidth 800
#define leftItemWidth  200
#define rightItemWidth 600

void SettingsUI::draw(const char* title, bool* p_open, ImGuiWindowFlags flags) {
	ImGui::SetNextWindowSize(ImVec2(windowWidth, 650), ImGuiSetCond_FirstUseEver);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(150, 50));
	ImGui::Begin(title, p_open, flags);

	// show error message of websocket
	StreamLabs& streamLabs = StreamLabs::instance();
	if (!streamLabs.isConnectionEstablished() || !streamLabs.isAuthorized()) {
		ImGui::TextColored(ImVec4{ 1, 0, 0, 1 }, streamLabs.errorMessage.c_str());
	}
	if (streamLabs.isAuthorized()) {
		ImGui::TextColored(ImVec4{0, 1, 0, 1}, "WebSocket connected and authorized");
	} else if (streamLabs.isConnectionEstablished()) {
		ImGui::TextColored(ImVec4{1, 1, 0, 1}, "WebSocket connected");
	}

	Settings& settings = Settings::instance();

	// show StreamLab token password field
	strcpy(tokenBuffer, settings.streamLabsToken.c_str());
	if (ImGui::InputText("Streamlabs OBS token", tokenBuffer, sizeof tokenBuffer, ImGuiInputTextFlags_Password)) {
		settings.setStreamLabsToken(tokenBuffer);
	}

	// show streamLab ChatCover Source name field
	strcpy(sourceNameBuffer, settings.chatCoverSourceName.c_str());
	if (ImGui::InputText("ChatCover Source Name", sourceNameBuffer, sizeof sourceNameBuffer)) {
		settings.chatCoverSourceName = sourceNameBuffer;
	}

	std::vector<Settings::Character>& characters = settings.characters;

	// add header if elements are found
	if (!characters.empty()) {
		ImGui::Text("Character name");
		ImGui::SameLine();
		ImVec2 currentPos = ImGui::GetCursorPos();
		currentPos.x = leftItemWidth + 10;
		ImGui::SetCursorPos(currentPos);
		ImGui::Text("filePath to local video");
	}

	std::vector<size_t> toRemove{};
	size_t bufferPos = 0;
	for (size_t i = 0; i < characters.size(); ++i) {
		Settings::Character& character = characters[i];

		// Set elementWidth
		ImGui::PushItemWidth(leftItemWidth);
		
		// set new sourcesBuffer if needed
		if (bufferPos >= sourcesBuffer.size()) {
			char* addBuffer = new char[128];
			memset(addBuffer, 0, 128);
			strcpy(addBuffer, character.charName.c_str());
			sourcesBuffer.push_back(addBuffer);
		}
		// generate ID ("##" is the delimiter between label and id)
		std::string id = "##";
		id += std::to_string(bufferPos);
		// create the text input, returns true, when something changed
		if (ImGui::InputText(id.c_str(), sourcesBuffer[bufferPos], 128)) {
			character.charName = sourcesBuffer[bufferPos];
		}
		// Set the sourcesBuffer to the next sourcesBuffer element
		++bufferPos;

		// Unset elementWidth
		ImGui::PopItemWidth();

		ImGui::SameLine();

		// set Element width
		ImGui::PushItemWidth(rightItemWidth);
		
		// repeat once for streamlabs source name
		if (bufferPos >= sourcesBuffer.size()) {
			char* addBuffer = new char[128];
			memset(addBuffer, 0, 128);
			strcpy(addBuffer, character.filePath.c_str());
			sourcesBuffer.push_back(addBuffer);
		}
		id = "##";
		id += std::to_string(bufferPos);
		if (ImGui::InputText(id.c_str(), sourcesBuffer[bufferPos], 128)) {
			// value modified
			character.filePath = sourcesBuffer[bufferPos];
		}
		++bufferPos;

		// unset element width
		ImGui::PopItemWidth();

		ImGui::SameLine();

		// remove button
		id = "-##";
		id += std::to_string(bufferPos);
		if (ImGui::Button(id.c_str(), ImVec2(20, 20))) {
			toRemove.push_back(i);
		}
	}

	// Delete Elements, that we dont need anymore
	// run from back to beginning, so vector position dont get messed up
	for (auto it = toRemove.rbegin(); it != toRemove.rend(); ++it) {
		characters.erase(characters.begin() + *it);
		auto remBegin = sourcesBuffer.begin() + (*it * 2);
		sourcesBuffer.erase(remBegin, remBegin + 2);
	}

	if (ImGui::Button("+", ImVec2(20, 20))) {
		// add element to sources
		characters.push_back(Settings::Character{});
	}

	ImGui::End();
	ImGui::PopStyleVar(1);
}
