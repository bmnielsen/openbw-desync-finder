#pragma once

#include "BWAPI.h"
#include "BWAPI/GameImpl.h"
#include <fstream>

#define LOGGING false

class StandaloneDesyncFinderModule : public BWAPI::AIModule
{
public:
    void onStart() override
    {
        std::cout << BWAPI::Broodwar->mapPathName() << std::flush;
#if LOGGING
        std::cout << std::endl;
#endif

        BWAPI::Broodwar->setLocalSpeed(0);
        BWAPI::Broodwar->setLatCom(false);
    }

    void onFrame() override
    {
        // Check every 25 frames
        if (BWAPI::Broodwar->getFrameCount() < 50 || BWAPI::Broodwar->getFrameCount() % 25 != 0) return;

#if !LOGGING
        if (BWAPI::Broodwar->getFrameCount() % 500 == 0)
        {
            std::cout << "\r" << BWAPI::Broodwar->mapPathName() << ": " << BWAPI::Broodwar->getFrameCount()
                      << " - " << unitsThatHaveNotBeenOrdered.size()
                      << "        " << std::flush;
        }
#endif

        // First detect any new units and add them to the list of units that haven't been ordered
        // We only consider units that can attack and aren't buildings
        auto addNewUnits = [&](int playerId)
        {
            for (const auto &unit : BWAPI::Broodwar->getPlayer(playerId)->getUnits())
            {
                if (!unit->exists()) continue;
                if (!unit->isCompleted()) continue;
                if (unit->getType() == BWAPI::UnitTypes::Zerg_Larva) continue;
                if (unit->getType() == BWAPI::UnitTypes::Zerg_Egg) continue;
                if (unit->getType() == BWAPI::UnitTypes::Protoss_Interceptor) continue;
                if (unit->getType() == BWAPI::UnitTypes::Protoss_Scarab) continue;
                if (unit->getType() == BWAPI::UnitTypes::Terran_Vulture_Spider_Mine) continue;
                if (unit->getType().isBuilding()) continue;
                if (unit->getType().groundWeapon() == BWAPI::WeaponTypes::None && unit->getType().airWeapon() == BWAPI::WeaponTypes::None) continue;

                if (seenIDs.contains(unit->getID())) continue;
                seenIDs.insert(unit->getID());

                unitsThatHaveNotBeenOrdered.emplace_back(unit, playerId, BWAPI::Broodwar->getFrameCount());
#if LOGGING
                std::cout << BWAPI::Broodwar->getFrameCount() << ": Added " << unit->getID() << " (" << unit->getType() << ")" << std::endl;
#endif
            }
        };
        addNewUnits(0);
        addNewUnits(1);

        // Next, remove any units from the list that have been ordered
        for (auto it = unitsThatHaveNotBeenOrdered.begin(); it != unitsThatHaveNotBeenOrdered.end(); )
        {
            const auto &unit = std::get<0>(*it);
            if (unit->getOrder() != BWAPI::Orders::None
                && unit->getOrder() != BWAPI::Orders::Nothing
                && unit->getOrder() != BWAPI::Orders::PickupIdle
                && unit->getOrder() != BWAPI::Orders::Guard
                && unit->getOrder() != BWAPI::Orders::PlayerGuard)
            {
#if LOGGING
                std::cout << BWAPI::Broodwar->getFrameCount() << ": Clearing " << unit->getID()
                          << "; order is " << unit>getOrder() << std::endl;
#endif

                it = unitsThatHaveNotBeenOrdered.erase(it);
            }
            else
            {
                it++;
            }
        }
    }

    void onEnd(bool) override
    {
        auto isProbableDesync = [&]()
        {
            if (unitsThatHaveNotBeenOrdered.size() < 10) return false;

            // Ensure the split of unit owners is reasonably even
            // Sometimes one of the players will just not utilize their units efficiently, e.g. leave workers unordered if there are no available
            // patches to mine
            unsigned long playerOneUnits = 0;
            unsigned long playerTwoUnits = 0;
            for (const auto &[unit, playerId, _] : unitsThatHaveNotBeenOrdered)
            {
                ((playerId == 0) ? playerOneUnits : playerTwoUnits)++;
            }
            if ((playerOneUnits * 4) > playerTwoUnits && (playerTwoUnits * 4) > playerOneUnits)
            {
                return true;
            }
            return false;
        };
        if (!isProbableDesync())
        {
            std::cout << "\r" << BWAPI::Broodwar->mapPathName() << ": OK" << std::endl;
        }
        else
        {
            std::cout << "\r" << BWAPI::Broodwar->mapPathName() << ": DESYNC" << std::endl;
            std::cout << "Not ordered units: ";
            for (const auto &[unit, _, frame] : unitsThatHaveNotBeenOrdered)
            {
                std::cout << "\n" << unit->getType() << " @ " << unit->getTilePosition() << " first seen at " << frame;
            }
        }
    }

    static void checkForDesync(const std::string &replayFile)
    {
        BW::GameOwner gameOwner;
        BWAPI::BroodwarImpl_handle h(gameOwner.getGame());
        BWAPI::BroodwarImpl.bwgame.setMapFileName(replayFile);

        h->createSinglePlayerGame([&]()
                                  {
                                      h->startGame();
                                  });

        auto module = StandaloneDesyncFinderModule();
        h->setAIModule(&module);

        while (!gameOwner.getGame().gameOver())
        {
            h->update();
            gameOwner.getGame().nextFrame();
        }
        module.onEnd(false);
    }

private:
    std::unordered_set<int> seenIDs;
    std::vector<std::tuple<BWAPI::Unit, int, int>> unitsThatHaveNotBeenOrdered;
};
