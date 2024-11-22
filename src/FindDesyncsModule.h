#pragma once

#include "BWAPI.h"
#include "BWAPI/GameImpl.h"
#include <fstream>
#include "UnitData.h"
#include "BulletData.h"

namespace
{
    template<class T, class U>
    void parseDataFile(const std::string &filename,
                       std::map<int, U> &map)
    {
        std::cout << "Parsing data from " << filename << std::endl;

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
                std::list<T> *dataList;
                auto lineDataStart = line.begin();
                if constexpr (std::is_same<T, UnitData>())
                {
                    int player = std::stoi(line[1]);
                    dataList = &map[frame][player];
                    lineDataStart += 2;
                }
                else
                {
                    dataList = &map[frame];
                    lineDataStart += 1;
                }

                // Parse the unit data and break if there is an error
                if (!T::parseCSVLineAndEmplace(std::span<std::string>(lineDataStart, line.end()), *dataList, lineNumber))
                {
                    break;
                }

                count++;
            }

            std::cout << "...Successfully read " << count << " rows" << std::endl;
        }
        catch (std::exception &ex)
        {
            std::cout << "ERROR: Exception caught attempting to parse data"
                      << " at line " << lineNumber << ": " << ex.what() << std::endl;
        }
    }

    template<class T, class U>
    bool findDesyncs(std::list<T> &bwItems,
                     const BWAPI::SetContainer<U, std::hash<void*>> &openbwItems)
    {
        bool foundDesync = false;

        for (auto bwapiObject : openbwItems)
        {
            if (!bwapiObject->exists()) continue;

            auto data = T(bwapiObject);
            if (data.shouldSkip()) continue;

            // Find the item in the BW data that best matches
            int bestDelta = INT_MAX;
            auto best = bwItems.end();
            for (auto it = bwItems.begin(); it != bwItems.end(); it++)
            {
                int delta = data.delta(*it);
                if (delta == 0)
                {
                    // Found a perfect match, can stop now
                    bwItems.erase(it);
                    bestDelta = 0;
                    break;
                }

                // Often the IDs will be the same, so if we see one with the same ID and a low delta, use it as the match
                if (data.id == it->id && delta < 100)
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

            // We didn't find an exact match, but treat items as likely being the same if they are close enough
            if (bestDelta < 100)
            {
                auto differences = data.differences(*best);
                if (!differences.empty())
                {
                    std::cout << "Frame " << BWAPI::Broodwar->getFrameCount() << ": Values mismatch for "
                              << bwapiObject->getType() << " @ " << BWAPI::WalkPosition(bwapiObject->getPosition()) << ": "
                              << differences << std::endl;
                }

                bwItems.erase(best);
            }
            else
            {
                std::cout << "Frame " << BWAPI::Broodwar->getFrameCount() << ": OpenBW item not found in BW data: " << data << std::endl;
            }
        }

        if (!bwItems.empty())
        {
            for (const auto &data : bwItems)
            {
                std::cout << "Frame " << BWAPI::Broodwar->getFrameCount() << ": BW item not found in OpenBW: " << data << std::endl;
            }
            foundDesync = true;
        }

        return foundDesync;
    }
}

class FindDesyncsModule : public BWAPI::AIModule
{
public:
    void onStart() override
    {
        auto unitFilename = BWAPI::Broodwar->mapPathName() + ".unitdata.csv";
        auto bulletFilename = BWAPI::Broodwar->mapPathName() + ".bulletdata.csv";

        parseDataFile<UnitData>(BWAPI::Broodwar->mapPathName() + ".unitdata.csv", frameToPlayerToUnitData);
        parseDataFile<BulletData>(BWAPI::Broodwar->mapPathName() + ".bulletdata.csv", frameToBulletData);

        std::cout << "Starting replay run" << std::endl;

        BWAPI::Broodwar->setLocalSpeed(0);
        BWAPI::Broodwar->setLatCom(false);
    }

    void onFrame() override
    {
        if (BWAPI::Broodwar->getFrameCount() > 1 && BWAPI::Broodwar->getFrameCount() % 1000 == 1)
        {
            std::cout << "...Processed " << (BWAPI::Broodwar->getFrameCount()-1) << " frames of replay" << std::endl;
        }

        if (desyncStartedAt != -1) return;

        // Skip if we have no BW data for this frame - we may have used a frame skip when exporting the data
        if (!frameToPlayerToUnitData.contains(BWAPI::Broodwar->getFrameCount()) &&
            !frameToBulletData.contains(BWAPI::Broodwar->getFrameCount()))
        {
            return;
        }

        bool foundDesync = false;
        foundDesync = findDesyncs(frameToPlayerToUnitData[BWAPI::Broodwar->getFrameCount()][0],
                                  BWAPI::Broodwar->neutral()->getUnits()) || foundDesync;
        for (int playerIdx = 0; playerIdx < 2; playerIdx++)
        {
            foundDesync = findDesyncs(frameToPlayerToUnitData[BWAPI::Broodwar->getFrameCount()][playerIdx + 1],
                                      BWAPI::Broodwar->getPlayer(playerIdx)->getUnits()) || foundDesync;
        }
        foundDesync = findDesyncs(frameToBulletData[BWAPI::Broodwar->getFrameCount()],
                                  BWAPI::Broodwar->getBullets()) || foundDesync;

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
                desyncStartedAt = BWAPI::Broodwar->getFrameCount() - 9;
            }
        }
    }

    static int getDesyncFrame(const std::string &replayFile)
    {
        BW::GameOwner gameOwner;
        BWAPI::BroodwarImpl_handle h(gameOwner.getGame());
        BWAPI::BroodwarImpl.bwgame.setMapFileName(replayFile);

        h->createSinglePlayerGame([&]()
                                  {
                                      h->startGame();
                                  });

        auto module = FindDesyncsModule();
        h->setAIModule(&module);

        while (!gameOwner.getGame().gameOver())
        {
            h->update();
            gameOwner.getGame().nextFrame();

            if (module.desyncStartedAt != -1) return module.desyncStartedAt;
        }

        return INT_MAX;
    }

private:
    int consecutiveDesyncFrames = 0;
    int lastDesyncFrame = -2;
    int desyncStartedAt = -1;
    std::map<int, std::array<std::list<UnitData>, 3>> frameToPlayerToUnitData;
    std::map<int, std::list<BulletData>> frameToBulletData;
};
