/*

Copyright (c) 2019 Robert Crossfield

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

#include "stdafx.hpp"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <time.h>

static inline std::string ltrim(std::string s, uint8_t pChar) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [pChar](int ch) {
		return ch != pChar;
		}));
	return s;
}
static inline std::string rtrim(std::string s, uint8_t pChar) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [pChar](int ch) {
		return ch != pChar;
		}).base(), s.end());
	return s;
}

#ifndef _MSC_VER
void localtime_s(struct tm* const _Tm, time_t const* const _Time) {

	*_Tm = *localtime(_Time);
}
#endif

std::string sRecordRaw::getName(eGames pGame, size_t mEventID) const {
	if(pGame == eGAME_CALIFORNIA)
		return std::string((const char*)&mName[0] + (mEventID * 1), 0x0A);

	return std::string((const char*)& mName[0], 0x0A);
}

/**
 * Retrieve a raw score, processing it if required
 */
std::string sRecordRaw::getScore(eGames pGame, size_t mEventID) const {
	// Most scores use 7 characters
	std::string res = std::string((const char*)& mScore[0], 0x07);
	switch (pGame) {
	default:
		return res;

		/**
		 * California Games
		 */
	case eGAME_CALIFORNIA:
		res = std::string((const char*)&mScore[0] + (mEventID * 1), 0x0A);
		res = ltrim(res,0x20);
		res.erase(std::remove_if(res.begin(), res.end(), [](auto c) {
			if (c == 0x2E)
				return false;
			return !(c >= 0x20 && c < 0x60);
		}), res.end());
		res = ltrim(res, 0x30);

		switch (mEventID) {
		default:
			return res;

		case 5: // Flying Disk
		case 4:	// BMX
			res.erase(std::remove_if(res.begin(), res.end(), [](auto c) {
				if (c == 0x2E)
					return false;
				return !(c >= 0x20 && c < 0x40);
				}), res.end());
			return res;
		}

		/**
		 * World Games
		 */
	case eGAME_WORLD:
		switch (mEventID) {
		default:
			return res;
		case 6: // Caber
			std::replace(res.begin(), res.end(), (char)0xD6, '\'');
			std::replace(res.begin(), res.end(), (char)0xD5, '"');
			return res;
		case 7: // Sumo
			return std::string((char*)& mScore[0], 0x08);
		}
		/**
		 * Summer Games
		 */
	case eGAME_SUMMER:
		switch (mEventID) {
		default:
			return res;
		case 0: // Pole Vault
			std::replace(res.begin(), res.end(), (char)0xB8, 'm');
			return res;
		}
		/**
		 * Summer Games II
		 */
	case eGAME_SUMMER2:
		switch (mEventID) {
		default:
			return res;
		case 0: // Tripple Jump
		case 4: // High Jump
			std::replace(res.begin(), res.end(), (char)0xB8, 'm');
			return res;
		}
	}
	return res;
}

/**
 * Comparision operator for sRecord
 */
bool sRecord::operator<(const sRecord& pRight) const {
	switch (mKnownGames[pRight.mGameID].mEventSorting[pRight.mEventID]) {
		case 0:	// Less than
			if (mScore < pRight.mScore)
				return true;
			break;

		case 1:	// Greater than
			if (mScore > pRight.mScore)
				return true;
			break;

		default:
			std::cout << "Unknown comparision for scores: " << pRight.mGameID << " " << pRight.mEventID;
			break;
	}
	return false;
}

/**
 * Records constructor. Load previous records from JSON
 */
cRecords::cRecords() {
	mHasChanged = false;

	auto json = gResources->FileRead("records.json");
	if(json->size())
		mRecords = Json::parse(json->begin(), json->end());
}

/**
 * Save records to disk, if they've changed
 */
cRecords::~cRecords() {
	if(mHasChanged)
		gResources->FileSave("records.json", mRecords.dump(1));
}

/**
 * Add a record to the database
 */
