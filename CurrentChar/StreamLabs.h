#pragma once

#include <string>
#include <ixwebsocket/IXWebSocket.h>

#include "parson.h"

class StreamLabs {
public:
	static StreamLabs& instance();
	std::string errorMessage = "Trying to connect...";
	void sendAuth();

	// getter
	[[nodiscard]] bool isConnectionEstablished() const {
		return connectionEstablished;
	}
	[[nodiscard]] bool isAuthorized() const {
		return authorized;
	}
	[[nodiscard]] const std::string& getUsername() const {
		return username;
	}

	void setUsername(const std::string& username) {
		this->username = username;
	}

	[[nodiscard]] const std::string& getCurrentCharacter() const {
		return currentCharacter;
	}

	void setCurrentCharacter(const std::string& newCharacter) {
		const bool reload = this->currentCharacter != newCharacter;
		this->currentCharacter = newCharacter;
		if (reload) {
			updateVisibility();
		}
	}

private:
	// copy/move etc. will be deleted implicitly
	StreamLabs();

	// ws setup / connect
	void setupWebsocket();
	ix::WebSocket webSocket;
	bool connectionEstablished = false;

	// unique ID for every json-rpc call
	uint16_t nextId = 1;

	// last id for an auth call
	uint16_t authId = 0;
	bool authorized = false;

	// load sources of the active scene
	void loadCurrentSources();
	uint16_t loadSourcesId = 0;

	// process active scenes elements
	void processCurrentSources(JSON_Object* result);
	// Key is "name", value is "resourceId"
	std::unordered_map<std::string, std::string> sources;

	// tracking
	std::string username;
	std::string currentCharacter;

	// change Visibility of all defined elements
	void updateVisibility();

	void subscribeToSceneService(const std::string& method);
	void subscribeToSceneServices();
};
