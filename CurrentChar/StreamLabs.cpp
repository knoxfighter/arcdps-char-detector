#include "StreamLabs.h"

#include <ixwebsocket/IXNetSystem.h>

#include "Settings.h"

StreamLabs& StreamLabs::instance() {
	static StreamLabs instance;
	return instance;
}

StreamLabs::StreamLabs() {
	setupWebsocket();
}

void StreamLabs::setupWebsocket() {
	// Required on Windows
	ix::initNetSystem();

	std::string url("ws://127.0.0.1:59650/api/websocket");
	webSocket.setUrl(url);

	// Setup a callback to be fired (in a background thread, watch out for race conditions !)
	// when a message or an event (open, close, error) is received
	webSocket.setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) {
		switch (msg->type) {
		case ix::WebSocketMessageType::Message:
			{
				JSON_Value* messageValue = json_parse_string(msg->str.c_str());
				JSON_Object* messageObject = json_value_get_object(messageValue);
				double id = json_object_get_number(messageObject, "id");
				if (id == authId) {
					// react to an authorization
					if (json_object_get_boolean(messageObject, "result") == 1) {
						authorized = true;
						loadChatCoverResourceId();
						subscribeToSceneServices();
					} else {
						authorized = false;
						errorMessage = "Unable to authorize user";
					}
				} else if (id == loadChatCoverId) {
					// answer of getting resourceID
					JSON_Array* resultArray = json_object_get_array(messageObject, "result");
					if (resultArray != nullptr) {
						for (size_t i = 0; i < json_array_get_count(resultArray); ++i) {
							JSON_Object* resultObject = json_array_get_object(resultArray, i);
							
							const char* resourceId = json_object_get_string(resultObject, "resourceId");
							if (resourceId != nullptr) {
								chatCoverResourceId = replaceString(resourceId, "\"", "\\\"");
								changeResourceLocalFile();
								break;
							}
						}
					}
				} else if (id == 0) {
					// no id provided, see if this is an even
					JSON_Object* resultObject = json_object_get_object(messageObject, "result");
					if (std::string(json_object_get_string(resultObject, "_type")) == "EVENT") {
						
					}
				}
				break;
			}
		case ix::WebSocketMessageType::Open:
			{
				connectionEstablished = true;
				sendAuth();
				break;
			}
		case ix::WebSocketMessageType::Error:
			{
				errorMessage = msg->errorInfo.reason;
				connectionEstablished = false;
				authorized = false;
				break;
			}
		case ix::WebSocketMessageType::Close:
			{
				errorMessage = msg->closeInfo.reason;
				connectionEstablished = false;
				authorized = false;
				break;
			}
		}
	});

	// Now that our callback is setup, we can start our background thread and receive messages
	webSocket.start();
}

void StreamLabs::loadChatCoverResourceId() {
	if (!isAuthorized()) {
		return;
	}
	
	Settings& settings = Settings::instance();
	const std::string& sourceName = settings.getChatCoverSourceName();
	
	if (!sourceName.empty()) {
		loadChatCoverId = ++nextId;
		char buf[1024] = {};
		snprintf(buf, 1024,
			R"({"jsonrpc": "2.0","id": %u,"method": "getSourcesByName","params": {"resource": "SourcesService","args": ["%s"]}})",
			loadChatCoverId, sourceName.c_str());
		webSocket.send(buf);
	}
}

void StreamLabs::changeResourceLocalFile() {
	if (!chatCoverResourceId.empty()) {
		Settings& settings = Settings::instance();
		const std::vector<Settings::Character>& characters = settings.getCharacters();
		for (const auto& character : characters) {
			if (character.charName == currentCharacter) {
				if (!character.filePath.empty()) {
					std::string filePath = replaceString(character.filePath, "\\", "\\\\");
					
					char buf[2048];
					snprintf(buf, 2048,
						R"({"jsonrpc": "2.0","id": %u,"method": "updateSettings","params": {"resource": "%s","args": [{"local_file": "%s"}]}})",
						++nextId, chatCoverResourceId.c_str(), filePath.c_str());
					webSocket.send(buf);
				}
			}
		}
	}
}

void StreamLabs::subscribeToSceneService(const std::string& method) {
	char buf[1024] = {};
	snprintf(buf, 1024,
	         R"({"jsonrpc": "2.0","id": %u,"method": "%s","params": {"resource": "ScenesService","args": []}})",
	         ++nextId, method.c_str());
	webSocket.send(buf);
}

void StreamLabs::subscribeToSceneServices() {
	// subscribeToSceneService("sceneSwitched");
	// subscribeToSceneService("itemAdded");
	// subscribeToSceneService("itemUpdated");
}

std::string StreamLabs::replaceString(const std::string& str, const std::string& toReplace, const std::string& toInsert) {
	std::string newStr = str;
	
	// Get the first occurrence
	size_t pos = str.find(toReplace);
	// Repeat till end is reached
	while (pos != std::string::npos) {
		newStr.replace(pos, toReplace.size(), toInsert);
		// Get the next occurrence from the current position
		pos = newStr.find(toReplace, pos + toInsert.size());
	}

	return newStr;
}

void StreamLabs::sendAuth() {
	std::string token = Settings::instance().getStreamLabsToken();
	authId = ++nextId;
	char buf[4096] = {};
	snprintf(buf, 4096,
	         R"({"jsonrpc":"2.0","id":%u,"method":"auth","params":{"resource":"TcpServerService","args":["%s"]}})",
	         authId, token.c_str());
	webSocket.send(buf);
}
