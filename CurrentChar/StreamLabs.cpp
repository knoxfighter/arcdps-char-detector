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
						loadCurrentSources();
						subscribeToSceneServices();
						authorized = true;
					} else {
						authorized = false;
						errorMessage = "Unable to authorize user";
					}
				} else if (id == loadSourcesId) {
					// answer of loading all sources
					JSON_Object* resultObject = json_object_get_object(messageObject, "result");
					if (resultObject != nullptr) {
						processCurrentSources(resultObject);
					}
				} else if (id == 0) {
					// no id provided, see if this is an even
					JSON_Object* resultObject = json_object_get_object(messageObject, "result");
					if (std::string(json_object_get_string(resultObject, "_type")) == "EVENT") {
						if (std::string(json_object_get_string(resultObject, "resourceId")) == "ScenesService.sceneSwitched") {
							// the scene is switched, so reload the scene and run visibility
							JSON_Object* dataObject = json_object_get_object(resultObject, "data");
							processCurrentSources(dataObject);
						} else if (std::string(json_object_get_string(resultObject, "resourceId")) == "ScenesService.itemAdded") {
							// an item was added to the scene
							loadCurrentSources();
						} else if (std::string(json_object_get_string(resultObject, "resourceId")) == "ScenesService.itemUpdated") {
							// an item in the scene got updated
							loadCurrentSources();
						}
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

void StreamLabs::loadCurrentSources() {
	loadSourcesId = ++nextId;
	char buf[1024] = {};
	snprintf(buf, 1024,
	         R"({"jsonrpc": "2.0","id": %u,"method": "activeScene","params": {"resource": "ScenesService"}})",
	         loadSourcesId);
	webSocket.send(buf);
}

void StreamLabs::processCurrentSources(JSON_Object* result) {
	// get the nodes array, nodes is an array of objects, that contain all information to a scene element
	JSON_Array* nodes = json_object_get_array(result, "nodes");

	// clear previous sources
	sources.clear();

	// save new sources into the array
	for (size_t i = 0; i < json_array_get_count(nodes); ++i) {
		JSON_Object* node = json_array_get_object(nodes, i);
		const char* name = json_object_get_string(node, "name");
		std::string resourceId = json_object_get_string(node, "resourceId");

		sources.insert({name, resourceId});
	}

	updateVisibility();
}

void StreamLabs::updateVisibility() {
	// dont run if not authorized
	if (!authorized) return;

	// get sourceName for current Character
	Settings& settings = Settings::instance();
	const std::vector<Settings::Character>& characters = settings.getCharacters();
	std::string characterSourceName;
	for (const Settings::Character& character : characters) {
		if (character.charName == currentCharacter) {
			characterSourceName = character.sourceName;
			break;
		}
	}

	// show only the correct sources
	// dont do anything, if the currentCharacter has no source to it or the source is not in the active scene
	if (!characterSourceName.empty() && sources.contains(characterSourceName)) {
		for (const auto& [charName, sourceName] : characters) {
			if (sources.contains(sourceName)) {
				auto resourceId = sources.at(sourceName);
				// set Visibility
				char buf[4096] = {};

				// replace single backslashes to double backslashes, so they will get sent correctly
				std::string toReplace = "\"";
				std::string toInsert = "\\\"";
				// Get the first occurrence
				size_t pos = resourceId.find(toReplace);
				// Repeat till end is reached
				while (pos != std::string::npos) {
					resourceId.replace(pos, toReplace.size(), toInsert);
					// Get the next occurrence from the current position
					pos = resourceId.find(toReplace, pos + toInsert.size());
				}

				snprintf(buf, 4096,
				         R"({"jsonrpc": "2.0","id": %u,"method": "setVisibility","params": {"resource": "%s","args": [%s]}})",
				         ++nextId, resourceId.c_str(), characterSourceName == sourceName ? "true" : "false");
				webSocket.send(buf);
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
	subscribeToSceneService("sceneSwitched");
	subscribeToSceneService("itemAdded");
	subscribeToSceneService("itemUpdated");
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
