#include "Settings.h"

#include "parson.h"
#include "StreamLabs.h"

Settings& Settings::instance() {
    static Settings b;
    return b;
}

void Settings::setStreamLabsToken(const std::string& streamLabsToken) {
	this->streamLabsToken = streamLabsToken;

	// rerun authorization, if not yet successful
    StreamLabs& streamLabs = StreamLabs::instance();
	if (!streamLabs.isAuthorized()) {
        streamLabs.sendAuth();
	}
}

void Settings::setChatCoverSourceName(const std::string& chatCoverSourceName) {
	this->chatCoverSourceName = chatCoverSourceName;

	StreamLabs& streamLabs = StreamLabs::instance();
	streamLabs.loadChatCoverResourceId();
}

Settings::Settings() {
    // according to standard, this constructor is completely thread-safe
    // read settings from file
    readFromFile();
}

Settings::~Settings() {
    saveToFile();
}

void Settings::saveToFile() {
    // setup json object
    JSON_Value* rootValue = json_value_init_object();
    JSON_Object* rootObject = json_value_get_object(rootValue);

    // write data to json object
    JSON_Value* sourcesValue = json_value_init_array();
    JSON_Array* sourcesArray = json_value_get_array(sourcesValue);

    for (Settings::Character& source : characters) {
        JSON_Value* sourceValue = json_value_init_object();
        JSON_Object* sourceObject = json_value_get_object(sourceValue);
        json_object_set_string(sourceObject, "charName", source.charName.c_str());
        json_object_set_string(sourceObject, "filePath", source.filePath.c_str());

        json_array_append_value(sourcesArray, sourceValue);
    }

    json_object_set_value(rootObject, "sources", sourcesValue);

    json_object_set_string(rootObject, "streamLabsToken", streamLabsToken.c_str());

	json_object_set_string(rootObject, "chatCoverSourceName", chatCoverSourceName.c_str());
    
    json_serialize_to_file_pretty(rootValue, "addons\\arcdps\\arcdps_charDetector.json");
    
    json_value_free(rootValue);
}

void Settings::readFromFile() {
    JSON_Value* rootValue = json_parse_file("addons\\arcdps\\arcdps_charDetector.json");
    if (json_value_get_type(rootValue) != JSONObject) {
        return;
    }
	
	JSON_Object* rootObject = json_value_get_object(rootValue);

    JSON_Array* sourcesArray = json_object_get_array(rootObject, "sources");

	for (size_t i = 0; i < json_array_get_count(sourcesArray); i++) {
        JSON_Object* sourceObject = json_array_get_object(sourcesArray, i);
        Settings::Character character = Settings::Character{};
		const char* charName = json_object_get_string(sourceObject, "charName");
		if (charName != nullptr) {
			character.charName = charName;
		}
        const char* filePath = json_object_get_string(sourceObject, "filePath");
		if (filePath != nullptr) {
			character.filePath = filePath;
		}
        characters.push_back(character);
	}

    const char* streamLabsTokenChar = json_object_get_string(rootObject, "streamLabsToken");
	if (streamLabsTokenChar != nullptr) {
        streamLabsToken = streamLabsTokenChar;
	}

	const char* chatCoverSourceNameChar = json_object_get_string(rootObject, "chatCoverSourceName");
	if (chatCoverSourceNameChar != nullptr) {
		chatCoverSourceName = chatCoverSourceNameChar;
	}

    json_value_free(rootValue);
}

[[nodiscard]] const std::vector<Settings::Character>& Settings::getCharacters() const {
    return characters;
}
