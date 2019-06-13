#include <iostream>
#include <vector>
#include <memory>

typedef std::shared_ptr<std::vector<uint8_t>> tSharedBuffer;

#include "json.hpp"
using Json = nlohmann::json;


#include "d64.hpp"
#include "resource.hpp"
#include "epyx_highscores.hpp"
#include "records.hpp"


extern std::shared_ptr<cResource> gResources;
extern std::vector<sKnownGame> mKnownGames;
extern std::shared_ptr<cRecords> gRecords;

sKnownGame KnownGameByID(eGames pGame);

// Read a word from the buffer
inline uint16_t readWord(const void* buffer) {
	const uint16_t* wordBytes = (const uint16_t*)buffer;
	return *wordBytes;
}

// Read a word from the buffer
inline uint16_t readLEWord(const void* buffer) {
	const uint16_t* wordBytes = (const uint16_t*)buffer;
	return *wordBytes;
}

inline void writeLEWord(const void* buffer, uint16_t pValue) {
	uint16_t* wordBytes = (uint16_t*)buffer;
	*wordBytes = pValue;
}

