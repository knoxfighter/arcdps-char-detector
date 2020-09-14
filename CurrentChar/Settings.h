#pragma once

#include <vector>
#include <string>

struct SettingsUI;

class Settings
{
    friend SettingsUI;
	
public:
    struct Character {
        std::string charName;
        std::string sourceName;
    };

    static Settings& instance();

	// getter/setter
    [[nodiscard]] const std::vector<Character>& getCharacters() const;
    [[nodiscard]] const std::string& getStreamLabsToken() const {
	    return streamLabsToken;
    }
    void setStreamLabsToken(const std::string& streamLabsToken);

private:
    // copy/move etc. will be deleted implicitly
    Settings();
    ~Settings();
    void saveToFile();
    void readFromFile();

    std::vector<Character> characters {};
    std::string streamLabsToken;
};