bool cRecords::add(sRecordRaw* pRawRecords, sKnownGame pGame, size_t mEventID, size_t mEventMapID) {
	auto playerName = rtrim(rtrim(pRawRecords[mEventID].getName(pGame.mGameID, mEventID), 0x20), 0);
	auto playerScore = ltrim(ltrim(pRawRecords[mEventID].getScore(pGame.mGameID, mEventID), 0x20), 0);

	if (!playerName.size() || !playerScore.size())
		return false;

	if (mRecords.find(pGame.mName) == mRecords.end()) {
		mRecords[pGame.mName] = Json();
	}

	// Look for this record
	for (auto& event_records : mRecords[pGame.mName]) {
		for (auto& event_record : event_records) {
			std::string name = event_record["name"];
			std::string score = event_record["score"];

			if (!playerName.compare(name))
				if (!playerScore.compare(score))
					return false;
		}
	}

	auto& eventName = pGame.mEvents[mEventMapID];
	std::cout << std::setw(20) << eventName << ": ";
	std::cout << std::setw(10) << playerName << " - " << std::setw(10) << playerScore << "\n";

	if (mRecords[pGame.mName].find(eventName) == mRecords[pGame.mName].end()) {
		mRecords[pGame.mName][eventName] = Json();
	}

	auto now = std::chrono::system_clock::now();

	auto record = Json();
	record["name"] = playerName;
	record["score"] = playerScore;
	record["gameid"] = pGame.mGameID;
	record["eventid"] = mEventMapID;
	record["date"] = std::chrono::system_clock::to_time_t(now);

	mRecords[pGame.mName][eventName].emplace_back(record);
	mHasChanged = true;
	return true;
}

/**
 * Import World Records from the "Epyx Games Collection" cartridge
 *  @see: https://csdb.dk/release/?id=107705
 */
bool cRecords::importCartRecords(const std::string& pFile) {

	auto cart = gResources->FileRead(pFile);
	if (!cart)
		return false;
	if (memcmp("C64 CARTRIDGE   ", cart->data(), 0x0F))
		return false;

	uint8_t* raw = (uint8_t*) (cart->data() + 0xBC630);
	for (auto& knowngame : mKnownGames) {
		sRecordRaw* recordPtr = (sRecordRaw*)raw;
		for (size_t id = 0; id < knowngame.mEvents.size(); ++id) {
			size_t AddID = id;

			// Some of WinterGames have changed in the cartridge release
			// So we map them back to the disk order
			switch (knowngame.mGameID) {
				case eGAME_WINTER: {
					switch (id) {
					default:
						break;
					case 2:	// Speed Skating
						AddID = 4;
						break;
					case 3:	// Figure Skating
						AddID = 2;
						break;
					case 4:	// Ski Kump
						AddID = 3;
						break;
					}
				}
				default:
					break;
			}

			gRecords->add(recordPtr, knowngame, id, AddID);
		}

		raw += (0x100);
	}

	return true;
}

/**
 * Import records from a disk
 */
bool cRecords::importRecordsDisk(const std::string& pFile) {
	cD64 Disk(pFile);

	for (auto& knowngame : mKnownGames) {
		// All disks for this game
		for (auto& knowndisk : knowngame.mDisks) {
			// Label match for this game?
			if (Disk.disklabelGet() == knowndisk.mLabel) {
				auto File = Disk.fileGet(knowndisk.mRecordFile);
				if (File) {
					if (File->mFileSize > 2)
						continue;

					std::cout << knowngame.mName << "\n";

					// + 2 to skip the PRG load address
					sRecordRaw* RawRecords = (sRecordRaw*)(File->mBuffer->data() + 2);
					for (size_t id = 0; id < knowngame.mEvents.size(); ++id)
						gRecords->add(RawRecords, knowngame, id, id);
					std::cout << "\n";
				}
			}
		}
	}
	return true;
}

/**
 * Find older records on a disk
 */
