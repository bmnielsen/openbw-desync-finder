#pragma once

#include "BWAPI.h"
#include <fstream>
#include "UnitData.h"

class FindDesyncsModule : public BWAPI::AIModule
{
public:
    void onStart() override
    {
        auto filename = BWAPI::Broodwar->mapPathName() + ".unitdata.csv";
        std::cout << "Parsing replay data from " << filename << std::endl;

        std::ifstream file;
        file.open(filename);
        if (!file.good())
        {
            std::cout << "ERROR: Could not open file " << filename << std::endl;
            return;
        }

        int lineNumber = 0;
        try
        {
            // Read and parse each position
            int count = 0;
            while (true)
            {
                lineNumber++;

                std::vector<std::string> line;
                std::string lineStr;
                std::getline(file, lineStr);
                std::stringstream lineStream(lineStr);
                std::string cell;
                while (std::getline(lineStream, cell, ';'))
                {
                    line.push_back(cell);
                }

                if (line.empty()) break; // at end of file
                if (lineNumber == 1) continue; // header row

                if (line.size() < 3)
                {
                    std::cout << "ERROR: Not enough fields"
                              << " at line " << lineNumber << std::endl;
                }

                // Parse the frame and player and reference the data map
                int frame = std::stoi(line[0]);
                int player = std::stoi(line[1]);
                auto &unitData = frameToPlayerToUnitData[frame][player];

                // Parse the unit data and break if there is an error
                if (!UnitData::parseCSVLineAndEmplace(std::span<std::string>(line.begin() + 2, line.end()), unitData, lineNumber))
                {
                    break;
                }

                count++;
            }

            std::cout << "...Successfully read " << count << " rows" << std::endl;
        }
        catch (std::exception &ex)
        {
            std::cout << "ERROR: Exception caught attempting to parse unit data"
                      << " at line " << lineNumber << ": " << ex.what() << std::endl;
        }

        std::cout << "Starting replay run" << std::endl;

        BWAPI::Broodwar->setLocalSpeed(0);
        BWAPI::Broodwar->setLatCom(false);
    }

    void onFrame() override
    {
        bool foundDesync = false;

        auto &frameData = frameToPlayerToUnitData[BWAPI::Broodwar->getFrameCount()];

        for (int playerIdx = 0; playerIdx < 2; playerIdx++)
        {
            auto &playerData = frameData[playerIdx];

            for (auto unit : BWAPI::Broodwar->getPlayer(playerIdx)->getUnits())
            {
                if (!unit->exists()) continue;

                auto unitData = UnitData(unit);

                // Find the unit in the BW data that best matches
                int bestDelta = INT_MAX;
                auto best = playerData.end();
                for (auto it = playerData.begin(); it != playerData.end(); it++)
                {
                    int delta = unitData.delta(*it);
                    if (delta == 0)
                    {
                        // Found a perfect match, can stop now
                        playerData.erase(it);
                        bestDelta = 0;
                        break;
                    }

                    // Often the IDs will be the same, so if we see one with the same ID and a low delta, use it as the match
                    if (unitData.id == it->id && delta < 100)
                    {
                        bestDelta = delta;
                        best = it;
                        break;
                    }

                    if (delta < bestDelta)
                    {
                        bestDelta = delta;
                        best = it;
                    }
                }
                if (bestDelta == 0) continue; // found an exact match

                foundDesync = true;

                // We didn't find an exact match, but treat units as likely being the same if they are close enough
                if (bestDelta < 100)
                {
                    auto differences = unitData.differences(*best);
                    if (!differences.empty())
                    {
                        std::cout << "Frame " << BWAPI::Broodwar->getFrameCount() << ": Unit values mismatch for "
                                  << unit->getType() << ":" << unit->getBWID() << " @ " << unit->getTilePosition() << ": "
                                  << differences << std::endl;
                    }

                    playerData.erase(best);
                }
                else
                {
                    std::cout << "Frame " << BWAPI::Broodwar->getFrameCount() << ": OpenBW unit not found in BW data: " << unitData << std::endl;
                }
            }

            if (!playerData.empty())
            {
                for (const auto &unitData : playerData)
                {
                    std::cout << "Frame " << BWAPI::Broodwar->getFrameCount() << ": BW unit not found in OpenBW: " << unitData << std::endl;
                }
                foundDesync = true;
            }
        }

        if (foundDesync)
        {
            if (lastDesyncFrame == (BWAPI::Broodwar->getFrameCount() - 1))
            {
                consecutiveDesyncFrames++;
            }
            else
            {
                consecutiveDesyncFrames = 1;
            }
            lastDesyncFrame = BWAPI::Broodwar->getFrameCount();

            if (consecutiveDesyncFrames == 10)
            {
                exit(0);
            }
        }

        if (BWAPI::Broodwar->getFrameCount() > 0 && BWAPI::Broodwar->getFrameCount() % 1000 == 0)
        {
            std::cout << "...Processed " << BWAPI::Broodwar->getFrameCount() << " frames of replay" << std::endl;
        }
    }

    void onEnd(bool) override
    {
        std::cout << "Game ended, no desyncs found" << std::endl;
    }

private:
    int consecutiveDesyncFrames = 0;
    int lastDesyncFrame = -2;
    std::map<int, std::array<std::list<UnitData>, 2>> frameToPlayerToUnitData;
};
