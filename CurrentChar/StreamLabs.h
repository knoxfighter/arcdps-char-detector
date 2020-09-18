#pragma once

#include <string>
#include <ixwebsocket/IXWebSocket.h>

#include "parson.h"

#define UID uint16_t

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
			changeResourceLocalFile();
		}
	}

	void loadChatCoverResourceId();

private:
	// copy/move etc. will be deleted implicitly
	StreamLabs();

	// ws setup / connect
	void setupWebsocket();
	ix::WebSocket webSocket;
	bool connectionEstablished = false;

	// unique ID for every json-rpc call
	UID nextId = 1;

	// last id for an auth call
	UID authId = 0;
	bool authorized = false;

	// Save the ChatCover ResourceId
	std::string chatCoverResourceId;
	UID loadChatCoverId = 0;

	// tracking
	std::string username;
	std::string currentCharacter;

	// change local file of resource
	void changeResourceLocalFile();

	void subscribeToSceneService(const std::string& method);
	void subscribeToSceneServices();

	std::string replaceString(const std::string& str, const std::string& toReplace, const std::string& toInsert);
};