bool cRecords::findRecordsDisk(const std::string& pFile) {
	cD64 Disk(pFile);

	for (auto& knowngame : mKnownGames) {
		// All disks for this game
		for (auto& knowndisk : knowngame.mDisks) {
			// Label match for this game?
			if (Disk.disklabelGet() == knowndisk.mLabel) {

				for (uint8_t Track = 1; Track <= Disk.trackCount(); ++Track) {
					for (uint8_t Sector = 0; Sector < Disk.trackRange(Track); ++Sector) {
						auto ptr = Disk.sectorPtr(Track, Sector);
						auto prgLoadAddr = readLEWord(&ptr[0x02]);

						// This is a really crap search for high scores
						// Highscores load to 0x0E00 and 0x0B00
						if (prgLoadAddr == 0x0E00 || prgLoadAddr == 0x0B00) {
							// +4 skip the T/S chain, and the load address
							sRecordRaw* RawRecords = (sRecordRaw*)(ptr + 4);

							bool ok = false;
							for (int x = 0; x < 0x0a; ++x) {
								uint8_t letter = RawRecords[0].mName[x];
								if (std::isprint(letter))
									ok = true;
								else {
									ok = false;
									break;
								}
							}
							if (ok) {
								for (size_t id = 0; id < knowngame.mEvents.size(); ++id)
									gRecords->add(RawRecords, knowngame, id, id);
							}
						}
					}
				}
			}
		}
	}
	return true;
}

/**
 * Get all records by 'pName'
 */
tRecords cRecords::getByName(std::string pName) {
	tRecords result;

	// Each game
	for (auto game_records : mRecords) {

		// Each event
		for (auto& event_records : game_records) {

			// Each record
			for (auto& record : event_records) {
				std::string name = record["name"];

				if (!pName.compare(name)) {
					result.push_back({ record["gameid"], record["eventid"], record["name"], record["score"], record["date"] });
				}
			}
		}
	}

	return result;
}

/**
 * Get all records for 'pGame'
 */
tRecordMap cRecords::getByGame(eGames pGame) {
	tRecordMap result;
	auto game = KnownGameByID(pGame);

	// loop each event in this game
	for (auto& event_records : mRecords[game.mName]) {
		std::vector<sRecord> records;

		// Each record
		for (auto& record : event_records) {
			records.push_back( sRecord{ record["gameid"], record["eventid"], record["name"], record["score"], record["date"] });
		}
		// Enough to sort
		if (records.size() > 1) {
			std::sort(records.begin(), records.end(), [](auto& a, auto& b) {
				return a < b;
			});
		}

		if(records.size())
			result.emplace(std::make_pair(records[0].mEventID, records));
	}

	return result;
}

/**
 * Dump all records for an event
 */
std::string cRecords::dumpRecordsForEvent(const eGames pGame, const size_t pEventID, const tRecords& pRecords) {
	std::stringstream result;
	auto game = KnownGameByID(pGame);

	for (auto& record : pRecords) {

		time_t t = time_t(record.mTimestamp);
		tm tm;
		localtime_s(&tm, &t);
		auto date = std::put_time(&tm, "%Y-%m-%d");

		// Filtering by name?
		if (gParameters.mFilterName.size()) {
			if (str_to_upper(record.mName) != str_to_upper(gParameters.mFilterName))
				continue;
		}

		// Filter records after the provided date
		if (gParameters.mFilterDate) {
			if (gParameters.mFilterDateForward) {
				if (record.mTimestamp < gParameters.mFilterDate)
					continue;
			}
			else {
				// Filter records before the provided date
				if (record.mTimestamp > gParameters.mFilterDate)
					continue;
			}
		}

		result << std::setw(20) << game.mEvents[pEventID] << ": ";
		result << std::setw(10) << record.mName << " - " << std::setw(10) << record.mScore << " on " << std::setw(8) << date << "\n";
	}
	return result.str();
}

/**
 * Dump all records for all games
 */
std::string cRecords::dumpRecordsAll() {
	std::stringstream result;

	// Each Game
	for (auto& game : mKnownGames)
		result << dumpRecordsByGame(game.mGameID);

	return result.str();
}

/**
 * Dump all records for a game
 */
std::string cRecords::dumpRecordsByGame(eGames pGame) {
	std::stringstream result;
	auto game = KnownGameByID(pGame);
	auto events = gRecords->getByGame(pGame);

	result << "\n" << game.mName << "\n";

	for (auto& event : events) {
		result << dumpRecordsForEvent(pGame, event.first, event.second);
	}

	return result.str();
}
