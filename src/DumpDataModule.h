#pragma once

#include "BWAPI.h"
#include <fstream>
#include "UnitData.h"
#include "BulletData.h"

namespace
{
    template<class T>
    void openDataFile(const std::string &filename, std::ofstream &file)
    {
        file.open(filename, std::ofstream::trunc);
        if (T::splitByPlayer())
        {
            file << "Frame;Player;";
        }
        else
        {
            file << "Frame;";
        }
        T::outputCSVHeader(file);
        file << "\n";
    }

    template<class T, class U>
    void outputData(std::ofstream &file, const std::string &prefix, const BWAPI::SetContainer<U, std::hash<void*>> &bwapiObjects)
    {
        for (auto bwapiObject : bwapiObjects)
        {
            if (!bwapiObject->exists()) continue;

            auto data = T(bwapiObject);
            if (data.shouldSkip()) continue;

            file << prefix;
            data.outputToCSV(file);
            file << "\n";
        }
    }
}

class DumpDataModule : public BWAPI::AIModule
{
public:
    void onStart() override
    {
        // Get the frame bounds
        std::ifstream frameBoundsFile;
        frameBoundsFile.open(BWAPI::Broodwar->mapPathName() + ".framebounds.txt");
        if (frameBoundsFile.good())
        {
            std::string lineStr;

            std::getline(frameBoundsFile, lineStr);
            startFrame = std::stoi(lineStr);

            std::getline(frameBoundsFile, lineStr);
            endFrame = std::stoi(lineStr);

            frameBoundsFile.close();
        }

        openDataFile<UnitData>(BWAPI::Broodwar->mapPathName() + ".unitdata.csv", unitFile);
        openDataFile<BulletData>(BWAPI::Broodwar->mapPathName() + ".bulletdata.csv", bulletFile);

        std::cout << "Started replay dump of " << BWAPI::Broodwar->mapPathName() << std::endl;

        BWAPI::Broodwar->setLocalSpeed(0);
        BWAPI::Broodwar->setLatCom(false);
    }

    void onFrame() override
    {
        if (BWAPI::Broodwar->getFrameCount() < startFrame) return;
        if (BWAPI::Broodwar->getFrameCount() == (endFrame + 1))
        {
            unitFile.close();
            bulletFile.close();
        }
        if (BWAPI::Broodwar->getFrameCount() > endFrame) return;

        if (BWAPI::Broodwar->getFrameCount() % 100 == 0)
        {
            unitFile.flush();
            bulletFile.flush();
        }

        outputData<UnitData>(unitFile,
                             (std::ostringstream() << BWAPI::Broodwar->getFrameCount() << ";0;").str(),
                             BWAPI::Broodwar->neutral()->getUnits());
        for (int playerIdx = 0; playerIdx < 2; playerIdx++)
        {
            outputData<UnitData>(unitFile,
                                 (std::ostringstream() << BWAPI::Broodwar->getFrameCount() << ";" << (playerIdx + 1) << ";").str(),
                                 BWAPI::Broodwar->getPlayer(playerIdx)->getUnits());
        }
        outputData<BulletData>(bulletFile,
                               (std::ostringstream() << BWAPI::Broodwar->getFrameCount() << ";").str(),
                               BWAPI::Broodwar->getBullets());

        if (BWAPI::Broodwar->getFrameCount() > 0 && BWAPI::Broodwar->getFrameCount() % 1000 == 0)
        {
            std::cout << "...Processed " << BWAPI::Broodwar->getFrameCount() << " frames of replay" << std::endl;
        }
    }

    void onEnd(bool) override
    {
        unitFile.close();
        bulletFile.close();
    }

private:
    int startFrame = 0;
    int endFrame = INT_MAX;
    std::ofstream unitFile;
    std::ofstream bulletFile;
};
