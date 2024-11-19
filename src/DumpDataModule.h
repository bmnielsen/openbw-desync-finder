#pragma once

#include "BWAPI.h"
#include <fstream>
#include "UnitData.h"

class DumpDataModule : public BWAPI::AIModule
{
public:
    void onStart() override
    {
        filename = BWAPI::Broodwar->mapPathName() + ".unitdata.csv";
        file.open(filename, std::ofstream::trunc);

        // Header row
        file << "Frame;Player;";
        UnitData::outputCSVHeader(file);
        file << "\n";

        std::cout << "Started replay dump of " << BWAPI::Broodwar->mapPathName() << std::endl;

        BWAPI::Broodwar->setLocalSpeed(0);
    }

    void onFrame() override
    {
        for (int playerIdx = 0; playerIdx < 2; playerIdx++)
        {
            for (auto unit : BWAPI::Broodwar->getPlayer(playerIdx)->getUnits())
            {
                if (!unit->exists()) continue;

                file << BWAPI::Broodwar->getFrameCount() << ";" << playerIdx << ";";
                UnitData(unit).outputToCSV(file);
                file << "\n";
            }
        }

        file.flush();

        if (BWAPI::Broodwar->getFrameCount() > 0 && BWAPI::Broodwar->getFrameCount() % 1000 == 0)
        {
            std::cout << "...Processed " << BWAPI::Broodwar->getFrameCount() << " frames of replay" << std::endl;
        }
    }

    void onEnd(bool) override
    {
        file.close();
        std::cout << "Game ended, unit data written to " << filename << std::endl;
    }

private:
    std::string filename;
    std::ofstream file;
};
