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
                auto &data = frameToPlayerToUnits[frame][player];

                // Parse the unit data and break if there is an error
                if (!UnitData::parseCSVLineAndEmplace(std::span<std::string>(line.begin() + 2, line.end()), data, lineNumber))
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
    }

    void onFrame() override
    {
        bool foundDesync = false;

        auto &frameData = frameToPlayerToUnits[BWAPI::Broodwar->getFrameCount()];

        for (int playerIdx = 0; playerIdx < 2; playerIdx++)
        {
            auto &playerData = frameData[playerIdx];

            for (auto unit : BWAPI::Broodwar->getPlayer(playerIdx)->getUnits())
            {
                if (!unit->exists()) continue;

                auto unitData = UnitData(unit);

                // Find a matching unit in the player data
                bool found = false;
                for (auto it = playerData.begin(); it != playerData.end(); it++)
                {
                    if (unitData == *it)
                    {
                        found = true;
                        playerData.erase(it);
                        break;
                    }
                }

                if (!found)
                {
                    std::cout << "Could not find match in data file: " << unitData << std::endl;
                    foundDesync = true;
                }
            }

            if (!playerData.empty())
            {
                for (const auto &unitData : playerData)
                {
                    std::cout << "Unmatched unit in data file: " << unitData << std::endl;
                }
                foundDesync = true;
            }
        }

        if (foundDesync)
        {
            std::cout << "Desync found at frame " << BWAPI::Broodwar->getFrameCount() << "; stopping processing" << std::endl;
            exit(0);
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
    std::map<int, std::array<std::vector<UnitData>, 2>> frameToPlayerToUnits;
};
